#include <IHwApiMock.hpp>

namespace hwapi
{
std::shared_ptr<ISpi>  getSpi(uint8_t channel);
std::shared_ptr<IGpio> getGpio();
void setupHwapi();
void teardownHwapi();
} // hwapi