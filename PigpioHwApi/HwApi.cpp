#include <IHwApi.hpp>
#include <exception>
#include <pigpiod_if2.h>
#include <Logger.hpp>

namespace hwapi
{

static int gGpioHandle = -1;

class Spi : public ISpi
{
public:
    Spi(uint8_t pChannel)
    {
        logger << logger::DEBUG << "opening spi channel="<< unsigned(pChannel) << " for gpio=" << gGpioHandle;
        mHandle  = spi_open(gGpioHandle, pChannel, 57600,
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
        return spi_xfer(gGpioHandle, mHandle, (char*)dataOut, (char*)dataIn, count);
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
        return set_mode(gGpioHandle, pGpio, pinmode);
    }
    int get(unsigned pGpio)
    {
        return gpio_read(gGpioHandle, pGpio);
    }
    int set(unsigned pGpio, unsigned pLevel)
    {
        return gpio_write(gGpioHandle, pGpio, pLevel);
    }
    int registerCallback(unsigned pUserGpio, Edge pEdge, std::function<void(uint32_t tick)> pCb)
    {
        unsigned edge = 0;
        if (Edge::RISING == pEdge)
        {
            edge = 1;
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
        fnCb[user_gpio](tick);
    }
    static std::array<std::function<void(uint32_t tick)>, 20> fnCb;
};

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
    char port[] = {'7','7','7','7',0};
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