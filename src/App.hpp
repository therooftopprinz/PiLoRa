#ifndef __APP_HPP__
#define __APP_HPP__

#include <regex>
#include <IHwApi.hpp>
#include <Logger.hpp>
#include <Udp.hpp>

namespace app
{

using Options = std::map<std::string, std::string>;

class Args
{
public:
    Args(const Options& pOptions)
        : mOptions(pOptions)
    {}

    int getChannel() const
    {
        return parseInt("channel");
    }

    net::IpPort getCtrlAddr() const
    {
        return parseIpPort("cx", {0, 2221u});
    }

    net::IpPort getIoAddr() const
    {
        if (isTx())
        {
            return parseIpPort("tx", {0, 0});
        }
        return parseIpPort("rx", {0, 0});
    }

    bool isTx() const
    {
        auto ioAddr = parseIpPort("rx", {0, 0});
        if (ioAddr.port == 0)
        {
            return true;
        }
        if (ioAddr.port == 0)
        {
            throw std::runtime_error("please select either tx or rx");
        }
        return false;
    }

    flylora_sx127x::Bw getBw() const
    {
        return parseBw("bandwidth");
    }

    flylora_sx127x::CodingRate getCr() const
    {
        return parseCr("coding-rate");
    }

    flylora_sx127x::SpreadingFactor getSf() const
    {
        return parseSf("spreading-factor");
    }

    int getMtu() const
    {
        return parseInt("mtu", 0);
    }

    int getTxPower() const
    {
        return parseInt("tx-power", 17);
    }

    flylora_sx127x::LnaGain getLnaGain() const
    {
        return parseGain("rx-gain");
    }

    int getResetPin() const
    {
        return parseInt("reset-pin");
    }

    int getGetDio1Pin() const
    {
        return parseInt("txrx-done-pin");
    }

private:
    int parseInt(std::string pKey) const
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            throw std::runtime_error(pKey + " option is missing!");
        }
        return std::stoi(it->second);
    }

    int parseInt(std::string pKey, int pDefaultValue) const
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return pDefaultValue;
        }
        return std::stoi(it->second);
    }

    net::IpPort parseIpPort(std::string pKey, net::IpPort pDefault) const
    {
        std::regex addressFilter("([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+):([0-9]+)");
        std::smatch match;
        auto it = mOptions.find(pKey);
        net::IpPort rv;
        if (it == mOptions.cend())
        {
            rv = pDefault;
        }
        else
        {
            if (std::regex_match(it->second, match, addressFilter))
            {
                if (match.size() != 6)
                {
                    throw std::runtime_error(std::string("invalid address: `") + it->second + "`");
                }
                uint8_t a = std::stoi(match[1].str());
                uint8_t b = std::stoi(match[2].str());
                uint8_t c = std::stoi(match[3].str());
                uint8_t d = std::stoi(match[4].str());
                uint16_t port = std::stoi(match[5].str());
                rv = net::toIpPort(a,b,c,d,port);
            }
            else
            {
                throw std::runtime_error(std::string("invalid address: `") + it->second + "`");
            }
        }
        return rv;
    }

    flylora_sx127x::Bw parseBw(std::string pKey) const
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return flylora_sx127x::Bw::BW_500_KHZ;
        }

        if (it->second == "7.8")   return flylora_sx127x::Bw::BW_7P8_KHZ;
        if (it->second == "10.4")  return flylora_sx127x::Bw::BW_10P4_KHZ;
        if (it->second == "15.6")  return flylora_sx127x::Bw::BW_15P6_KHZ;
        if (it->second == "20.8")  return flylora_sx127x::Bw::BW_20P8_KHZ;
        if (it->second == "31.25") return flylora_sx127x::Bw::BW_31P25_KHZ;
        if (it->second == "41.7")  return flylora_sx127x::Bw::BW_41P7_KHZ;
        if (it->second == "62.5")  return flylora_sx127x::Bw::BW_62P5_KHZ;
        if (it->second == "125")   return flylora_sx127x::Bw::BW_125_KHZ;
        if (it->second == "250")   return flylora_sx127x::Bw::BW_250_KHZ;
        if (it->second == "500")   return flylora_sx127x::Bw::BW_500_KHZ;

        throw std::runtime_error(it->second + " is invalid bandwidth value");
    }

    flylora_sx127x::CodingRate parseCr(std::string pKey) const
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return flylora_sx127x::CodingRate::CR_4V5;
        }

        if (it->second == "4/5") return flylora_sx127x::CodingRate::CR_4V5;
        if (it->second == "4/6") return flylora_sx127x::CodingRate::CR_4V6;
        if (it->second == "4/7") return flylora_sx127x::CodingRate::CR_4V7;
        if (it->second == "4/8") return flylora_sx127x::CodingRate::CR_4V8;

        throw std::runtime_error(it->second + " is invalid coding rate value");
    }

    flylora_sx127x::SpreadingFactor parseSf(std::string pKey) const
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return flylora_sx127x::SpreadingFactor::SF_7;
        }

        if (it->second == "SF6") return flylora_sx127x::SpreadingFactor::SF_6;
        if (it->second == "SF7") return flylora_sx127x::SpreadingFactor::SF_7;
        if (it->second == "SF8") return flylora_sx127x::SpreadingFactor::SF_8;
        if (it->second == "SF9") return flylora_sx127x::SpreadingFactor::SF_9;
        if (it->second == "SF10") return flylora_sx127x::SpreadingFactor::SF_10;
        if (it->second == "SF11") return flylora_sx127x::SpreadingFactor::SF_11;
        if (it->second == "SF12") return flylora_sx127x::SpreadingFactor::SF_12;

        throw std::runtime_error(it->second + " is invalid spreading factor value");
    }

    flylora_sx127x::LnaGain parseGain(std::string pKey) const
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return flylora_sx127x::LnaGain::G1;
        }

        if (it->second == "G1") return flylora_sx127x::LnaGain::G1;
        if (it->second == "G2") return flylora_sx127x::LnaGain::G2;
        if (it->second == "G3") return flylora_sx127x::LnaGain::G3;
        if (it->second == "G4") return flylora_sx127x::LnaGain::G4;
        if (it->second == "G5") return flylora_sx127x::LnaGain::G5;
        if (it->second == "G6") return flylora_sx127x::LnaGain::G6;

        throw std::runtime_error(it->second + " is invalid lna gain value");
    }


private:
    const Options& mOptions;
};

class App
{
public:
    App(net::IUdpFactory& pUdpFactory, const Args& pArgs)
        : mChannel(pArgs.getChannel())
        , mCtrlAddr(pArgs.getCtrlAddr())
        , mMode(pArgs.isTx()? Mode::TX : Mode::RX)
        , mIoAddr(pArgs.getIoAddr())
        , mBw(pArgs.getBw())
        , mCr(pArgs.getCr())
        , mSf(pArgs.getSf())
        , mMtu(pArgs.getMtu())
        , mTxPower(pArgs.getTxPower())
        , mRxGain(pArgs.getLnaGain())
        , mResetPin(pArgs.getResetPin())
        , mDio1Pin(pArgs.getGetDio1Pin())
        , mCtrlSock(pUdpFactory.create())
        , mIoSock(pUdpFactory.create())
        , mSpi(hwapi::getSpi(mChannel))
        , mGpio(hwapi::getGpio())
        , mModule(*mSpi, *mGpio, mResetPin, mDio1Pin)
        , mLogger("App")
    {
        mLogger << logger::DEBUG << "-------------- Parameters ---------------";
        mLogger << logger::DEBUG << "channel:         " << mChannel;
        mLogger << logger::DEBUG << "Mode:            " << ((const char*[]){"TX", "RX"})[int(mMode)];
        mLogger << logger::DEBUG << "Control Address: "
            << ((mCtrlAddr.addr>>24)&0xFF) << "."
            << ((mCtrlAddr.addr>>16)&0xFF) << "."
            << ((mCtrlAddr.addr>>8)&0xFF) << "."
            << (mCtrlAddr.addr&0xFF) << ":"
            << (mCtrlAddr.port);
        mLogger << logger::DEBUG << "TX/RX Address:   "
            << ((mIoAddr.addr>>24)&0xFF) << "."
            << ((mIoAddr.addr>>16)&0xFF) << "."
            << ((mIoAddr.addr>>8)&0xFF) << "."
            << (mIoAddr.addr&0xFF) << ":"
            << (mIoAddr.port);
        mLogger << logger::DEBUG << "bandwidth:       " <<
            ((const char*[]){"7.8", "10.4", "15.6", "20.8", "31.25", "41.7", "62.5", "125", "250", "500",})[int(mCr)] << " kHz";
        mLogger << logger::DEBUG << "Coding Rate:     " <<
            ((const char*[]){0, "4/5", "4/6", "4/7", "4/8"})[int(mCr)];
        mLogger << logger::DEBUG << "Spread Factor:   " <<
            ((const char*[]){0,0,0,0,0,0,"SF6", "SF7", "SF8", "SF9", "SF10", "SF11", "SF12"})[int(mSf)];
        mLogger << logger::DEBUG << "MTU:             " << mMtu;
        mLogger << logger::DEBUG << "Tx Power:        " << mTxPower;
        mLogger << logger::DEBUG << "Rx Gain:         " <<
            ((const char*[]){"", "G1", "G2", "G3", "G4", "G5", "G6"})[int(mRxGain)];

        mCtrlSock->bind(mCtrlAddr);
        if (Mode::TX == mMode)
        {
            mIoSock->bind(mIoAddr);
        }
        else
        {
            mIoSock->bind({});
        }
    }

    int run()
    {
        mLogger << logger::DEBUG << "App::run";
        return 0;
    }

private:
    enum class Mode{TX, RX};
    uint32_t mChannel;
    net::IpPort mCtrlAddr;
    Mode mMode;
    net::IpPort mIoAddr;
    flylora_sx127x::Bw mBw;
    flylora_sx127x::CodingRate mCr;
    flylora_sx127x::SpreadingFactor mSf;
    int mMtu;
    int mTxPower;
    flylora_sx127x::LnaGain mRxGain;
    int mResetPin;
    int mDio1Pin;
    std::unique_ptr<net::ISocket> mCtrlSock;
    std::unique_ptr<net::ISocket> mIoSock;
    std::shared_ptr<hwapi::ISpi>  mSpi;
    std::shared_ptr<hwapi::IGpio> mGpio;
    flylora_sx127x::SX1278 mModule;
    logger::Logger mLogger;
};

}
#endif //__APP_HPP__