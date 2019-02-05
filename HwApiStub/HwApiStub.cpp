#include <IHwApi.hpp>
#include <Logger.hpp>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <memory>

namespace hwapi
{

std::string toHexString(const uint8_t* pData, size_t size)
{
    std::stringstream ss;;
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    return ss.str();
}

class Sx1278SpiStub : public ISpi
{
public:
    Sx1278SpiStub(int pChannel)
        : mChannel(pChannel)
        , mLogger("Sx1278SpiStub")
    {
        mLogger << logger::DEBUG << "channel: " << mChannel;
    }

    int read(uint8_t *data, unsigned count)
    {
        throw std::runtime_error("Sx1278SpiStub::read unsupported");
    }
    int write(uint8_t *data, unsigned count)
    {
        throw std::runtime_error("Sx1278SpiStub::write unsupported");
    }
    int xfer(uint8_t *dataOut, uint8_t *dataIn, unsigned count)
    {
        std::memcpy(dataOut+1, dataIn+1, count-1);
        return count;
    }
private:
    uint mRegs[128];
    uint mFifo[128];
    int mChannel;
    logger::Logger mLogger;
};

struct GpioStub : IGpio
{
public:
    int setMode(unsigned pGpio, PinMode pMode)
    {
        return 0;
    }

    int get(unsigned pGpio)
    {
        return 0;
    }

    int set(unsigned pGpio, unsigned pLevel)
    {
        return 0;
    }

    int registerCallback(unsigned pUserGpio, Edge pEdge, std::function<void(uint32_t tick)> pCb)
    {
        return 0;
    }

    int deregisterCallback(int pCallbackId)
    {
        return 0;
    }

private:
};

std::shared_ptr<ISpi> getSpi(uint8_t pChannel)
{
    static std::map<int, std::shared_ptr<ISpi>> spiMap;
    auto it = spiMap.find(pChannel);
    if (spiMap.end() == it)
    {
        spiMap.emplace(pChannel, std::make_shared<Sx1278SpiStub>(pChannel));
    }

    return spiMap[pChannel];
}

std::shared_ptr<IGpio> getGpio()
{
    static std::shared_ptr<IGpio> gpio = std::make_shared<GpioStub>();
    return gpio;
}
void setup()
{
}

void teardown()
{
}

} // hwapi