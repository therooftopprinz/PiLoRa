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
                uint8_t port = std::stoi(match[5].str());
                rv = {uint32_t((a<<24)|(b<<16)|(c<<8)|d), port};
            }
            else
            {
                throw std::runtime_error(std::string("invalid address: `") + it->second + "`");
            }
        }
        return rv;
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
        mCtrl = parser.parseIpPort("cx", {0, 2221});
        mIo = parser.parseIpPort("rx", {0, 0});
        if (mIo.port == 0)
        {
            mIo = parser.parseIpPort("tx", {0, 0});
        }
        if (mIo.port == 0)
        {
            throw std::runtime_error("please select either tx or rx");
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
    net::IpPort mCtrl;
    Mode mMode;
    net::IpPort mIo;
    flylora_sx127x::Bw mBw;
    flylora_sx127x::CodingRate mCr;
    flylora_sx127x::SpreadngFactor mSf;
    uint8_t mMtu;
    uint8_t mTxPower;
    uint8_t mRxGain;

    std::unique_ptr<net::IUdpFactory> mUdpFactory;
    logger::Logger mLogger;
};

}
#endif //__APP_HPP__