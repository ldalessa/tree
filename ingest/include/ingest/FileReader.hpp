#pragma once

#include "ingest/Tuple.hpp"

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string_view>
#include <tuple>

namespace ingest
{
	struct GenericFileReader {
		virtual ~GenericFileReader() {};
		virtual auto vnext() -> std::optional<Tuple> = 0;
	};
	
    /// This provides a FILE* based parallel file reader.
    ///
    /// The File reader provides next()-based iteration API that will produce a
    /// sequence of Tuples from an input file. It uses CRTP static polymorphism
    /// so that it can be customized by parser type (wmd parser, coffee parser,
    /// etc). It should be initialized with the number of parallel tasks that
    /// are reading the file, and the index of the current task.
    ///
    /// It produces a blocked distribution from the file.
    ///
    /// We target a uniform distribution of block size, but there is no way to
    /// know ahead of time how many records each rank will process.
    ///
    /// We assume that the maximum number of characters per line is 512. This
    /// could be promoted to a user-provided template parameter or a runtime
    /// value if it becomes necessary.
    ///
    /// @param CRTP The derived type, must provide a parse_tuple(char const*,
    ///             char const*) -> std::optional<Tuple> function.
    template <class CRTP>
    struct FileReader : GenericFileReader
    {
        FILE* _input;                     //!< the underlying file
        long n_total;                     //!< the number of total bytes to read
        long n_read{};                    //!< the number of bytes we have read
        char _line[512]{};                //!< the fgets buffer
		std::string_view _path{}; //!< the file path for debugging output

		FileReader(FILE* input, std::uint32_t const n_ranks, std::uint32_t const rank, std::string_view path, long const n_bytes)
				: _input(input)
				, _path(path)
		{
			assert(rank < n_ranks);
            if (_input == nullptr) {
                throw _error("failed to open input file {}\n", _path);
            }

			long offset;
            std::tie(offset, n_total) = _partition(n_bytes, n_ranks, rank);

            // Rank 0 always just starts reading from the beginning of the file,
            // everyone else backs up one character to start in order to deal
            // with the case when the pure partition winds up exactly on the
            // start of one line.
            if (rank != 0) {
                offset -= 1;
                n_total += 1;
            }

            if (std::fseek(_input, offset, SEEK_CUR) != 0) {
                throw _error("failed fseek on {} to offset {}\n", _path, offset);
            }

            // Non-rank-zero partitions advance to the start of the next line.
            if (rank != 0) {
                _read_line();
            }
		}
		
        /// Construct a file reader.
        ///
        /// This will throw std::runtime_error() in various error conditions.
        ///
        /// @param input The input file (already opened).
        /// @param n_ranks The number of ranks to partition across.
        /// @param rank The id of the current rank.
        /// @param path The file path for debugging purposes.
        FileReader(FILE* input, std::uint32_t const n_ranks = 1, std::uint32_t const rank = 0, std::string_view path = {})
                : FileReader(input, n_ranks, rank, path, _get_file_bytes(input, path))
        {
		}
		
        /// Construct a file reader from a path.
        ///
        /// This opens the file and forwards to the FILE* constructor, which may
        /// throw std::runtime_error() in various error conditions.
        ///
        /// @param path The file path.
        /// @param n_ranks The number of ranks to partition across.
        /// @param rank The id of the current rank.
        FileReader(std::string_view path, std::uint32_t const n_ranks = 1, std::uint32_t const rank = 0)
                : FileReader(std::fopen(path.data(), "r"), n_ranks, rank, path)
        {
        }

        /// Destructor for the file reader.
        ~FileReader() {
            if (_input) {
                std::fclose(_input);
            }
        }

        /// Construct the next tuple, and advance the underlying file.
        ///
        /// This will skip lines that the CRTP parsing algorithm cannot create
        /// tuples from (like `#` and any other malformed input that the
        /// underlying parser handles.
        ///
        /// @returns the next tuple in the file, or std::nullopt if we've
        ///          reached the end of our block
        auto next() -> std::optional<Tuple>
        {
            while (_read_line()) {
                if (auto tuple = _derived()->_parse_tuple(_line, _line + sizeof(_line))) {
                    return tuple;
                }
            }
            return std::nullopt;
        }

		auto vnext() -> std::optional<Tuple> override {
			return next();
		}

	  protected:
		/// Create a runtime_error from a format string.
        template <class... Ts>
        static auto _error(std::format_string<Ts...> fmt, Ts&&... ts) -> std::runtime_error {
            return std::runtime_error(std::format(fmt, std::forward<Ts>(ts)...));
        }
		
      private:
        /// Perform a block-based partitioning on a number of bytes.
        ///
        /// This will throw `std::runtime_error`s in various conditions.
        ///
        /// @param n_bytes The total number of bytes to partition.
        /// @param n_ranks The number of partitions.
        /// @param rank The partition id.
        ///
        /// @returns The offset and size of the partition for the rank.
        static auto _partition(long const n_bytes, std::uint32_t const n_ranks, std::uint32_t const rank) -> std::tuple<long, long>
        {
            auto const d = n_bytes / n_ranks;
            auto const r = n_bytes % n_ranks;
            auto const offset = [&] {
                if (rank < r) {
                    return (d + 1) * rank;
                }
                else {
                    return (d + 1) * r + d * (rank - r);
                }
            }();
            auto const length = d + (rank < r);

            if (rank == 0 and offset != 0) {
                throw _error("logic error when partitioning the input file across ranks\n");
            }

            if (rank == n_ranks - 1 and offset + length != n_bytes) {
                throw _error("logic error when partitioning the input file across ranks\n");
            }

            if (n_bytes < offset + length) {
                throw _error("logic error when partitioning the input file across ranks\n");
            }

            return std::tuple(offset, length);
        }

        /// Read a line from the file into the fgets buffer.
        ///
        /// This updates the total number read.
        ///
        /// @returns true on success, false on EOF.
        auto _read_line() -> bool
        {
            if (n_total <= n_read) {
                return false;
            }

            if (std::fgets(_line, sizeof(_line), _input) == nullptr) {
                if (std::feof(_input)) {
                    return false;
                }
                throw _error("could not read from {}\n", _path);
            }

            // Hate this strlen, I wish gets or our parser would return the
            // number of strings.
            auto const read = strnlen(_line, sizeof(_line));
            if (read == sizeof(_line)) {
                throw _error("file line exceeded temporary buffer length {} file {}\n", sizeof(_line), _path);
            }
            n_read += read;
            return true;
        }

		/// Figure out how many bytes in the file.
		///
		/// This will reset the input file pointer to the beginning of the file.
		static auto _get_file_bytes(FILE* input, std::string_view path) -> long
		{
			assert(input);
            if (std::fseek(input, 0, SEEK_END) != 0) {
                throw _error("failed to seek the end of the file {}\n", path);
            }

            auto const n_bytes = std::ftell(input);
            if (n_bytes < 0) {
                throw _error("failed ftell on {}\n", path);
            }

            if (std::fseek(input, 0, SEEK_SET) != 0) {
                throw _error("failed to seek the back to the beginning of the file {}\n", path);
            }

			return n_bytes;
		}
		
        /// CRTP derived utilities.
        auto _derived() const -> CRTP const* {
            return static_cast<CRTP const*>(this);
        }

        auto _derived() -> CRTP* {
            return static_cast<CRTP*>(this);
        }
    };
}
