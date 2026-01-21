#include "sparrow/debug/copy_tracker.hpp"

#include <map>
#include <ranges>
#include <string>

namespace sparrow::copy_tracker
{
#ifdef SPARROW_TRACK_COPIES

    class copy_tracker_impl
    {
    public:

        static copy_tracker_impl& instance()
        {
            static copy_tracker_impl inst;
            return inst;
        }

        void increase(const std::string& key)
        {
            m_copy_count[key] += 1;
        }

        void reset(const std::string& key)
        {
            m_copy_count[key] = 0;
        }

        void reset_all()
        {
            m_copy_count.clear();
        }

        int count(const std::string& key)
        {
            return m_copy_count[key];
        }

        auto key_list()
        {
            return std::views::keys(m_copy_count);
        }

    private:

        copy_tracker_impl() = default;

        std::map<std::string, int> m_copy_count = {};
    };

    void increase(const std::string& key)
    {
        return copy_tracker_impl::instance().increase(key);
    }

    void reset(const std::string& key)
    {
        return copy_tracker_impl::instance().reset(key);
    }

    void reset_all()
    {
        return copy_tracker_impl::instance().reset_all();
    }

    int count(const std::string& key, int /*disabled_value*/)
    {
        return copy_tracker_impl::instance().count(key);
    }

    std::vector<std::string> key_list()
    {
        auto keys = copy_tracker_impl::instance().key_list();
        return {keys.begin(), keys.end()};
    }

#else

    void increase(const std::string&)
    {
    }

    void reset(const std::string&)
    {
    }

    void reset_all()
    {
    }

    int count(const std::string&, int disabled_value)
    {
        return disabled_value;
    }

    std::vector<std::string> key_list()
    {
        return {};
    }

#endif

}
