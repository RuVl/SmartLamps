#include "hal/Button.h"

namespace hal
{
    namespace
    {
        class SimButton final : public Button
        {
        public:
            void begin() override {}

            Gesture poll() override
            {
                return Gesture::None;
            }
        };
    }

    Button& button()
    {
        static SimButton instance;
        return instance;
    }
}
