#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <memory>
#include <thread>
#include <chrono>
#include <IHwApi.hpp>
#include <Logger.hpp>
#include <SX127x.hpp>

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

struct GpioStub : IGpio
{
public:
    int setMode(unsigned pGpio, PinMode pMode)
    {
        mLogger << logger::DEBUG << " setMode(" << pGpio << ", " << (PinMode::OUTPUT==pMode? "OUTPUT" : "INPUT") << ")" ;
        return 0;
    }

    int get(unsigned pGpio)
    {
        mLogger << logger::DEBUG << " get(" << pGpio << ")";
        return 0;
    }

    int set(unsigned pGpio, unsigned pLevel)
    {
        mLogger << logger::DEBUG << " set(" << pGpio << ", " << pLevel << ")";
        return 0;
    }

    int registerCallback(unsigned pUserGpio, Edge pEdge, std::function<void(uint32_t tick)> pCb)
    {
        cb = pCb;
        mLogger << logger::DEBUG << " registerCallback(" << pUserGpio << ", " << (Edge::FALLING==pEdge? "FALLING" : "RISING") << ", cb)" ;
        return 0;
    }

    int deregisterCallback(int pCallbackId)
    {
        mLogger << logger::DEBUG << " deregisterCallback(" << pCallbackId << ")" ;
        return 0;
    }

    std::function<void(uint32_t tick)> cb;
private:
    logger::Logger mLogger = logger::Logger("Gpio");
};

std::shared_ptr<ISpi> getSpi(uint8_t pChannel);
std::shared_ptr<IGpio> getGpio();

class Sx1278SpiStub : public ISpi
{
public:
    Sx1278SpiStub(int pChannel)
        : mChannel(pChannel)
        , mLogger("Sx1278SpiStub")
    {
        mLogger << logger::DEBUG << "channel: " << mChannel;
    }

    int read(uint8_t *, unsigned)
    {
        throw std::runtime_error("Sx1278SpiStub::read unsupported");
    }
    int write(uint8_t *, unsigned)
    {
        throw std::runtime_error("Sx1278SpiStub::write unsupported");
    }
    int xfer(uint8_t *pDataOut, uint8_t *pDataIn, unsigned pCount)
    {
        mLogger << logger::DEBUG << "channel: " << mChannel << " xfer: " << toHexString(pDataOut, pCount);
        bool isWrite = 0x80&pDataOut[0];
        uint8_t reg = 0x7F&pDataOut[0];
        std::memcpy(pDataIn+1, pDataOut+1, pCount-1);

        if (isWrite)
        {
            regwrite(reg, pDataOut, pCount);
        }

        return pCount;
    }
private:

    void regwrite(uint8_t pReg, uint8_t *pDataOut, unsigned pCount)
    {
        uint regVal = pDataOut[1];
        switch (pReg)
        {
            case flylora_sx127x::REGOPMODE:
            {
                mLogger << logger::DEBUG << "REGOPMODE=" << regVal;
                auto mode = flylora_sx127x::getUnmasked(flylora_sx127x::MODEMASK, regVal);
                if (mode == unsigned(flylora_sx127x::Mode::TX))
                {
                    std::thread([this]{
                        using namespace std::literals::chrono_literals;
                        std::this_thread::sleep_for(1ms);
                        mLogger << logger::DEBUG << "CALLING CB";
                        std::static_pointer_cast<GpioStub>(getGpio())->cb(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
                    }).detach();
                }
            }
            default:
            mLogger << logger::WARNING << "Uknown register " << unsigned(pReg);
        }; 
    }
    uint mRegs[128];
    uint mFifo[128];
    int mChannel;
    logger::Logger mLogger;
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