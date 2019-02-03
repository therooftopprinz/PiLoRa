#include <IHwApi.hpp>
#include <Logger.hpp>
#include <sstream>
#include <iomanip>
#include <cstring>

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
private:
};

std::shared_ptr<ISpi>  getSpi(uint8_t channel);
std::shared_ptr<IGpio> getGpio();
void setup();
void teardown();
} // hwapi