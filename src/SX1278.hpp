#ifndef __SX1278_HPP__
#define __SX1278_HPP__

#include <thread>
#include <SX127x.hpp>
#include <IHwApi.hpp>
#include <condition_variable>
#include <atomic>

namespace flylora_sx127x
{

class SX1278
{
public:
    enum class Usage {TX, RXC};

    SX1278(hwapi::ISpi& pSpi, hwapi::IGpio& pGpio, unsigned pResetPin, unsigned pDio1Pin)
        : mResetPin(pResetPin)
        , mDio1Pin(pDio1Pin)
        , mSpi(pSpi)
        , mGpio(pGpio)
    {
        mGpio.setMode(pResetPin, hwapi::PinMode::OUTPUT);
        mGpio.setMode(pDio1Pin,  hwapi::PinMode::INPUT);
        mGpio.set(mResetPin, 0);
        mDio1CbId = mGpio.registerCallback(mDio1Pin, hwapi::Edge::RISING, [](uint32_t) {});

        setMode(Mode::STDBY);
    }

    void resetModule()
    {
        // TODO: Anotate specs
        using namespace std::chrono_literals;
        mGpio.set(mResetPin, 1);
        std::this_thread::sleep_for(100us);
        mGpio.set(mResetPin, 0);
        std::this_thread::sleep_for(5ms);
    }

    void setUsage(Usage pUsage)
    {
        mUsage = pUsage;
        // TODO: SET DIO MAPPING
        // TODO: SET INTERRUPT MASK
        // TODO: SetTxBase = 0
        // TODO: SetRxBase = 0

        prepareUsageMode();
    }

    uint8_t getMode()
    {
        uint8_t wro[2] = {REGOPMODE, 0};
        uint8_t wri[2];
        mSpi.xfer(wro, wri, 2);
        return getUnmasked(MODEMASK, wri[2]);
    }

    void setRegister(uint8_t reg, uint8_t mask, uint8_t value)
    {
        uint8_t wro[2] = {uint8_t(0x80|reg), 0};
        uint8_t wri[2];
        mSpi.xfer(wro, wri, 2);
        value = setMasked(mask, value);
        wro[0] |= 0x80;
        wro[1] = (wri[1] & (~mask)) | value;
        wri[0] = 0;
        wri[1] = 0;
        mSpi.xfer(wro, wri, 2);
    }

    void setMode(Mode mode)
    {
        uint8_t wro[2] = {0x80|REGOPMODE, 0};
        uint8_t wri[2];
        wro[1] = LONGRANGEMODEMASK | LOWFREQUENCYMODEONMASK | setMasked(MODEMASK, uint8_t(mode));
        mSpi.xfer(wro, wri, 2);
    }

    void setCarrier(uint32_t cf)
    {
        // SET MODE STANDBY
        setMode(Mode::STDBY);

        // TODO: DO SPURRIOUS OPTIMIZATION

        // TODO: DO DetectionOptimize

        uint8_t wro[2];
        uint8_t wri[2];

        wro[0] = 0x80 | REGFRLSB;
        wro[1] = cf&0xFF;
        mSpi.xfer(wro, wri, 2);

        wro[0] = 0x80 | REGFRMID;
        wro[1] = (cf>>8)&0xFF;
        mSpi.xfer(wro, wri, 2);

        wro[0] = 0x80 | REGFRMSB;
        wro[1] = (cf>>16)&0xFF;
        mSpi.xfer(wro, wri, 2);

        prepareUsageMode();
    }

    uint32_t getCarrier()
    {
        uint32_t cf = 0;

        uint8_t wro[2] = {0, 0};
        uint8_t wri[2] = {0, 0};

        wro[0] = REGFRLSB;
        mSpi.xfer(wro, wri, 2);
        cf |= wri[1];

        wro[0] = REGFRMID;
        wro[1] = (cf>>8)&0xFF;
        mSpi.xfer(wro, wri, 2);
        cf |= wri[1]<<8;

        wro[0] = REGFRMSB;
        wro[1] = (cf>>16)&0xFF;
        mSpi.xfer(wro, wri, 2);
        cf |= wri[1]<<16;

        return cf;
    }

    void configureModulation(uint8_t pBandwidth, uint8_t pCodingRate, bool implicitHeader, uint8_t pSpreadingFactor)
    {
        // SET MODE STANDBY
        setMode(Mode::STDBY);

        uint8_t config1 = setMasked(BWMASK, pBandwidth)
                        | setMasked(CODINGRATEMASK, pCodingRate)
                        | setMasked(IMPLICITHEADERMODEONMASK, implicitHeader);
        uint8_t config2 = setMasked(SPREADNGFACTORMASK, pSpreadingFactor);
        uint8_t wro[2];
        uint8_t wri[2];

        wro[0] = 0x80 | REGMODEMCONFIG1;
        wro[1] = config1;
        mSpi.xfer(wro, wri, 2);

        wro[0] = 0x80 | REGMODEMCONFIG2;
        wro[1] = config2;
        mSpi.xfer(wro, wri, 2);

        prepareUsageMode();
    }

    void setOutputPower(int8_t pPower)
    {
        // SET MODE STANDBY
        setMode(Mode::STDBY);

        // 5.4.2.  RF Power Amplifiers
        bool isPaBoost = false;
        uint8_t power = pPower;
        if (pPower>14)
        {
            isPaBoost = true;
            power = pPower-2;
        }

        uint8_t wro[2] = {uint8_t(0x80|REGPACONFIG),
            uint8_t(setMasked(PASELECTMASK, isPaBoost)
                  | setMasked(MAXPOWERMASK, 7)
                  | setMasked(OUTPUTPOWERMASK, power))};
        uint8_t wri[2];

        mSpi.xfer(wro, wri, 2);
    }

    void setLnaGain()
    {
        // RegModemConfig3

    }

    void getLastRssi()
    {
        // RegPktRssiValue
    }

    void tx(const uint8_t *pData, uint8_t pSize)
    {
        // TODO: SetPayloadSize
        // TODO: SetFifoPtr = 0
        // TODO: SetFifoData
        // TODO: SetModeCacheTx
        // TODO: SetModeTx
        // TODO: Wait TxDone
        // TODO: SetModeCacheStby
    }

    uint8_t rx(uint8_t *pData, uint8_t pSize)
    {
        // TODO: Pop Buffer
        return 0;
    }

private:
    void onDio1()
    {
        if (Mode::RXCONTINUOUS)
        {
            // TODO: SetFifoPtr = 0
            // TODO: GetFifoData(RxByte+RxSize) and push to buffer
            // TODO: RxByte = 0
        }
        else
        {
            // TODO: Notify TxDone
        }
    }

    void prepareUsageMode()
    {
        if (Usage::RXC == mUsage)
        {
            setMode(Mode::RXCONTINUOUS);
        }
    }

    std::condition_variable pTxDoneCv;
    std::atomic<bool> pTxDone;
    std::condition_variable pRxDoneCv;
    std::atomic<bool> pRxDone;

    unsigned mResetPin;
    unsigned mDio1Pin;
    int mDio1CbId;
    Usage mUsage;

    // TODO: CACHE CURRENT MODE

    unsigned mCarrier = 433;
    unsigned mBandwidth;
    unsigned mSpreadingFactor;
    hwapi::ISpi& mSpi;
    hwapi::IGpio& mGpio;
};

} // flylora_sx127x

#endif // __SX1278_HPP__