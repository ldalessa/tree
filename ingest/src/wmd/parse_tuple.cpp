#include "die/utils.hpp"
#include "die/wmd.hpp"
#include <format>

using die::Tuple;
using namespace die::utils;
using namespace die::wmd::tuple_types;

static auto _parse_author(char const* str, char const* e) -> Tuple
{
    auto const id = parse_id(skip(str, ","), e);

    if (str[3] != ',') {
        return Tuple {
            .k = id,
            .type = _Author_ForumEvent,
            .b = parse_id(skip(str, ",,,"), e)
        };
    }
    assert(str[4] != ',');
    return Tuple {
        .k = parse_id(skip(str, ",,,,"), e),
        .type = _Author_Publication,
        .b = id
    };
}

static auto _parse_forum(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ",,,"), e),
        .type = _Forum
    };
}

static auto _parse_forum_event(char const* str, char const* e) -> Tuple
{
    auto const forum = maybe_parse_id(skip(str, ",,,"), e);
    auto const id = parse_id(skip(str, ","), e);

    return Tuple {
        .k = id,
        .type = _ForumEvent,
        .b = forum,
        .c = parse_date(skip(str, ",,,"), e)
    };
}

static auto _parse_has_org(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ",,,,,"), e),
        .type = _HasOrg,
        .b = parse_id(skip(str, ","), e)
    };
}

static auto _parse_has_topic(char const* str, char const* e) -> Tuple
{
    if (str[3] != ',') {
        return Tuple {
            .k = parse_id(skip(str, ",,,"), e),
            .type = _HasTopic_Forum,
            .b = parse_id(skip(str, ",,,"), e)
        };
    }
    if (str[4] != ',') {
        return Tuple {
            .k = parse_id(skip(str, ",,,,"), e),
            .type = _HasTopic_ForumEvent,
            .b = parse_id(skip(str, ",,"), e)
        };
    }
    assert(str[5] != ',');
    return Tuple {
        .k = parse_id(skip(str, ",,,,,"), e),
        .type = _HasTopic_Publication,
        .b = parse_id(skip(str, ","), e)
    };
}

static auto _parse_includes(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ",,,"), e),
        .type = _Includes,
        .b = parse_id(skip(str, ","), e)
    };
}

static auto _parse_person(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ","), e),
        .type = _Person
    };
}

static auto _parse_publication(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ",,,,,"), e),
        .type = _Publication,
        .b = parse_date(skip(str, ",,"), e)
    };
}

static auto _parse_sale(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ","), e),
        .type = _Sale,
        .b = parse_id(skip(str, ","), e),
        .c = maybe_parse_id(skip(str, ",,,,"), e),
        .d = parse_date(skip(str, ","), e)
    };
}

static auto _parse_topic(char const* str, char const* e) -> Tuple
{
    return Tuple {
        .k = parse_id(skip(str, ",,,,,,"), e),
        .type = _Topic,
        .b = maybe_parse_double(skip(str, ",,"), e),
        .c = maybe_parse_double(skip(str, ","), e)
    };
}

auto
die::wmd::parse_tuple(char const* str, char const* e)
    -> std::optional<Tuple>
{
    switch (str[0]) {
      case 'A': return _parse_author(skip(str, "Author"), e);
      case 'F':
        switch (str[5]) {
          case ',': return _parse_forum(skip(str,"Forum"), e);
          case 'E': return _parse_forum_event(skip(str, "ForumEvent"), e);
        }
        break;
      case 'H':
        switch (str[3]) {
          case 'O': return _parse_has_org(skip(str, "HasOrg"), e);
          case 'T': return _parse_has_topic(skip(str, "HasTopic"), e);
        }
        break;
      case 'I': return _parse_includes(skip(str, "Includes"), e);
      case 'P':
        switch (str[1]) {
          case 'e': return _parse_person(skip(str, "Person"), e);
          case 'u': return _parse_publication(skip(str, "Publication"), e);
        }
        break;
      case 'S': return _parse_sale(skip(str, "Sale"), e);
      case 'T': return _parse_topic(skip(str, "Topic"), e);
      case '#': return std::nullopt;
    }
    throw std::runtime_error(std::format("Unexpected wmd input string {}\n", str));
}

auto
die::wmd::parse_tuple(std::string_view str)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end());
}
