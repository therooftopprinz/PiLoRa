#ifndef __SX1278_HPP__
#define __SX1278_HPP__

#include <thread>
#include "SX127x.hpp"

namespace flylora_sx1278
{

class LoRa
{
public:
    enum class Usage {TX, RXC};

    LoRa(ISpi& pSpi, IGpio& pGpio, unsigned pResetPin, unsigned pDio1Pin, unsigned pDio2Pin)
        : mResetPin(pResetPin)
        , mSpi(pSpi)
        , mGpio(pGpio)
    {
        mGpio.setMode(resetPin, PI_OUTPUT);
        mGpio.setMode(dio1Pin, PI_INPUT);
        mGpio.setMode(dio2Pin, PI_INPUT);
        mGpio.set(mResetPin, 0);
    }

    void resetModule()
    {
        using namespace std::chrono_literals;
        mGpio.set(mResetPin, 1);
        std::this_thread::sleep_for(100us);
        mGpio.set(mResetPin, 0);
        std::this_thread::sleep_for(5ms);
    }

    void setUsage(Usage pUsage)
    {
        mUsage = pUsage;
        prepareUsageMode();
    }

    uint8_t getMode()
    {
        uint8_t wro[2] = {RegOpMode, 0};
        uint8_t wri[2];
        mSpi.xfer(wro, wri, 2);
        return getUnmasked(ModeMask, wri[2]);
    }

    void setRegister(uint8_t reg, uint8_t mask, uint8_t value)
    {
        uint8_t wro[2] = {0x80|reg, 0};
        uint8_t wri[2];
        mSpi.xfer(wro, wri, 2);
        value = setMasked(mask, value);
        wro[0] |= 0x80;
        wro[1] = (wri[1] & (~mask)) | value;
        wri[0] = 0;
        wri[1] = 0;
        mSpi.xfer(wro, wri, 2);
    }

    void setMode(uint8_t mode)
    {
        uint8_t wro[2] = {0x80|RegOpMode, 0};
        uint8_t wri[2];
        wro[1] = LongRangeModeMask | (isHf() ? 0 : LowFrequencyModeOnMask) | setMasked(ModeMask, mode);
        mSpi.xfer(wro, wri, 2);
    }

    void setCarrier(uint32_t cf)
    {
        // SET MODE STANDBY
        setMode(Mode::STDBY);

        // TODO: DO SPURRIOUS OPTIMIZATION

        uint8_t wro[2];
        uint8_t wri[2];

        wro[0] = 0x80 | RegFrLsb;
        wro[1] = cf&0xFF;
        mSpi.xfer(wro, wri, 2);

        wro[0] = 0x80 | RegFrMid;
        wro[1] = (cf>>8)&0xFF;
        mSpi.xfer(wro, wri, 2);

        wro[0] = 0x80 | RegFrMsb;
        wro[1] = (cf>>16)&0xFF;
        mSpi.xfer(wro, wri, 2);

        prepareUsageMode();
    }

    uin32_t getCarrier()
    {
        uint32_t cf = 0;

        uint8_t wro[2] = {0, 0};
        uint8_t wri[2] = {0, 0};

        wro[0] = RegFrLsb;
        mSpi.xfer(wro, wri, 2);
        cf |= wri[1];

        wro[0] = egFrMid;
        wro[1] = (cf>>8)&0xFF;
        mSpi.xfer(wro, wri, 2);
        cf |= wri[1]<<8;

        wro[0] = RegFrMsb;
        wro[1] = (cf>>16)&0xFF;
        mSpi.xfer(wro, wri, 2);
        cf |= wri[1]<<16;

        return cf;
    }

    void configureModulation(uint8_t pBandwidth, uint8_t pCodingRate, bool implicitHeader, uint8_t pSpreadingFactor)
    {
        // SET MODE STANDBY
        setMode(Mode::STDBY);

        uint8_t config1 = setMasked(BwMask, pBandwidth) | setMasked(CodingRateMask, pCodingRate) | setMasked(CodingRImplicitHeaderModeOnMaskateMask, implicitHeader);
        uint8_t config2 = setMasked(SpreadngFactorMask, pSpreadingFactor);
        uint8_t wro[2];
        uint8_t wri[2];

        wro[0] = 0x80 | RegModemConfig1;
        wro[1] = config1;
        mSpi.xfer(wro, wri, 2);

        wro[0] = 0x80 | RegModemConfig2;
        wro[1] = config2;
        mSpi.xfer(wro, wri, 2);

        prepareUsageMode();
    }

    void setPaSelect(); // +14 dbm or +20 dbm
    void setMaxPower();
    void setOutputPower(); //

    void setLnaGain();

    void getLastRssi();

    void tx(); // transmit
    void rx(); // receive

private:
    void onRxDone();
    void onTxDone();
    void prepareUsageMode()
    {
        if (Usage::RXC == mUsage)
        {
            setMoode(Mode::RXCONTINUOUS);
        }
    }

    bool isHf() {return mCarrier>500;} // TODO CHECK

    unsigned mResetPin;
    unsigned Usage mUsage;

    unsigned mCarrier = 433;
    unsigned mBandwidth;
    unsigned mSpreadingFactor;
    unsigned m;
    ISpi& mSpi;
    IGpio& mGpio;
};

} // flylora_sx1278

#endif __SX1278_HPP__