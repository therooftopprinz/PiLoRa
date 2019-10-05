#ifndef __SX1278_HPP__
#define __SX1278_HPP__

#include <thread>
#include <SX127x.hpp>
#include <IHwApi.hpp>
#include <condition_variable>
#include <atomic>
#include <cstring>
#include <deque>
#include <BFC/Buffer.hpp>
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
        mGpio.set(mResetPin, 1);
        mDio1CbId = mGpio.registerCallback(mDio1Pin, hwapi::Edge::RISING, [this](uint32_t){onDio1();});
        init();
    }

    ~SX1278()
    {
        mGpio.deregisterCallback(mDio1CbId);
        mTeardown = true;
        mRxTxDoneCv.notify_one();
    }

    void resetModule()
    {
        // 7.2.2. Manual Reset - SX1276/77/78/79 DATASHEET
        // TODO: There's something wrong with reset
        using namespace std::chrono_literals;
        mGpio.set(mResetPin, 0);
        std::this_thread::sleep_for(100us);
        mGpio.set(mResetPin, 1);
        std::this_thread::sleep_for(5ms);
        init();
    }

    void setUsage(Usage pUsage)
    {
        mUsage = pUsage;
        // 4.1.6.1.  Digital IO Pin Mapping - SX1276/77/78/79 DATASHEET
        if (Usage::TX == mUsage)
        {
            setRegister(REGDIOMAPPING1, DIO0TXDONEMASK);
        }
        else
        {
            setRegister(REGDIOMAPPING1, DIO0RXDONEMASK);
        }
    }

    void setCarrier(uint64_t pCf)
    {
        // 4.1.4.  Frequency Settings - SX1276/77/78/79 DATASHEET
        // 6.4.    LoRa Mode Register Map - SX1276/77/78/79 DATASHEET
        // TODO: DO SPURRIOUS OPTIMIZATION - SX1276/77/78 Errata fixes
        // TODO: DO DetectionOptimize - SX1276/77/78 Errata fixes
        pCf = (pCf*524288ul)/mFosc;

        mFrLsb = pCf&0xFF;
        mFrMid = (pCf>>8)&0xFF;
        mFrMsb = (pCf>>16)&0xFF;

        setRegister(REGFRLSB, mFrLsb);
        setRegister(REGFRMID, mFrMid);
        setRegister(REGFRMSB, mFrMsb);
    }

    uint32_t getCarrier()
    {
        // 4.1.4.  Frequency Settings - SX1276/77/78/79 DATASHEET
        // 6.4.    LoRa Mode Register Map - SX1276/77/78/79 DATASHEET
        uint64_t cf = 0;
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

        mModemConfig1 = config1;
        mModemConfig2 = config2;
        mModemConfig3 = config3;

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

        mPaConfig = paConfig;
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
            setRegister(REGFIFOADDRPTR, 0);
            setMode(Mode::RXCONTINUOUS);
            return;
        }
        standby();
    }

    int getLastSnr()
    {
        // 5.5.5.  RSSI and SNR in LoRa Mode - SX1276/77/78/79 DATASHEET
        return getRegister(REGPKTSNRVALUE);
    }

    int getLastRssi()
    {
        // 5.5.5.  RSSI and SNR in LoRa Mode - SX1276/77/78/79 DATASHEET
        return -164+getRegister(REGPKTRSSIVALUE);
    }

    int getCurrentRssi()
    {
        // 5.5.5.  RSSI and SNR in LoRa Mode - SX1276/77/78/79 DATASHEET
        return -164+getRegister(REGRSSIVALUE);
    }

    bool validate()
    {
        if (0x12 != getRegister(REGVERSION))
        {
            throw std::runtime_error("Invalid chip version!");
        }

        return mFrLsb == getRegister(REGFRLSB) &&
        mFrMid == getRegister(REGFRMID) &&
        mFrMsb == getRegister(REGFRMSB) &&
        mModemConfig1 == getRegister(REGMODEMCONFIG1) &&
        mModemConfig2 == getRegister(REGMODEMCONFIG2) &&
        mModemConfig3 == getRegister(REGMODEMCONFIG3) &&
        mPaConfig == getRegister(REGPACONFIG);
    }

    int tx(const uint8_t *pData, uint8_t pSize)
    {
        Logless("DBG SX1278::tx DBG ---------- tx start --------------");
        if (Usage::TX != mUsage || pSize>=256)
        {
            return -1;
        }

        setRegister(REGDIOMAPPING1, DIO0TXDONEMASK);

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
            std::unique_lock<std::mutex> lock(mTxDoneMutex);
            mTxDone = false;
            // TODO: Configurable TX TIMEOUT
            mRxTxDoneCv.wait(lock, [this](){return mTxDone||mTeardown;});

            if (!mTxDone)
            {
                Logless("ERR SX1278::tx DBG ---------- tx timeout --------------");
                Logger::getInstance().flush();
                return -1;
            }
        }
        Logless("DBG SX1278::tx ---------- tx done --------------");
        return pSize;
    }

    bfc::Buffer rx()
    {
        std::unique_lock<std::mutex> lock(bufferQueueMutex);

        if (!bufferQueue.size())
        {
            // TODO: Configurable RX TIMEOUT
            using namespace std::chrono_literals;
            mRxTxDoneCv.wait_for(lock, 10s, [this]{return bufferQueue.size()||mTeardown;});
        }

        if (!bufferQueue.size())
        {
            Logless("SX1278::rx ERR rx timeout");
            Logger::getInstance().flush();
            return {};
        }

        bfc::Buffer rv = std::move(bufferQueue.front());
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
        setRegister(REGOPMODE, 0); // Sleep Mode
        setRegister(REGOPMODE, LONGRANGEMODEMASK | LOWFREQUENCYMODEONMASK); // Set LoRa
        standby();
        // 4.1.2.3.  LoRa Mode FIFO Data Buffer - SX1276/77/78/79 DATASHEET
        // 4.1.6.    LoRa Modem State Machine Sequences - SX1276/77/78/79 DATASHEET
        setRegister(REGFIFOTXBASEADD, 0);
        setRegister(REGFIFORXBASEADD, 0);
        setRegister(REGFIFORXCURRENTADDR, 0);
    }

    void onDio1()
    {
        if (Usage::RXC == mUsage)
        {
            Logless("DBG SX1278::onDio1 RX DONE \\");
            // TODO: ANNOTATE SPECS
            // TODO: what value in implicit header
            uint8_t rcvSz = getRegister(REGRXNBBYTES);
            uint8_t currRx = getRegister(REGFIFORXCURRENTADDR);
            uint8_t rdBase = getRegister(REGFIFOADDRPTR);
            if (uint8_t(currRx+rcvSz) == rdBase)
            {
                Logless("ERR SX1278::onDio1 FALSE RX");
                return;
            }

            if (currRx>rdBase)
            {
                Logless("ERR SX1278::onDio1 FALSE RX");
                rcvSz += (currRx-rdBase);
            }
            else if (currRx<rdBase)
            {
                Logless("WRN SX1278::onDio1 CURRENTRX > READBASE! Mising Interrupt?!");
                rcvSz += (255-rdBase)+currRx;
            }

            bfc::Buffer pvect(new std::byte[rcvSz], size_t(rcvSz));
            ssize_t maxsize = 256-currRx;
            uint8_t wro[257];
            uint8_t wri[257];
            wro[0] = REGFIFO;

            if (rcvSz>=maxsize)
            {
                Logless("WRN SX1278::onDio1 FIFO AT: _", unsigned(rdBase));
                mSpi.xfer(wro, wri, 1+maxsize);
                std::memcpy(pvect.data(), wri+1, maxsize);
                if (size_t remSize = rcvSz-maxsize)
                {
                    Logless("WRN SX1278::onDio1 FIFO AT: _", unsigned(getRegister(REGFIFOADDRPTR)));
                    mSpi.xfer(wro, wri, 1+remSize);
                    std::memcpy(pvect.data()+maxsize, wri+1, remSize);
                }
            }
            else
            {
                mSpi.xfer(wro, wri, 1+rcvSz);
                std::memcpy(pvect.data(), wri+1, rcvSz);
                Logless("DBG SX1278::onDio1 FIFO AT: _", unsigned(rdBase));
            }


            {
                std::unique_lock<std::mutex> lock(bufferQueueMutex);
                bufferQueue.push_back(std::move(pvect));
            }

            mRxTxDoneCv.notify_one();
            setRegister(REGIRQFLAGS, RXDONEMASK);
            Logless("DBG SX1278::onDio1 RX DONE /");
        }
        else
        {
            {
                std::unique_lock<std::mutex> lock(mTxDoneMutex);
                mTxDone = true;
            }
            mRxTxDoneCv.notify_one();
            setRegister(REGIRQFLAGS, TXDONEMASK);
            Logless("DBG SX1278::onDio1 TX DONE!");
        }
    }

    bool mTeardown = false;
    std::mutex bufferQueueMutex;
    std::deque<bfc::Buffer> bufferQueue;

    std::condition_variable mRxTxDoneCv{};
    std::mutex mTxDoneMutex;
    bool mTxDone{};
    bool mLastPacketAddr;

    uint8_t mFrLsb;
    uint8_t mFrMid;
    uint8_t mFrMsb;
    uint8_t mModemConfig1;
    uint8_t mModemConfig2;
    uint8_t mModemConfig3;
    uint8_t mPaConfig;

    uint32_t mFosc = 32000000ul;
    unsigned mResetPin{};
    unsigned mDio1Pin{};
    int mDio1CbId{};
    Usage mUsage{};

    hwapi::ISpi& mSpi;
    hwapi::IGpio& mGpio;
};

} // flylora_sx127x

#endif // __SX1278_HPP__