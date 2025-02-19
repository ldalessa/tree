#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <format>

namespace ingest
{
    inline
    namespace utils
    {
        /// Restructure the passed id.
        ///
        /// Swizzling reorders bytes in the passed value in order to take its
        /// least-significant bytes and move them to the most significant
        /// bytes. We do this because our address space is designed to do a PGAS
        /// distribution by default, and that benefits from high-order-bit
        /// entropy. The ids we get tend to be 0s in their high order bits, so
        /// this avoids a bunch of wasted tcam entries in the common case.
        ///
        /// @param x The input id.
        ///
        /// @returns The swizzled id.
        inline constexpr auto swizzle(std::uint64_t x) -> std::uint64_t {
            return std::byteswap(x);
        }

        /// Skip over a number of characters in a string.
        ///
        /// This is used to skip over a number of characters in a string,
        /// corresponding to another string. It will update the passed
        /// pointer. It ignores the `\0` in the string to skip. It is most
        /// easily understood by example.
        ///
        /// ```
        ///    s = ",,,foo"
        ///    t = skip(s, ",,,")
        ///    assert(s == t and t[0] == 'f')
        /// ```
        ///
        /// This is used by all of our parsers to move through well-understood
        /// csv records.
        ///
        /// **It does not do any pattern matching or error checking.**
        ///
        /// @param[in/out] i The string we are skipping through.
        /// @param[in] The string to skip.
        ///
        /// @returns i + N - 1
        template <std::size_t N>
        inline constexpr auto skip(char const*& i, char const (&)[N]) -> char const*& {
            i += N - 1;
            return i;
        }

        /// Parse an id.
        ///
        /// This will return a swizzled id, and advance `i`. It does not check
        /// for errors.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A swizzled id.
        inline auto parse_id(char const*& i, char const* e) -> std::uint64_t
        {
            assert(i[0] != ',');
            std::uint64_t id{};
            i = std::from_chars(i, e, id).ptr;
            return swizzle(id);
        }

        /// Parse an id.
        ///
        /// Some of our csvs have fields that may be ids, or might just be
        /// empty. This returns a swizzled id if one exists, or 0 if no id
        /// exists. It will update `i` during parsing.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A swizzled id, or 0 if there was no id.
        inline auto maybe_parse_id(char const*& i, char const* e) -> std::uint64_t
        {
            // if the id is empty return 0
            if (i[0] == ',') {
                return 0;
            }

            return parse_id(i, e);
        }

        /// Parse double value.
        ///
        /// This will advance the string pointer and does not check for
        /// errors. The resulting value is `std::bit_cast<uint64>` in order to
        /// be stored in a Tuple field.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A double bitcast into a uint64.
        inline auto parse_double(char const*& i, char const* e) -> std::uint64_t
        {
            assert(i[0] != ',');
            double d{};
            i = std::from_chars(i, e, d).ptr;
            return std::bit_cast<std::uint64_t>(d);
        }

        /// Parse double value.
        ///
        /// This will check if the field is missing, and return 0 if it is,
        /// otherwise this will advance the string pointer and does not check for
        /// errors. The resulting value is `std::bit_cast<uint64>` in order to
        /// be stored in a Tuple field.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A double bitcast into a uint64, or 0.
        inline auto maybe_parse_double(char const*& i, char const* e) -> std::uint64_t
        {
            // if the field is empty return 0
            if (i[0] == ',') {
                return 0;
            }

            return parse_double(i, e);
        }


        /// Parse integer value.
        ///
        /// This will advance the string pointer and does not check for
        /// errors. The resulting value is `uint64` in order to
        /// be stored in a Tuple field.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A uint64_t.
        inline auto parse_int(char const*& i, char const* e) -> std::uint64_t
        {
            assert(i[0] != ',');
            uint64_t d{};
            i = std::from_chars(i, e, d).ptr;
            return d;
        }

        /// Parse integer value.
        ///
        /// This will check if the field is missing, and return 0 if it is,
        /// otherwise this will advance the string pointer and does not check for
        /// errors. The resulting value is `uint64` in order to
        /// be stored in a Tuple field.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A uint64, or 0.
        inline auto maybe_parse_int(char const*& i, char const* e) -> std::uint64_t
        {
            // if the field is empty return 0
            if (i[0] == ',') {
                return 0;
            }

            return parse_int(i, e);
        }

        /// Parse a date.
        ///
        /// This parses a month/day/year date into a linearized uint64.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A uint64 that is a base-10 linearization of the date.
        inline auto parse_date(char const*& i, char const* e) -> std::uint64_t
        {
            assert(i[0] != ',');

            std::uint64_t year{};
            std::uint64_t month{};
            std::uint64_t day{};

            i = std::from_chars(i,e,month).ptr + 1; assert(i[-1] == '/');
            i = std::from_chars(i,e,day).ptr + 1;   assert(i[-1] == '/');
            i = std::from_chars(i,e,year).ptr;      assert(i[0] == ',');

            return year * 10000 + month * 100 + day;
        }

        /// Parse a date.
        ///
        /// This checks to see if a field is actually present, and if it is
        /// parses it as a month/day/year date into a linearized uint64.
        ///
        /// @param[in/out] i The string we are parsing, updated during the parse.
        /// @param[in] e A pointer to somewhere in the same string to serve as
        ///              an upper bound for parsing.
        ///
        /// @returns A uint64 that is a base-10 linearization of the date, or 0.
        inline auto maybe_parse_date(char const*& i, char const* e) -> std::uint64_t
        {
            // if the field is empty return 0
            if (i[0] == ',') {
                return 0;
            }

            return parse_date(i, e);
        }

        /// Print an id.
        ///
        /// This unswizzles and creates a std::string for an id.
        ///
        /// @param id The swizzled id.
        ///
        /// @returns A string.
        inline auto print_id(std::uint64_t const id) -> std::string
        {
            return std::format("{}", swizzle(id));
        }

        /// Print an id.
        ///
        /// This unswizzles and creates a std::string for an id.
        ///
        /// @param id The swizzled id.
        ///
        /// @returns A string.
        inline auto maybe_print_id(std::uint64_t const id) -> std::string
        {
            return (id) ? print_id(id) : "";
        }

        /// Print an integer.
        ///
        /// @param x The integer.
        ///
        /// @returns A string.
        inline auto print_int(std::uint64_t const x) -> std::string
        {
            return std::to_string(x);
        }

        /// Print an optional integer.
        ///
        /// @param x The integer.
        ///
        /// @returns A string.
        inline auto maybe_print_int(std::uint64_t const x) -> std::string
        {
            return (x) ? print_int(x) : "";
        }

        /// Print a floating point value back to the format used in the input
        /// files.
        ///
        /// This first converts the double from its uint64 representation and
        /// then uses std::format to format it as expected by the input csv.
        ///
        /// @param x The value.
        ///
        /// @returns A string.
        inline auto print_double(std::uint64_t const x) -> std::string
        {
            // The input files don't use a decimal for integers, so we have to
            // check for that condition.
            double const d = std::bit_cast<double>(x);
            double _;
            if (std::modf(d, &_)) {
                return std::format("{}", d);
            }
            else {
                return std::format("{:.1f}", d);
            }
        }

        /// Print a floating point value back to the format used in the input
        /// files.
        ///
        /// This first converts the double from its uint64 representation and
        /// then uses std::format to format it as expected by the input csv.
        ///
        /// @param x The value.
        ///
        /// @returns A string.
        inline auto maybe_print_double(std::uint64_t const id) -> std::string
        {
            return (id) ? print_double(id) : "";
        }

        /// Print a date.
        ///
        /// This deserializes the base-10 encoded date in order to print it as day/month/year.
        ///
        /// @param date The input date.
        ///
        /// @returns A string.
        inline auto print_date(std::uint64_t date) -> std::string
        {
            std::uint64_t month = date % 100;
            date /= 100;
            std::uint64_t day = date % 100;
            std::uint64_t year = date / 100;

            return std::format("{}/{}/{}", day, month, year);
        }

        /// Print a date.
        ///
        /// This deserializes the base-10 encoded date in order to print it as day/month/year.
        ///
        /// @param date The input date.
        ///
        /// @returns A string.
        inline auto maybe_print_date(std::uint64_t const date) -> std::string
        {
            return (date) ? print_date(date) : "";
        }
    }
}
