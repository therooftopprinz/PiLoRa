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
        // 7.2.2. Manual Reset - SX1276/77/78/79 DATASHEET
        using namespace std::chrono_literals;
        mGpio.set(mResetPin, 1);
        std::this_thread::sleep_for(100us);
        mGpio.set(mResetPin, 0);
        std::this_thread::sleep_for(5ms);
    }

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

    void setUsage(Usage pUsage)
    {
        mUsage = pUsage;

        // 4.1.6.1.  Digital IO Pin Mapping - SX1276/77/78/79 DATASHEET
        DioMapping1 dioMapping = DioMapping1::CadDone_FhssChangeChannel_RxTimeout_RxDone;
        if (Usage::TX == mUsage)
        {
            dioMapping = DioMapping1::ValidHeader_FhssChangeChannel_FhssChangeChannel_TxDone;
        }

        // 4.1.2.4.  Interrupts in LoRa Mode - SX1276/77/78/79 DATASHEET
        // 4.1.6.    LoRaTM Modem State Machine Sequences - SX1276/77/78/79 DATASHEET

        uint8_t interruptMask = TXDONEMASKMASK | RXDONEMASKMASK;

        // 4.1.2.3.  LoRaTM Mode FIFO Data Buffer - SX1276/77/78/79 DATASHEET
        setRegister(REGDIOMAPPING1, uint8_t(dioMapping));
        setRegister(REGIRQFLAGSMASKMASK, interruptMask);
        setRegister(REGFIFOTXBASEADD, 0);
        setRegister(REGFIFORXBASEADD, 0);
    }

    uint8_t getMode()
    {
        return getUnmasked(MODEMASK, getRegister(REGOPMODE));
    }

    // void setRegister(uint8_t reg, uint8_t mask, uint8_t value)
    // {
    //     uint8_t wro[2] = {uint8_t(0x80|reg), 0};
    //     uint8_t wri[2];
    //     mSpi.xfer(wro, wri, 2);
    //     value = setMasked(mask, value);
    //     wro[0] |= 0x80;
    //     wro[1] = (wri[1] & (~mask)) | value;
    //     wri[0] = 0;
    //     wri[1] = 0;
    //     mSpi.xfer(wro, wri, 2);
    // }

    void setMode(Mode mode)
    {
        setRegister(REGOPMODE, LONGRANGEMODEMASK | LOWFREQUENCYMODEONMASK | setMasked(MODEMASK, uint8_t(mode)));
    }

    void setCarrier(uint32_t cf)
    {
        // 4.1.4.  Frequency Settings - SX1276/77/78/79 DATASHEET
        // 6.4.    LoRaTM Mode Register Map - SX1276/77/78/79 DATASHEET

        // TODO: DO SPURRIOUS OPTIMIZATION - SX1276/77/78 Errata fixes
        // TODO: DO DetectionOptimize - SX1276/77/78 Errata fixes

        setRegister(REGFRLSB, cf&0xFF);
        setRegister(REGFRMID, (cf>>8)&0xFF);
        setRegister(REGFRMSB, (cf>>16)&0xFF);
    }

    uint32_t getCarrier()
    {
        // 4.1.4.  Frequency Settings - SX1276/77/78/79 DATASHEET
        // 6.4.    LoRaTM Mode Register Map - SX1276/77/78/79 DATASHEET
        uint32_t cf = 0;
        cf |= getRegister(REGFRLSB);
        cf |= getRegister(REGFRMID)<<8;
        cf |= getRegister(REGFRMSB)<<16;
        return cf;
    }

    void configureModem(Bw pBandwidth, CodingRate pCodingRate, bool implicitHeader, SpreadngFactor pSpreadingFactor)
    {

        // 4.1.1. Link Design Using the LoRaTM Modem - SX1276/77/78/79 DATASHEET
        // 6.4.   LoRaTM Mode Register Map - SX1276/77/78/79 DATASHEET

        if (SpreadngFactor::SF_6 == pSpreadingFactor)
        {
            implicitHeader = true;
        }

        uint8_t config1 = setMasked(BWMASK, uint8_t(pBandwidth))
                        | setMasked(CODINGRATEMASK, uint8_t(pCodingRate))
                        | setMasked(IMPLICITHEADERMODEONMASK, implicitHeader);
        uint8_t config2 = setMasked(SPREADNGFACTORMASK, uint8_t(pSpreadingFactor));
        uint8_t config3 = (uint8_t(pSpreadingFactor) >= uint8_t(SpreadngFactor::SF_11) ? LOWDATARATEOPTIMIZEMASK : 0); // DEFAULT LNA GAIN IS G1

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

        setRegister(REGPACONFIG,
                    uint8_t(setMasked(PASELECTMASK, isPaBoost)
                  | setMasked(MAXPOWERMASK, 7)
                  | setMasked(OUTPUTPOWERMASK, power)));
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
        if (Usage::RXC == mUsage)
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

    std::condition_variable pTxDoneCv{};
    std::atomic<bool> pTxDone{};
    std::condition_variable pRxDoneCv{};
    std::atomic<bool> pRxDone{};

    unsigned mResetPin{};
    unsigned mDio1Pin{};
    int mDio1CbId{};
    Usage mUsage{};

    // TODO: CACHE CURRENT MODE

    unsigned mCarrier = 433;
    Bw mBandwidth{};
    SpreadngFactor mSpreadingFactor{};
    hwapi::ISpi& mSpi;
    hwapi::IGpio& mGpio;
};

} // flylora_sx127x

#endif // __SX1278_HPP__