#ifndef __APP_HPP__
#define __APP_HPP__

#include <regex>
#include <PigpioHwApi/IHwApi.hpp>
#include <logger/Logger.hpp>
#include <bfc/Udp.hpp>
#include <SX127x.hpp>
#include <SX1278.hpp>

namespace app
{

using Options = std::map<std::string, std::string>;

class Args
{
public:
    Args(const Options& pOptions);
    int getChannel() const;
    bfc::IpPort getCtrlAddr() const;
    bfc::IpPort getIoAddr() const;
    bool isTx() const;
    uint32_t getCarrier() const;
    flylora_sx127x::Bw getBw() const;
    flylora_sx127x::CodingRate getCr() const;
    flylora_sx127x::SpreadingFactor getSf() const;
    int getMtu() const;
    int getTxPower() const;
    flylora_sx127x::LnaGain getLnaGain() const;
    int getResetPin() const;
    int getGetDio1Pin() const;

private:
    uint32_t parseUnsigned(std::string pKey) const;
    int parseInt(std::string pKey) const;
    int parseInt(std::string pKey, int pDefaultValue) const;
    bfc::IpPort parseIpPort(std::string pKey, bfc::IpPort pDefault) const;
    flylora_sx127x::Bw parseBw(std::string pKey) const;
    flylora_sx127x::CodingRate parseCr(std::string pKey) const;
    flylora_sx127x::SpreadingFactor parseSf(std::string pKey) const;
    flylora_sx127x::LnaGain parseGain(std::string pKey) const;

    const Options& mOptions;
};

class App
{
public:
    App(bfc::IUdpFactory& pUdpFactory, const Args& pArgs);
    int run();

private:
    void runRx();
    void runTx();

    enum class Mode{TX, RX};
    uint32_t mChannel;
    bfc::IpPort mCtrlAddr;
    Mode mMode;
    bfc::IpPort mIoAddr;
    uint32_t mCarrier;
    flylora_sx127x::Bw mBw;
    flylora_sx127x::CodingRate mCr;
    flylora_sx127x::SpreadingFactor mSf;
    int mMtu;
    int mTxPower;
    flylora_sx127x::LnaGain mRxGain;
    int mResetPin;
    int mDio1Pin;
    std::unique_ptr<bfc::ISocket> mCtrlSock;
    std::unique_ptr<bfc::ISocket> mIoSock;
    std::shared_ptr<hwapi::ISpi>  mSpi;
    std::shared_ptr<hwapi::IGpio> mGpio;
    flylora_sx127x::SX1278 mModule;
    std::thread mCtrlReceiver;
    std::thread mModulelReceiver;
};

}
#endif //__APP_HPP__