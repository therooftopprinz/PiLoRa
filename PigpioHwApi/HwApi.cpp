#include <IHwApi.hpp>
#include <exception>
#include <pigpiod_if2.h>
#include <Logger.hpp>
#include <iomanip>
#include <SX127x.hpp>

namespace hwapi
{

std::string toHexString(const uint8_t* pData, size_t size)
{
    std::stringstream ss;;
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    return ss.str();
}

static int gGpioHandle = -1;

class Spi : public ISpi
{
public:
    Spi(uint8_t pChannel)
    {
        logger << logger::DEBUG << "opening spi channel="<< unsigned(pChannel) << " for gpio=" << gGpioHandle;
        mHandle  = spi_open(gGpioHandle, pChannel, 921600,
            //bbbbbbRTnnnnWAuuupppmm
            0b0000000000000000000000);
        if (mHandle<0)
        {
            throw std::runtime_error(std::string{} + "spi_open failed! handle=" + std::to_string(mHandle));
        }
    }

    ~Spi()
    {
        if (spi_close>=0)
        {
            spi_close(gGpioHandle, mHandle);
        }
    }

    int read(uint8_t *data, unsigned count)
    {
        throw std::runtime_error("spi read/write not implemented!");
    }
    int write(uint8_t *data, unsigned count)
    {
        throw std::runtime_error("spi read/write not implemented!");
    }
    int xfer(uint8_t *dataOut, uint8_t *dataIn, unsigned count)
    {
        auto rv = spi_xfer(gGpioHandle, mHandle, (char*)dataOut, (char*)dataIn, count);
        auto regname = flylora_sx127x::regIndexToString(dataOut[0]&0x7f);
        logger << logger::DEBUG << "SPI XFER REG" << (dataOut[0]&0x80 ? "WRITE" : "READ ") << " " << regname;
        logger << logger::DEBUG << rv << " = OUT xfer[" << count << "]: " << toHexString(dataOut, count);
        logger << logger::DEBUG << rv << " = IN  xfer[" << count << "]: " << toHexString(dataIn, count);
        return rv;
    }
private:
    int mHandle;
    logger::Logger logger = logger::Logger("hwapi::Spi");
};

class Gpio : public IGpio
{
public:
    int setMode(unsigned pGpio, PinMode pMode)
    {
        int pinmode = PI_INPUT;
        if (PinMode::OUTPUT == pMode)
        {
            pinmode = PI_OUTPUT;
        }
        auto rv = set_mode(gGpioHandle, pGpio, pinmode);
        logger << logger::DEBUG << rv << " = setMode(" << pGpio << ", " << (PinMode::OUTPUT==pMode? "OUTPUT" : "INPUT") << ")" ;

        // logger << logger::DEBUG << set_noise_filter(gGpioHandle, pGpio, 1, 1) << " = set_noise_filter()";
        // logger << logger::DEBUG << set_glitch_filter(gGpioHandle, pGpio, 1) << " = set_glitch_filter()";

        return rv;
    }
    int get(unsigned pGpio)
    {
        auto rv = gpio_read(gGpioHandle, pGpio);
        logger << logger::DEBUG << rv << "= get(" << pGpio << ")";
        return rv;
    }
    int set(unsigned pGpio, unsigned pLevel)
    {
        auto rv = gpio_write(gGpioHandle, pGpio, pLevel);
        logger << logger::DEBUG << rv << " = set(" << pGpio << ", " << pLevel << ")";
        return rv;
    }
    int registerCallback(unsigned pUserGpio, Edge pEdge, std::function<void(uint32_t tick)> pCb)
    {
        unsigned edge = 0;
        if (Edge::RISING == pEdge)
        {
            edge = RISING_EDGE;
        }
        else
        {
            edge = FALLING_EDGE;
        }

        fnCb[pUserGpio] = pCb;
        return callback(gGpioHandle, pUserGpio, edge, &Gpio::cb);
    }
    int deregisterCallback(int pCallbackId)
    {
        return callback_cancel(pCallbackId);
    }
private:
    static void cb(int pi, unsigned user_gpio, unsigned level, uint32_t tick)
    {
        Gpio::logger << logger::DEBUG << "CHANGED! PIN[" << user_gpio << "]=" << level;
        fnCb[user_gpio](tick);
    }
    static logger::Logger logger;
    static std::array<std::function<void(uint32_t tick)>, 20> fnCb;
};

logger::Logger Gpio::logger = logger::Logger("hwapi::Gpio");

std::array<std::function<void(uint32_t tick)>, 20> Gpio::fnCb = {};

std::shared_ptr<ISpi>  getSpi(uint8_t channel)
{
    return std::make_shared<Spi>(channel);
}

std::shared_ptr<IGpio> getGpio()
{
    return std::make_shared<Gpio>();
}

void setup()
{
    logger::Logger logger("hwapi::setup");
    char port[] = {'8','8','8','8',0};
    gGpioHandle = pigpio_start(nullptr, port);
    if (gGpioHandle<0)
    {
        throw std::runtime_error(std::string{} + "connecting to pigpiod failed! handle=" + std::to_string(gGpioHandle));
    }
    logger << logger::DEBUG << "gpiohandle="<<gGpioHandle;
}

void teardown()
{
    if (gGpioHandle>=0)
    {
        pigpio_stop(gGpioHandle);
    }
}

}