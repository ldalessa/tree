#include "die/coffee.hpp"
#include "die/commercial.hpp"
#include "die/cyber.hpp"
#include "die/social.hpp"
#include "die/uses.hpp"
#include <exception>

auto
die::coffee::parse_tuple(char const* i, char const* e, FileType type)
    -> std::optional<Tuple>
{
    switch (type) {
      case FileType::COMMERCIAL: return commercial::parse_tuple(i, e);
      case FileType::CYBER: return cyber::parse_tuple(i, e);
      case FileType::SOCIAL: return social::parse_tuple(i, e);
      case FileType::USES: return uses::parse_tuple(i, e);
      case FileType::NONE:
      default:
        throw std::runtime_error("Coffee FileReader has no file type configured");
    }
}

auto
die::coffee::parse_tuple(std::string_view str, FileType type)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end(), type);
}
