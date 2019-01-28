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
    static constexpr uint8_t mDio2Pin = 3;

    SX1278Tests()
    {
    }

    void SetUp()
    {
        EXPECT_CALL(mGpioMock, setMode(mResetPin, hwapi::PinMode::OUTPUT));
        EXPECT_CALL(mGpioMock, setMode(mDio1Pin, hwapi::PinMode::INPUT));
        EXPECT_CALL(mGpioMock, setMode(mDio2Pin, hwapi::PinMode::INPUT));
        EXPECT_CALL(mGpioMock, set(mResetPin, 0)).Times(1).RetiresOnSaturation();

        static uint8_t startMode[] = { uint8_t(0x80|REGOPMODE), uint8_t(LONGRANGEMODEMASK|LOWFREQUENCYMODEONMASK|uint8_t(Mode::STDBY))};
        EXPECT_CALL(mSpiMock,  xfer(isBufferEq(startMode, 2), _, 2)).Times(1).RetiresOnSaturation();

        mSut = std::make_unique<flylora_sx127x::SX1278>(mSpiMock, mGpioMock, mResetPin, mDio1Pin, mDio2Pin);
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
    mSut->resetModule();
}

TEST(SX1278, should_)
{
    //
}