#include <string>
#include <unordered_map>

#include "hal/Storage.h"

namespace hal
{
    namespace
    {
        class SimStorage final : public Storage
        {
        public:
            void begin() override
            {
            }

            void tick() override
            {
            }

            void flush() override
            {
            }

            int32_t getInt(uint32_t key, int32_t fallback) override
            {
                const auto it = ints_.find(key);
                return it != ints_.end() ? it->second : fallback;
            }

            void setInt(uint32_t key, int32_t value) override
            {
                ints_[key] = value;
            }

            void initInt(uint32_t key, int32_t value) override
            {
                if (!ints_.count(key)) ints_[key] = value;
            }

            const char* getString(uint32_t key, const char* fallback) override
            {
                const auto it = strings_.find(key);
                return it != strings_.end() ? it->second.c_str() : fallback;
            }

            void setString(uint32_t key, const char* value) override
            {
                strings_[key] = value;
            }

        private:
            std::unordered_map<uint32_t, uint32_t> ints_;
            std::unordered_map<uint32_t, std::string> strings_;
        };
    }

    Storage& storage()
    {
        static SimStorage instance;
        return instance;
    }

    // database is needed only for net
}
