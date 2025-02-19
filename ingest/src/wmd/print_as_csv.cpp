#include "die/Tuple.hpp"
#include "die/utils.hpp"
#include "die/wmd.hpp"

#include <exception>
#include <format>

using die::Tuple;
using die::utils::maybe_print_double;
using die::utils::maybe_print_id;
using die::utils::print_date;
using die::utils::print_id;

static auto _print_author_format_event(Tuple const& tuple) -> std::string
{
    return std::format("Author,{},,,{},,,,,", print_id(tuple.k), print_id(tuple.b));
}

static auto _print_author_publication(Tuple const& tuple) -> std::string
{
    return std::format("Author,{},,,,{},,,,", print_id(tuple.b), print_id(tuple.k));
}

static auto _print_forum(Tuple const& tuple) -> std::string
{
    return std::format("Forum,,,{},,,,,,", print_id(tuple.k));
}

static auto _print_forum_event(Tuple const& tuple) -> std::string
{
    return std::format("ForumEvent,,,{},{},,,{},,", maybe_print_id(tuple.b), print_id(tuple.k), print_date(tuple.c));
}

static auto _print_has_org(Tuple const& tuple) -> std::string
{
    return std::format("HasOrg,,,,,{},{},,,", print_id(tuple.k), print_id(tuple.b));
}

static auto _print_has_topic_forum(Tuple const& tuple) -> std::string
{
    return std::format("HasTopic,,,{},,,{},,,", print_id(tuple.k), print_id(tuple.b));
}

static auto _print_has_topic_forum_event(Tuple const& tuple) -> std::string
{
    return std::format("HasTopic,,,,{},,{},,,", print_id(tuple.k), print_id(tuple.b));
}

static auto _print_has_topic_public(Tuple const& tuple) -> std::string
{
    return std::format("HasTopic,,,,,{},{},,,", print_id(tuple.k), print_id(tuple.b));
}

static auto _print_includes(Tuple const& tuple) -> std::string
{
    return std::format("Includes,,,{},{},,,,,", print_id(tuple.k), print_id(tuple.b));
}

static auto _print_person(Tuple const& tuple) -> std::string
{
    return std::format("Person,{},,,,,,,,", print_id(tuple.k));
}

static auto _print_publication(Tuple const& tuple) -> std::string
{
    return std::format("Publication,,,,,{},,{},,", print_id(tuple.k), print_date(tuple.b));
}

static auto _print_sale(Tuple const& tuple) -> std::string
{
    return std::format("Sale,{},{},,,,{},{},,", print_id(tuple.k), print_id(tuple.b), maybe_print_id(tuple.c), print_date(tuple.d));
}

static auto _print_topic(Tuple const& tuple) -> std::string
{
    return std::format("Topic,,,,,,{},,{},{}", print_id(tuple.k), maybe_print_double(tuple.b), maybe_print_double(tuple.c));
}

auto
die::wmd::print_as_csv(Tuple const& tuple)
    -> std::string
{
    switch (auto type = die::wmd::type(tuple))
    {
      case _Author_ForumEvent: return _print_author_format_event(tuple);
      case _Author_Publication: return _print_author_publication(tuple);
      case _Forum: return _print_forum(tuple);
      case _ForumEvent: return _print_forum_event(tuple);
      case _HasOrg: return _print_has_org(tuple);
      case _HasTopic_Forum: return _print_has_topic_forum(tuple);
      case _HasTopic_ForumEvent: return _print_has_topic_forum_event(tuple);
      case _HasTopic_Publication: return _print_has_topic_public(tuple);
      case _Includes: return _print_includes(tuple);
      case _Person: return _print_person(tuple);
      case _Publication: return _print_publication(tuple);
      case _Sale: return _print_sale(tuple);
      case _Topic: return _print_topic(tuple);
      default:
        throw std::runtime_error(std::format("Unkown tuple type when printing as csv {}\n", type));
    }
}
