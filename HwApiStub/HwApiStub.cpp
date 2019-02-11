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
#include <Udp.hpp>

namespace hwapi
{

// TODO: put somewhere
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
        , mLogger(std::string("Sx1278SpiStub[")+std::to_string(pChannel)+"]")
    {
        mSocket.bind(net::toIpPort(127,0,0,1, 8888));
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
        // mLogger << logger::DEBUG << " xfer[" << pCount << "]: " << toHexString(pDataOut, pCount);
        bool isWrite = 0x80&pDataOut[0];
        uint8_t reg = 0x7F&pDataOut[0];
        std::memcpy(pDataIn+1, pDataOut+1, pCount-1);

        if (isWrite)
        {
            regwrite(reg, pDataOut, pDataIn, pCount);
        }
        else
        {
            regread(reg, pDataOut, pDataIn, pCount);
        }

        return pCount;
    }
private:

    void loraRx()
    {
        timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0; // 10 ms
        mSocket.setsockopt(SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(timeval));
        mLoRaRxActive = true;
        while (mLoRaRxActive)
        {
            auto fifoRxBase = getValue(flylora_sx127x::REGFIFORXBASEADD);
            auto fifoRxCurrent = getValue(flylora_sx127x::REGFIFORXCURRENTADDR);
            size_t maxsize = 256-(fifoRxBase+fifoRxCurrent);
            common::Buffer data((std::byte*)(mFifo + fifoRxBase + fifoRxCurrent), maxsize, false);
            net::IpPort src;
            auto rc = mSocket.recvfrom(data, src);
            if (rc > 0)
            {
                mLogger << logger::DEBUG << "LORA RX[" << rc << "]";
                setValue(flylora_sx127x::REGRXNBBYTES, rc);
                setValue(flylora_sx127x::REGFIFORXCURRENTADDR, fifoRxCurrent+rc);
                std::static_pointer_cast<GpioStub>(getGpio())->cb(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
        }
    }

    void regwrite(uint8_t pReg, uint8_t *pDataOut, uint8_t *, unsigned pCount)
    {
        // TODO: DATAIN VALIDATION
        uint regVal = pDataOut[1];
        auto oldValue = getValue(pReg);
        setValue(pReg, regVal);
        switch (pReg)
        {
            case flylora_sx127x::REGFIFO:
            {
                auto fifoIdx = getValue(flylora_sx127x::REGFIFOADDRPTR);
                mLogger << logger::DEBUG << "FIFO WRITE[" << pCount << "]: " << toHexString(pDataOut+1, pCount-1);
                if (fifoIdx+pCount > 256)
                {
                    throw std::runtime_error("FIFO OVERRUN!!");
                }
                std::memcpy(mFifo+fifoIdx, pDataOut+1, pCount-1);
                break;
            }
            case flylora_sx127x::REGOPMODE:
            {
                auto mode = getValue<flylora_sx127x::Mode>(pReg, flylora_sx127x::MODEMASK);
                auto oldMode = static_cast<flylora_sx127x::Mode>(flylora_sx127x::getUnmasked(flylora_sx127x::MODEMASK, oldValue));

                if (oldMode != mode && flylora_sx127x::Mode::RXCONTINUOUS == oldMode)
                {
                    mLoRaRxActive = false;
                    if (mLoRaRxThread.joinable())
                    {
                        mLoRaRxThread.join();
                    }
                }

                if (flylora_sx127x::Mode::TX == mode)
                {
                    mLogger << logger::DEBUG << "----- TRANSMITTING -----";
                    auto fifoTxBase = getValue(flylora_sx127x::REGFIFOTXBASEADD);
                    // TODO: TX LEN FOR IMPLICIT HEADER MODE
                    auto txLen = getValue(flylora_sx127x::REGPAYLOADLENGTH);
                    common::Buffer data((std::byte*)(mFifo+fifoTxBase), txLen, false);
                    mSocket.sendto(data, net::toIpPort(127,0,0,1,8889));

                    std::static_pointer_cast<GpioStub>(getGpio())->cb(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
                    setValue(pReg, flylora_sx127x::Mode::STDBY, flylora_sx127x::MODEMASK);
                    mLogger << logger::DEBUG << "------- TX DONE --------";
                }
                else if (flylora_sx127x::Mode::RXCONTINUOUS == mode)
                {
                    mLoRaRxThread = std::thread(&Sx1278SpiStub::loraRx, this);
                }

                break;
            }
        }; 
    }

    void regread(uint8_t pReg, uint8_t *pDataOut, uint8_t *pDataIn, unsigned pCount)
    {
        switch (pReg)
        {
            case flylora_sx127x::REGFIFO:
            {
                auto fifoIdx = getValue(flylora_sx127x::REGFIFOADDRPTR);
                if (fifoIdx+pCount > 256)
                {
                    throw std::runtime_error("FIFO OVERRUN!!");
                }
                std::memcpy(pDataOut+1, mFifo+fifoIdx, pCount-1);
                std::memcpy(pDataIn+1, mFifo+fifoIdx, pCount-1);
                mLogger << logger::DEBUG << "FIFO READ[" << pCount << "]: " << toHexString(pDataOut+1, pCount-1);
                break;
            }
            default:
            {
                pDataIn[1] = getValue(pReg);
            }

        }; 
    }

    template <typename R>
    R getValue(uint8_t pReg, uint8_t pMask)
    {
        return static_cast<R>(flylora_sx127x::getUnmasked(pMask, mRegs[pReg]));
    }

    uint8_t getValue(uint8_t pReg)
    {
        // std::stringstream ss;
        // ss << "READ  " << std::setw(24) << std::left << flylora_sx127x::regIndexToString(pReg) << " = 0x" <<
            // std::hex << std::setw(2) << std::right << std::setfill('0') << unsigned(mRegs[pReg]);
        // TODO: IMPROVE LOGGER
        // mLogger << logger::DEBUG << ss.str();
        return mRegs[pReg];
    }

    template <typename R>
    void setValue(uint8_t pReg, R pValue, uint8_t pMask)
    {
        setValue(pReg, (mRegs[pReg]&(~pMask)) | flylora_sx127x::setMasked(pMask, uint64_t(pValue)));
    }

    void setValue(uint8_t pReg, uint8_t pValue)
    {
        std::stringstream ss;
        ss << "WRITE " << std::setw(24) << std::left << flylora_sx127x::regIndexToString(pReg) << " = 0x" <<
            std::hex << std::setw(2) << std::right << std::setfill('0') << unsigned(pValue);
        // TODO: IMPROVE LOGGER
        mLogger << logger::DEBUG << ss.str();
        mRegs[pReg] = pValue;
    }

    uint8_t mRegs[128];
    uint8_t mFifo[256];
    int mChannel;
    std::thread mLoRaRxThread;
    bool mLoRaRxActive;
    net::UdpSocket mSocket;
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