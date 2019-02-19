#include <App.hpp>

namespace app
{

Args::Args(const Options& pOptions)
    : mOptions(pOptions)
{}

int Args::getChannel() const
{
    return parseInt("channel");
}

net::IpPort Args::getCtrlAddr() const
{
    return parseIpPort("cx", {0, 2221u});
}

net::IpPort Args::getIoAddr() const
{
    if (isTx())
    {
        return parseIpPort("tx", {0, 0});
    }
    return parseIpPort("rx", {0, 0});
}

bool Args::isTx() const
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

uint32_t Args::getCarrier() const
{
    return parseUnsigned("carrier");
}

flylora_sx127x::Bw Args::getBw() const
{
    return parseBw("bandwidth");
}

flylora_sx127x::CodingRate Args::getCr() const
{
    return parseCr("coding-rate");
}

flylora_sx127x::SpreadingFactor Args::getSf() const
{
    return parseSf("spreading-factor");
}

int Args::getMtu() const
{
    return parseInt("mtu", 0);
}

int Args::getTxPower() const
{
    return parseInt("tx-power", 17);
}

flylora_sx127x::LnaGain Args::getLnaGain() const
{
    return parseGain("rx-gain");
}

int Args::getResetPin() const
{
    return parseInt("reset-pin");
}

int Args::getGetDio1Pin() const
{
    return parseInt("txrx-done-pin");
}

uint32_t Args::parseUnsigned(std::string pKey) const
{
    auto it = mOptions.find(pKey);
    if (it == mOptions.cend())
    {
        throw std::runtime_error(pKey + " option is missing!");
    }
    return std::stoul(it->second);
}

int Args::parseInt(std::string pKey) const
{
    auto it = mOptions.find(pKey);
    if (it == mOptions.cend())
    {
        throw std::runtime_error(pKey + " option is missing!");
    }
    return std::stoi(it->second);
}

int Args::parseInt(std::string pKey, int pDefaultValue) const
{
    auto it = mOptions.find(pKey);
    if (it == mOptions.cend())
    {
        return pDefaultValue;
    }
    return std::stoi(it->second);
}

net::IpPort Args::parseIpPort(std::string pKey, net::IpPort pDefault) const
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

flylora_sx127x::Bw Args::parseBw(std::string pKey) const
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

flylora_sx127x::CodingRate Args::parseCr(std::string pKey) const
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

flylora_sx127x::SpreadingFactor Args::parseSf(std::string pKey) const
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

flylora_sx127x::LnaGain Args::parseGain(std::string pKey) const
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

App::App(net::IUdpFactory& pUdpFactory, const Args& pArgs)
    : mChannel(pArgs.getChannel())
    , mCtrlAddr(pArgs.getCtrlAddr())
    , mMode(pArgs.isTx()? Mode::TX : Mode::RX)
    , mIoAddr(pArgs.getIoAddr())
    , mCarrier(pArgs.getCarrier())
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
    mLogger << logger::DEBUG << "Carrier:         " << mCarrier << " Hz";
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
    mLogger << logger::DEBUG << "Reset Pin:       " << mResetPin;
    mLogger << logger::DEBUG << "TX/RX Done Pin:  " << mDio1Pin;

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

int App::run()
{
    mLogger << logger::DEBUG << "App::run Initializing LoRa module.";
    mModule.resetModule();

    bool validated = false;
    for (int i=0; i<3; i++)
    {
        mLogger << logger::DEBUG << "Configuring LoRa module...";
        mModule.setUsage(Mode::TX==mMode ? flylora_sx127x::SX1278::Usage::TX :
            flylora_sx127x::SX1278::Usage::RXC);
        mModule.setCarrier(mCarrier);
        mModule.configureModem(mBw, mCr, false, mSf);
        mModule.setOutputPower(mTxPower);
        {
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(100ms); // sx1278 configuration grace period
        }
        if (mModule.validate())
        {
            validated = true;
            mLogger << logger::DEBUG << "LoRa module configured!";
            break;
        }
        mLogger << logger::ERROR << "Validation failed!";
    }

    if (!validated)
    {
        throw std::runtime_error("LoRa module can't be configured!");
    }

    mModule.start();

    if (Mode::RX == mMode)
    {
        runRx();
    }
    else
    {
        runTx();
    }
    return 0;
}

void App::runRx()
{
    while (1)
    {
        common::Buffer received = mModule.rx();
        if (received.size())
        {
            mIoSock->sendto(received, mIoAddr);
        }
    }
}

void App::runTx()
{
    common::Buffer recvbuffer(new std::byte[1024], 1024);
    while (1)
    {
        net::IpPort src;
        auto sz = mIoSock->recvfrom(recvbuffer, src);
        mModule.tx((uint8_t*)recvbuffer.data(), sz);
    }
}


} // namespace app