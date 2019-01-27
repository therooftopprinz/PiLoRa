#include <IHwApiMock.hpp>

namespace hwapi
{
std::shared_ptr<ISpi>  spiFactory(uint8_t channel);
std::shared_ptr<IGpio> gpioFactory();
void setupHwapi();
void teardownHwapi();
} // hwapi