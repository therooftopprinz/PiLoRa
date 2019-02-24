#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <SX1278.hpp>
#include <IHwApiMock.hpp>
#include <cstring>
#include <iomanip>
#include <iostream>

using namespace ::testing;
using namespace flylora_sx127x;

inline void printHex(const uint8_t* pData, size_t size)
{
    std::cout <<  "printHex(@" << (void*) pData << ", " << size << "): ";
    for (size_t i=0; i<size; i++)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    std::cout << "\n";
}

MATCHER_P2(isBufferEq, buffer, sz, "")
{
    // printHex(arg, sz);
    // printHex(buffer, sz);
    // std::cout << "\n";
    return std::memcmp(arg, buffer, sz) == 0;
}

struct SX1278Tests : Test
{
    static constexpr uint8_t mResetPin = 1;
    static constexpr uint8_t mDio1Pin = 2;

    SX1278Tests()
    {
    }

    void SetUp()
    {
        EXPECT_CALL(mGpioMock, setMode(mResetPin, hwapi::PinMode::OUTPUT));
        EXPECT_CALL(mGpioMock, setMode(mDio1Pin, hwapi::PinMode::INPUT));
        EXPECT_CALL(mGpioMock, set(mResetPin, 1)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(mGpioMock, registerCallback(mDio1Pin, hwapi::Edge::RISING, _));
        EXPECT_CALL(mGpioMock, deregisterCallback(_));

        expectInit();

        mSut = std::make_unique<flylora_sx127x::SX1278>(mSpiMock, mGpioMock, mResetPin, mDio1Pin);
    }

    void expectInit()
    {
        // TODO: don't use the enum
        constexpr auto LONGRANGEMODEMASK      = 0b10000000;
        constexpr auto LOWFREQUENCYMODEONMASK = 0b00001000;
        constexpr auto STDBY                  = 1;
        constexpr auto REGOPMODE              = 1;
        constexpr auto REGFIFOTXBASEADD       = 0x0E;
        constexpr auto REGFIFORXBASEADD       = 0x0F;

        // REGOPMODE
        static uint8_t startMode0[] = { uint8_t(0x80|REGOPMODE), 0};
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startMode0, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGOPMODE
        static uint8_t startMode0a[] = { uint8_t(0x80|REGOPMODE), uint8_t(LONGRANGEMODEMASK|LOWFREQUENCYMODEONMASK)};
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startMode0a, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGOPMODE
        static uint8_t startMode1[] = { uint8_t(0x80|REGOPMODE), uint8_t(LONGRANGEMODEMASK|LOWFREQUENCYMODEONMASK|STDBY)};
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startMode1, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGFIFOTXBASEADD
        static uint8_t startFifoTxBaseAddr[] = { uint8_t(0x80|REGFIFOTXBASEADD), 0 };
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startFifoTxBaseAddr, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGFIFORXBASEADD
        static uint8_t startFifoRxBaseAddr[] = { uint8_t(0x80|REGFIFORXBASEADD), 0 };
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startFifoRxBaseAddr, 2), _, 2)).Times(1).RetiresOnSaturation();
        
        // REGFIFORXCURRENTADDR
        static uint8_t startFifoRxCurrentAddr[] = { uint8_t(0x80|REGFIFORXCURRENTADDR), 0 };
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startFifoRxCurrentAddr, 2), _, 2)).Times(1).RetiresOnSaturation();
    }

    void TearDown()
    {
    }

    SpiMock mSpiMock;
    GpioMock mGpioMock;
    std::unique_ptr<flylora_sx127x::SX1278> mSut;
};

TEST(SX1278Utils, shouldGetUnmasked)
{
    EXPECT_EQ(0b00000011u, getUnmasked(0b00000011, 0b10101011));
    EXPECT_EQ(0b00000001u, getUnmasked(0b00000110, 0b10101011));
    EXPECT_EQ(0b00001010u, getUnmasked(0b11110000, 0b10101011));
}

TEST(SX1278Utils, shouldSetUnmasked)
{
    EXPECT_EQ(0b00000011u, setMasked(0b00000011, 0b11));
    EXPECT_EQ(0b00000100u, setMasked(0b00000110, 0b10));
    EXPECT_EQ(0b01101000u, setMasked(0b01111000, 0b1101));
}

TEST_F(SX1278Tests, shouldResetModule)
{
    testing::InSequence dummy;
    EXPECT_CALL(mGpioMock, set(mResetPin, 0)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(mGpioMock, set(mResetPin, 1)).Times(1).RetiresOnSaturation();
    
    expectInit();
    
    mSut->resetModule();
}

TEST_F(SX1278Tests, shouldStandby)
{
    constexpr auto LONGRANGEMODEMASK      = 0b10000000;
    constexpr auto LOWFREQUENCYMODEONMASK = 0b00001000;
    constexpr auto STDBY                  = 1;
    constexpr auto REGOPMODE              = 1;

    uint8_t startMode[] = { uint8_t(0x80|REGOPMODE), uint8_t(LONGRANGEMODEMASK|LOWFREQUENCYMODEONMASK|STDBY)};
    EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startMode, 2), _, 2)).Times(1).RetiresOnSaturation();
    mSut->standby();
}

TEST_F(SX1278Tests, shouldSetUsageRx)
{
    constexpr auto REGDIOMAPPING1 = 0x40;
    constexpr auto DIO0RXDONEMASK = 0;
    uint8_t dio1mapping[] = { uint8_t(0x80|REGDIOMAPPING1), DIO0RXDONEMASK};
    EXPECT_CALL(mSpiMock,  xfer(isBufferEq(dio1mapping, 2), _, 2)).Times(1).RetiresOnSaturation();
    mSut->setUsage(SX1278::Usage::RXC);
}

TEST_F(SX1278Tests, shouldSetUsageTx)
{
    constexpr auto REGDIOMAPPING1 = 0x40;
    constexpr auto DIO0TXDONEMASK = 0x40;
    uint8_t dio1mapping[] = { uint8_t(0x80|REGDIOMAPPING1), DIO0TXDONEMASK};
    EXPECT_CALL(mSpiMock,  xfer(isBufferEq(dio1mapping, 2), _, 2)).Times(1).RetiresOnSaturation();
    mSut->setUsage(SX1278::Usage::TX);
}

TEST_F(SX1278Tests, shouldSetCarrier)
{
    constexpr auto REGFRLSB = 8;
    constexpr auto REGFRMID = 7;
    constexpr auto REGFRMSB = 6;
    constexpr auto FOSC = 32000000ull;

    constexpr auto carrier = 434000000ul;
    constexpr auto frf = (carrier*524288ull)/FOSC;

    uint8_t frfLsb[] = { uint8_t(0x80|REGFRLSB), uint8_t(frf&0xFF) };
    uint8_t frfMid[] = { uint8_t(0x80|REGFRMID), uint8_t(frf>>8&0xFF) };
    uint8_t frfMsb[] = { uint8_t(0x80|REGFRMSB), uint8_t(frf>>16&0xFF) };

    testing::InSequence dummy;
    EXPECT_CALL(mSpiMock,  xfer(isBufferEq(frfLsb, 2), _, 2)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(mSpiMock,  xfer(isBufferEq(frfMid, 2), _, 2)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(mSpiMock,  xfer(isBufferEq(frfMsb, 2), _, 2)).Times(1).RetiresOnSaturation();

    mSut->setCarrier(carrier);
}