# PiLora
Semtech SX1278 LoRa daemon for RaspBerry Pi

Dependency:
* pigpio

Usage:
```
./pilora -h
Usage:

--channel=N
                SPI Channel on Raspberry Pi
                Mandatory

--cx=address
                Link Control
                UDP address to open for link control
                Default value 0.0.0.0:2221
--tx=address
                Transmit Mode
                Address to open for tx data
                Required if not rx

--rx=address
                Receive Mode
                Address to send rx data
                Required if not tx

--bandwidth=N
                Bandwidth {500 kHz, 250 kHz, 125 kHz}
                Default: 500Khz

--coding-rate=N
                Coding Rate {4/5, 4/6, 4/7, 4/8}
                Default: 4/5

--spreading-factor=N
                Spreading Factor {SF6, SF7, SF8, SF9, SF10, SF11, SF12}
                Default: SF7

--mtu=N
                MTU Size (0 for variable size, max 255 bytes)
                Default: 0

--tx-power=N
                Power Amplifier
                0 to 14dBm
                Default: 14dBm

--rx-gain=N
                LNA Gain {G1, G2, G3, G4, G5, G6}
                G1 Is the highest
                Default: G1

```

Control Messages:
```
struct Header
{
    uint8_t msgId;
    uint8_t trId;
};

struct ReconfigurationRequest
{
    Header hdr;
    uint8_t bandwidth;
    uint8_t codingRate;
    uint8_t spreadingFactor;
    uint8_t mtuSize;
    uint8_t txPower;
    uint8_t rxGain;
};

struct ReconfigurationResponse
{
    Header hdr;
    uint8_t status;
};

```

Building:
```
# Generating Makefile:
mkdir build
cd build
../configure.py

# Building test
cd build
make test

# Building stubbed target
cd build
make binstub

# Building pigpio target
cd build
make binpigpio
```

Stubbed Target:
PiLoRa can still be tested without Raspberry Pi using the stubbed target.
Stubbed target will open udp socket at 0.0.0.0:8888 for rx and will send upd packet to localhost:8881 for tx

Pigpio Target:
PiLoRa is linked with pigpio. PiLoRa will use the SPI and GPIO in pigpio.