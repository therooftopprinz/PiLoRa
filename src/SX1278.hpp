#ifndef __SX1278_HPP__
#define __SX1278_HPP__

#include <thread>
#include <SX127x.hpp>
#include <IHwApi.hpp>
#include <condition_variable>
#include <atomic>
#include <cstring>
#include <deque>
#include <Buffer.hpp>
#include <Logger.hpp>

namespace flylora_sx127x
{

class SX1278
{
public:
    enum class Usage {UNSPEC, TX , RXC};

    SX1278(hwapi::ISpi& pSpi, hwapi::IGpio& pGpio, unsigned pResetPin, unsigned pDio1Pin)
        : mResetPin(pResetPin)
        , mDio1Pin(pDio1Pin)
        , mSpi(pSpi)
        , mGpio(pGpio)
    {
        mGpio.setMode(pResetPin, hwapi::PinMode::OUTPUT);
        mGpio.setMode(pDio1Pin,  hwapi::PinMode::INPUT);
        mGpio.set(mResetPin, 0);
        mDio1CbId = mGpio.registerCallback(mDio1Pin, hwapi::Edge::RISING, [this](uint32_t){onDio1();});
        init();
    }

    ~SX1278()
    {
        mGpio.deregisterCallback(mDio1CbId);
        bufferQueueCv.notify_one();
    }

    void resetModule()
    {
        // 7.2.2. Manual Reset - SX1276/77/78/79 DATASHEET
        using namespace std::chrono_literals;
        mGpio.set(mResetPin, 1);
        std::this_thread::sleep_for(100us);
        mGpio.set(mResetPin, 0);
        std::this_thread::sleep_for(5ms);
        init();
    }

    void setUsage(Usage pUsage)
    {
        mUsage = pUsage;
        // 4.1.6.1.  Digital IO Pin Mapping - SX1276/77/78/79 DATASHEET
        DioMapping1 dioMapping = DioMapping1::CadDone_FhssChangeChannel_RxTimeout_RxDone;
        if (Usage::TX == mUsage)
        {
            dioMapping = DioMapping1::ValidHeader_FhssChangeChannel_FhssChangeChannel_TxDone;
        }
        setRegister(REGDIOMAPPING1, uint8_t(dioMapping));
    }

    void setCarrier(uint32_t pCf)
    {
        // 4.1.4.  Frequency Settings - SX1276/77/78/79 DATASHEET
        // 6.4.    LoRa Mode Register Map - SX1276/77/78/79 DATASHEET
        // TODO: DO SPURRIOUS OPTIMIZATION - SX1276/77/78 Errata fixes
        // TODO: DO DetectionOptimize - SX1276/77/78 Errata fixes
        pCf = (pCf*524288ul)/mFosc;

        setRegister(REGFRLSB, pCf&0xFF);
        setRegister(REGFRMID, (pCf>>8)&0xFF);
        setRegister(REGFRMSB, (pCf>>16)&0xFF);
    }

    uint32_t getCarrier()
    {
        // 4.1.4.  Frequency Settings - SX1276/77/78/79 DATASHEET
        // 6.4.    LoRa Mode Register Map - SX1276/77/78/79 DATASHEET
        uint32_t cf = 0;
        cf |= getRegister(REGFRLSB);
        cf |= getRegister(REGFRMID)<<8;
        cf |= getRegister(REGFRMSB)<<16;
        return (mFosc*cf)/524288;
    }

    void configureModem(Bw pBandwidth, CodingRate pCodingRate, bool implicitHeader, SpreadingFactor pSpreadingFactor)
    {
        // 4.1.1. Link Design Using the LoRa Modem - SX1276/77/78/79 DATASHEET
        // 6.4.   LoRa Mode Register Map - SX1276/77/78/79 DATASHEET
        if (SpreadingFactor::SF_6 == pSpreadingFactor)
        {
            implicitHeader = true;
        }

        uint8_t config1 = setMasked(BWMASK, uint8_t(pBandwidth))
                        | setMasked(CODINGRATEMASK, uint8_t(pCodingRate))
                        | setMasked(IMPLICITHEADERMODEONMASK, implicitHeader);
        uint8_t config2 = setMasked(SPREADINGFACTORMASK, uint8_t(pSpreadingFactor));
        uint8_t config3 = (uint8_t(pSpreadingFactor) >= uint8_t(SpreadingFactor::SF_11) ? LOWDATARATEOPTIMIZEMASK : 0); // DEFAULT LNA GAIN IS G1

        setRegister(REGMODEMCONFIG1, config1);
        setRegister(REGMODEMCONFIG2, config2);
        setRegister(REGMODEMCONFIG3, config3);
    }

    void setOutputPower(int8_t pPower)
    {
        // 5.4.2. RF Power Amplifiers - SX1276/77/78/79 DATASHEET
        bool isPaBoost = false;
        uint8_t power = pPower;
        if (pPower>14)
        {
            isPaBoost = true;
            power = pPower-2;
        }

        uint8_t paConfig = uint8_t(
                    setMasked(PASELECTMASK, isPaBoost) |
                    setMasked(MAXPOWERMASK, 7) |
                    setMasked(OUTPUTPOWERMASK, power));

        setRegister(REGPACONFIG, paConfig);
    }

    void standby()
    {
        setMode(Mode::STDBY);
    }

    void start()
    {
        if (Usage::RXC == mUsage)
        {
            setMode(Mode::RXCONTINUOUS);
            return;
        }
        standby();
    }

    int getLastRssi()
    {
        // 5.5.5.  RSSI and SNR in LoRa Mode - SX1276/77/78/79 DATASHEET
        return -164+getRegister(REGPKTRSSIVALUE);
    }

    int tx(const uint8_t *pData, uint8_t pSize)
    {
        if (Usage::TX != mUsage || pSize>=256)
        {
            return -1;
        }

        // 4.1.6.  LoRaTM Modem State Machine Sequences - SX1276/77/78/79 DATASHEET
        // TODO: ANNOTATE SPECS
        setRegister(REGPAYLOADLENGTH, pSize);
        setRegister(REGFIFOADDRPTR, 0);

        uint8_t wro[257];
        uint8_t wri[257];

        wro[0] = 0x80|REGFIFO;
        std::memcpy(wro+1, pData, pSize);
        mSpi.xfer(wro, wri, 1+pSize);

        setMode(Mode::TX);
        {
            using namespace std::chrono_literals;
            std::unique_lock<std::mutex> lock(pTxDoneMutex);
            // TODO: Configurable TX TIMEOUT
            pTxDoneCv.wait_for(lock, 1s, [this]{return pTxDone;});
            if (!pTxDone)
            {
                mLogger << logger::ERROR << "tx timeout";
                return -1;
            }
            pTxDone = false;
        }
        return pSize;
    }

    common::Buffer rx()
    {
        std::unique_lock<std::mutex> lock(bufferQueueMutex);

        if (!bufferQueue.size())
        {
            // TODO: Configurable RX TIMEOUT
            using namespace std::chrono_literals;
            pTxDoneCv.wait_for(lock, 5s, [this]{return bufferQueue.size();});
        }

        if (bufferQueue.size())
        {
            mLogger << logger::ERROR << "rx timeout";
            return {};
        }

        common::Buffer rv = std::move(bufferQueue.front());
        bufferQueue.pop_front();
        return rv;
    }

private:

    void setRegister(uint8_t pReg, uint8_t val)
    {
        uint8_t wro[2] = {uint8_t(0x80|pReg), val};
        uint8_t wri[2];
        mSpi.xfer(wro, wri, 2);
    }

    uint8_t getRegister(uint8_t pReg)
    {
        uint8_t wro[2] = {pReg, 0};
        uint8_t wri[2];
        mSpi.xfer(wro, wri, 2);
        return wri[1];
    }

    uint8_t getMode()
    {
        return getUnmasked(MODEMASK, getRegister(REGOPMODE));
    }

    void setMode(Mode mode)
    {
        setRegister(REGOPMODE, LONGRANGEMODEMASK | LOWFREQUENCYMODEONMASK | setMasked(MODEMASK, uint8_t(mode)));
    }

    void init()
    {
        mUsage = Usage::UNSPEC;
        standby();
        // 4.1.2.4.  Interrupts in LoRa Mode - SX1276/77/78/79 DATASHEET
        uint8_t interruptMask = TXDONEMASKMASK | RXDONEMASKMASK;
        setRegister(REGIRQFLAGSMASK, interruptMask);
        // 4.1.2.3.  LoRa Mode FIFO Data Buffer - SX1276/77/78/79 DATASHEET
        // 4.1.6.    LoRa Modem State Machine Sequences - SX1276/77/78/79 DATASHEET
        setRegister(REGFIFOTXBASEADD, 0);
        setRegister(REGFIFORXBASEADD, 0);
    }

    void onDio1()
    {
        if (Usage::RXC == mUsage)
        {
            // TODO: ANNOTATE SPECS
            setRegister(REGFIFOADDRPTR, 0);
            // TODO: GetFifoData(RxByte+RxSize) and push to buffer
            uint8_t rcvSz = getRegister(REGRXNBBYTES) + getRegister(REGFIFORXBYTEADDR);
            uint8_t wro[257];
            uint8_t wri[257];
            wro[1] = REGFIFO;
            mSpi.xfer(wro, wri, 1+rcvSz);
            common::Buffer pvect(new std::byte[rcvSz], size_t(rcvSz));
            std::memcpy(pvect.data(), wri+1, rcvSz);

            {
                std::unique_lock<std::mutex> lock(bufferQueueMutex);
                bufferQueue.push_back(std::move(pvect));
            }

            bufferQueueCv.notify_one();
        }
        else
        {
            {
                std::unique_lock<std::mutex> lock(pTxDoneMutex);
                pTxDone = true;
            }
            pTxDoneCv.notify_one();
        }
    }

    std::condition_variable bufferQueueCv;
    std::mutex bufferQueueMutex;
    std::deque<common::Buffer> bufferQueue;

    std::condition_variable pTxDoneCv{};
    std::mutex pTxDoneMutex;
    bool pTxDone{};

    uint32_t mFosc = 32000000ul;
    unsigned mResetPin{};
    unsigned mDio1Pin{};
    int mDio1CbId{};
    Usage mUsage{};

    hwapi::ISpi& mSpi;
    hwapi::IGpio& mGpio;

    logger::Logger mLogger = logger::Logger("SX1278");
};

} // flylora_sx127x

#endif // __SX1278_HPP__