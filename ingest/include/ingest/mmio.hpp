#pragma once

#include "ingest/FileReader.hpp"
#include "ingest/Tuple.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace ingest::mmio
{
	/// Parse a tuple from a csv string.
    auto parse_tuple(char const*, char const*) -> std::optional<Tuple>;
    auto parse_tuple(std::string_view) -> std::optional<Tuple>;
	auto print_tuple(Tuple const&) -> std::string;
	
	struct Reader : FileReader<Reader>
	{
		int _n;
		int _m;
		int _nnz;
		
		Reader(std::string_view path, std::uint32_t const n_ranks = 1, std::uint32_t const rank = 0)
				: Reader(_open_mmio_file(path), n_ranks, rank, std::move(path))
		{
		}

		Reader(FILE* file, std::uint32_t const n_ranks, std::uint32_t const rank, std::string_view path)
				: Reader(file, n_ranks, rank, std::move(path), _process_mmio_header(file, path))
		{
		}

		auto _parse_tuple(char const* i, char const* e) -> std::optional<Tuple> {
			return parse_tuple(i, e);
		}

	  private:		
		static auto _open_mmio_file(std::string_view path) -> FILE*;

		struct _mmio_header_data {
			long bytes;
			int n;
			int m;
			int nnz;
		};
		
		static auto _process_mmio_header(FILE* file, std::string_view path) -> _mmio_header_data;

		Reader(FILE* file, std::uint32_t const n_ranks, std::uint32_t const rank, std::string_view path, _mmio_header_data mmio)
				: FileReader(file, n_ranks, rank, std::move(path), mmio.bytes)
				, _n(mmio.n)
				, _m(mmio.m)
				, _nnz(mmio.nnz)
		{
		}
	};
}
