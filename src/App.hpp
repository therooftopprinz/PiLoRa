#ifndef __APP_HPP__
#define __APP_HPP__

#include <regex>
#include <IHwApi.hpp>
#include <Logger.hpp>
#include <Udp.hpp>

namespace app
{

using Options = std::map<std::string, std::string>;

class ArgParser
{
public:
    ArgParser(const Options& pOptions)
        : mOptions(pOptions)
    {}

    int parseInt(std::string pKey)
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            throw std::runtime_error(pKey + " option is missing!");
        }
        return std::stoi(it->second);
    }

    int parseInt(std::string pKey, int pDefaultValue)
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return pDefaultValue;
        }
        return std::stoi(it->second);
    }

    net::IpPort parseIpPort(std::string pKey, net::IpPort pDefault)
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
                rv = {uint32_t((a<<24)|(b<<16)|(c<<8)|d), port};
            }
            else
            {
                throw std::runtime_error(std::string("invalid address: `") + it->second + "`");
            }
        }
        return rv;
    }

    flylora_sx127x::Bw parseBw(std::string pKey)
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

    flylora_sx127x::CodingRate parseCr(std::string pKey)
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

    flylora_sx127x::SpreadngFactor parseSf(std::string pKey)
    {
        auto it = mOptions.find(pKey);
        if (it == mOptions.cend())
        {
            return flylora_sx127x::SpreadngFactor::SF_7;
        }

        if (it->second == "SF6") return flylora_sx127x::SpreadngFactor::SF_6;
        if (it->second == "SF7") return flylora_sx127x::SpreadngFactor::SF_7;
        if (it->second == "SF8") return flylora_sx127x::SpreadngFactor::SF_8;
        if (it->second == "SF9") return flylora_sx127x::SpreadngFactor::SF_9;
        if (it->second == "SF10") return flylora_sx127x::SpreadngFactor::SF_10;
        if (it->second == "SF11") return flylora_sx127x::SpreadngFactor::SF_11;
        if (it->second == "SF12") return flylora_sx127x::SpreadngFactor::SF_12;

        throw std::runtime_error(it->second + " is invalid spreading factor value");
    }

    flylora_sx127x::LnaGain parseGain(std::string pKey)
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
    using Options = std::map<std::string, std::string>;
    App(std::unique_ptr<net::IUdpFactory> pUdpFactory, const Options& pOptions)
        : mUdpFactory(std::move(pUdpFactory))
        , mLogger("App")
    {
        mLogger << logger::DEBUG << "App::App";

        ArgParser parser(pOptions);

        mChannel = parser.parseInt("channel");
        mCtrl = parser.parseIpPort("cx", {0, 2221u});
        mIo = parser.parseIpPort("rx", {0, 0});

        mMode = Mode::RX;

        if (mIo.port == 0)
        {
            mMode = Mode::TX;
            mIo = parser.parseIpPort("tx", {0, 0});
        }

        if (mIo.port == 0)
        {
            throw std::runtime_error("please select either tx or rx");
        }

        mBw = parser.parseBw("bandwidth");
        mCr = parser.parseCr("coding-rate");
        mSf = parser.parseSf("spreading-factor");
        mMtu = parser.parseInt("mtu", 0);
        mTxPower = parser.parseInt("tx-power", 17);
        mRxGain = parser.parseGain("rx-gain");

        mLogger << logger::DEBUG << "-------------- Parameters ---------------";
        mLogger << logger::DEBUG << "channel:         " << mChannel;
        mLogger << logger::DEBUG << "Mode:            " << ((const char*[]){"TX", "RX"})[int(mMode)];
        mLogger << logger::DEBUG << "Control Address: "
            << ((mCtrl.addr>>24)&0xFF) << "."
            << ((mCtrl.addr>>16)&0xFF) << "."
            << ((mCtrl.addr>>8)&0xFF) << "."
            << (mCtrl.addr&0xFF) << ":"
            << (mCtrl.port);
        mLogger << logger::DEBUG << "TX/RX Address:   "
            << ((mIo.addr>>24)&0xFF) << "."
            << ((mIo.addr>>16)&0xFF) << "."
            << ((mIo.addr>>8)&0xFF) << "."
            << (mIo.addr&0xFF) << ":"
            << (mIo.port);

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
    }

    int run()
    {
        mLogger << logger::DEBUG << "App::run";
        return 0;
    }

private:
    enum class Mode{TX, RX};
    uint32_t mChannel;
    net::IpPort mCtrl;
    Mode mMode;
    net::IpPort mIo;
    flylora_sx127x::Bw mBw;
    flylora_sx127x::CodingRate mCr;
    flylora_sx127x::SpreadngFactor mSf;
    int mMtu;
    int mTxPower;
    flylora_sx127x::LnaGain mRxGain;

    std::unique_ptr<net::IUdpFactory> mUdpFactory;
    logger::Logger mLogger;
};

}
#endif //__APP_HPP__