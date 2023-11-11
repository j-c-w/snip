#include "includes.hpp"

std::set<std::string> IncludeCollector::includes;

// TODO -- find a better way of doing this?
std::set<std::string> IncludeCollector::filteredIncludes = {
    "features-time64.h",
    "features.h",
    "stdc-predef.h",
    "bits/floatn-common.h",
    "bits/floatn.h",
    "bits/libc-header-start.h",
    "bits/long-double.h",
    "bits/stdio_lim.h",
    "bits/time64.h",
    "bits/timesize.h",
    "bits/types.h",
    "bits/types/FILE.h",
    "bits/types/__FILE.h",
    "bits/types/__fpos64_t.h",
    "bits/types/__fpos_t.h",
    "bits/types/__mbstate_t.h",
    "bits/types/struct_FILE.h",
    "bits/typesizes.h",
    "bits/wordsize.h",
    "gnu/stubs-64.h",
    "gnu/stubs.h",
    "sys/cdefs.h"
};
