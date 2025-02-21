#ifndef require
#include <source_location>
#include "tree/error.hpp"
#define require(...) if (not (__VA_ARGS__)) throw tree::error("{} failed [{}:{}]\n", #__VA_ARGS__, std::source_location::current().file_name(), std::source_location::current().line())
#endif
