#pragma once

#include <string>
#include <vector>

#include "sparrow/config/config.hpp"

namespace sparrow::copy_tracker
{
    template <class T>
    std::string key();

    constexpr bool is_enabled()
    {
#ifdef SPARROW_TRACK_COPIES
        return true;
#else
        return false;
#endif
    }

    SPARROW_API void increase(const std::string& key);
    SPARROW_API void reset(const std::string& key);
    SPARROW_API void reset_all();
    SPARROW_API int count(const std::string& key, int disabled_value = 0);
    SPARROW_API std::vector<std::string> key_list();

}
