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
    std::cout <<  "printHex(" << size << "): ";
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
        EXPECT_CALL(mGpioMock, set(mResetPin, 0)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(mGpioMock, registerCallback(mDio1Pin, hwapi::Edge::RISING, _));
        EXPECT_CALL(mGpioMock, deregisterCallback(_));

        expectInit();

        mSut = std::make_unique<flylora_sx127x::SX1278>(mSpiMock, mGpioMock, mResetPin, mDio1Pin);
    }

    void expectInit()
    {
        // TODO: don't use the enum
        auto LONGRANGEMODEMASK      = flylora_sx127x::LONGRANGEMODEMASK;
        auto LOWFREQUENCYMODEONMASK = flylora_sx127x::LOWFREQUENCYMODEONMASK;
        auto STDBY                  = Mode::STDBY;
        auto REGOPMODE              = flylora_sx127x::REGOPMODE;
        auto REGIRQFLAGSMASKMASK    = flylora_sx127x::REGIRQFLAGSMASKMASK;
        auto REGFIFOTXBASEADD       = flylora_sx127x::REGFIFOTXBASEADD;
        auto REGFIFORXBASEADD       = flylora_sx127x::REGFIFORXBASEADD;

        // REGOPMODE
        static uint8_t startMode[] = { uint8_t(0x80|REGOPMODE), uint8_t(LONGRANGEMODEMASK|LOWFREQUENCYMODEONMASK|uint8_t(STDBY))};
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startMode, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGIRQFLAGSMASKMASK
        static uint8_t startIrqFlag[] = { uint8_t(0x80|REGIRQFLAGSMASKMASK), uint8_t(TXDONEMASKMASK | RXDONEMASKMASK) };
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startIrqFlag, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGFIFOTXBASEADD
        static uint8_t startFifoTxBaseAddr[] = { uint8_t(0x80|REGFIFOTXBASEADD), 0 };
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startFifoTxBaseAddr, 2), _, 2)).Times(1).RetiresOnSaturation();

        // REGFIFORXBASEADD
        static uint8_t startFifoRxBaseAddr[] = { uint8_t(0x80|REGFIFORXBASEADD), 0 };
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startFifoRxBaseAddr, 2), _, 2)).Times(1).RetiresOnSaturation();
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

TEST_F(SX1278Tests, should_ResetModule)
{
    testing::InSequence dummy;
    EXPECT_CALL(mGpioMock, set(mResetPin, 1)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(mGpioMock, set(mResetPin, 0)).Times(1).RetiresOnSaturation();

    expectInit();

    mSut->resetModule();
}

TEST(SX1278, should_)
{
    //
}