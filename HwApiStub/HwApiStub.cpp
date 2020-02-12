#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <memory>
#include <thread>
#include <chrono>
#include <PiGpioHwApi/HwApi.hpp>
#include <Logger.hpp>
#include <SX127x.hpp>
#include <bfc/Udp.hpp>

namespace hwapi
{

// TODO: put somewhere
inline std::string toHexString(const uint8_t* pData, size_t size)
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
        Logless("DBG GpioStub::setMode setMode(_, _)", pGpio, (PinMode::OUTPUT==pMode? "OUTPUT" : "INPUT"));
        return 0;
    }

    int get(unsigned pGpio)
    {
        Logless("DBG GpioStub::get get(_)", pGpio);
        return 0;
    }

    int set(unsigned pGpio, unsigned pLevel)
    {
        Logless("DBG GpioStub::set set(_, _)", pGpio, pLevel);
        return 0;
    }

    int registerCallback(unsigned pUserGpio, Edge pEdge, std::function<void(uint32_t tick)> pCb)
    {
        cb = pCb;
        Logless("DBG GpioStub::registerCallback registerCallback(_, _)", pUserGpio, (Edge::FALLING==pEdge? "FALLING" : "RISING"));
        return 0;
    }

    int deregisterCallback(int pCallbackId)
    {
        Logless("DBG GpioStub::deregisterCallback deregisterCallback(_)", pCallbackId); 
        return 0;
    }

    std::function<void(uint32_t tick)> cb;
private:
};

std::shared_ptr<ISpi> getSpi(uint8_t pChannel);
std::shared_ptr<IGpio> getGpio();

class Sx1278SpiStub : public ISpi
{
public:
    Sx1278SpiStub(int pChannel)
        : mChannel(pChannel)
    {
        mSocket.bind(bfc::toIpPort(127,0,0,1, 8000));
        setValue(flylora_sx127x::REGVERSION, 0x12);
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
        Logless("DBG Sx1278SpiStub::xfer xfer[_]: _", pCount, BufferLog(pCount, pDataOut));
        bool isWrite = 0x80&pDataOut[0];
        uint8_t reg = 0x7F&pDataOut[0];

        // TODO: FOR WRITE OPERATION DATA IN IS OLD REG VALUE
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
        tv.tv_sec = 5; // 5s
        tv.tv_usec = 0;
        mSocket.setsockopt(SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(timeval));
        mLoRaRxActive = true;
        std::byte buffer[256];
        bfc::BufferView data(buffer, sizeof(buffer));
        while (mLoRaRxActive)
        {
            auto fifoRxTop = getValue(flylora_sx127x::REGFIFORXBYTEADDR);
            ssize_t maxsize = 256-fifoRxTop;
            bfc::IpPort src;
            auto rc = mSocket.recvfrom(data, src);
            Logless("DBG Sx1278SpiStub::loraRx LORA RX[_] (UDP IN)", rc);
            if (rc < 0)
            {
                Logless("ERR Sx1278SpiStub::loraRx ERRNO:_", (const char*)strerror(errno));
            }
            else if (rc > 0)
            {
                // TODO: DETERMINE IF THIS IS CORRECT BEHAVIOR
                if (rc >= maxsize)
                {
                    std::memcpy(mFifo + fifoRxTop, data.data(), maxsize);
                    if (size_t remSize = rc-maxsize)
                    {
                        std::memcpy(mFifo, data.data()+maxsize, remSize);
                    }
                }
                else
                {
                    std::memcpy(mFifo + fifoRxTop, data.data(), rc);
                }

                setValue(flylora_sx127x::REGRXNBBYTES, rc);
                setValue(flylora_sx127x::REGFIFORXCURRENTADDR, fifoRxTop);
                setValue(flylora_sx127x::REGFIFORXBYTEADDR, fifoRxTop+rc);
                std::static_pointer_cast<GpioStub>(getGpio())->cb(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
        }
    }

    void regwrite(uint8_t pReg, uint8_t *pDataOut, uint8_t *, unsigned pCount)
    {

        auto mode = static_cast<flylora_sx127x::Mode>(flylora_sx127x::getUnmasked(flylora_sx127x::MODEMASK, getValue(flylora_sx127x::REGOPMODE)));
        if (pReg!=flylora_sx127x::REGOPMODE &&
            !( mode== flylora_sx127x::Mode::STDBY || mode==flylora_sx127x::Mode::SLEEP))
        {
            // throw std::runtime_error("WRITE WHILE NOT IN SLEEP OR STANDBY MODE!!");
        }

        // TODO: DATAIN VALIDATION
        uint regVal = pDataOut[1];
        auto oldValue = getValue(pReg);
        setValue(pReg, regVal);
        switch (pReg)
        {
            case flylora_sx127x::REGFIFO:
            {
                auto fifoCount = pCount-1;
                auto fifoIdx = getValue(flylora_sx127x::REGFIFOADDRPTR);
                if (fifoIdx+pCount > 256)
                {
                    throw std::runtime_error("FIFO OVERRUN!!");
                }
                Logless("DBG Sx1278SpiStub::regwrite FIFO WRITE[_]: _", fifoCount, BufferLog(fifoCount, pDataOut+1));
                std::memcpy(mFifo+fifoIdx, pDataOut+1, fifoCount);
                setValue(flylora_sx127x::REGFIFOADDRPTR, fifoIdx+fifoCount);
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
                    Logless("DBG Sx1278SpiStub::regwrite ----- TRANSMITTING -----");
                    auto fifoTxBase = getValue(flylora_sx127x::REGFIFOTXBASEADD);
                    // TODO: TX LEN FOR IMPLICIT HEADER MODE
                    auto txLen = getValue(flylora_sx127x::REGPAYLOADLENGTH);
                    bfc::BufferView data((std::byte*)(mFifo+fifoTxBase), txLen);
                    mSocket.sendto(data, bfc::toIpPort(127,0,0,1,8001));

                    std::static_pointer_cast<GpioStub>(getGpio())->cb(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
                    setValue(pReg, flylora_sx127x::Mode::STDBY, flylora_sx127x::MODEMASK);
                    Logless("DBG Sx1278SpiStub::regwrite ----- TRANSMITTING -----");
                }
                else if (flylora_sx127x::Mode::RXCONTINUOUS == mode)
                {
                    // TODO: reset rx fifo regs
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
                auto fifoCount = pCount-1;
                auto fifoIdx = getValue(flylora_sx127x::REGFIFOADDRPTR);
                if (fifoIdx+fifoCount > 256)
                {
                    throw std::runtime_error("FIFO OVERRUN!!");
                }
                std::memcpy(pDataOut+1, mFifo+fifoIdx, fifoCount);
                std::memcpy(pDataIn+1, mFifo+fifoIdx, fifoCount);
                Logless("DBG Sx1278SpiStub::regread FIFO READ[_]@_: _", fifoCount, unsigned(fifoIdx), BufferLog(fifoCount, pDataOut+1));
                setValue(flylora_sx127x::REGFIFOADDRPTR, fifoIdx+fifoCount);
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
        Logless("DBG Sx1278SpiStub::getValue READ _=_", flylora_sx127x::regIndexToString(pReg), unsigned(mRegs[pReg]));
        return mRegs[pReg];
    }

    template <typename R>
    void setValue(uint8_t pReg, R pValue, uint8_t pMask)
    {
        setValue(pReg, (mRegs[pReg]&(~pMask)) | flylora_sx127x::setMasked(pMask, uint64_t(pValue)));
    }

    void setValue(uint8_t pReg, uint8_t pValue)
    {
        Logless("DBG Sx1278SpiStub::getValue READ _=_", flylora_sx127x::regIndexToString(pReg), unsigned(pValue));
        mRegs[pReg] = pValue;
    }

    uint8_t mRegs[128]{};
    uint8_t mFifo[256]{};
    int mChannel;
    std::thread mLoRaRxThread;
    bool mLoRaRxActive;
    bfc::UdpSocket mSocket;
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