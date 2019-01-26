# PiLora
Semtech SX1278 LoRa daemon for RaspBerry Pi

Dependency:
* pigpio

Usage:
```
./pilora -h
Usage:

--channel N
                SPI Channel on Raspberry pi

--tx address
                Transmit Mode
                Address to open for tx data

--rx address
                Receive Mode
                Address to send rx data

--bandwidth N
                Bandwidth
                500 kHz
                250 kHz
                125 kHz

--coding-rate N
                Coding Rate
                4/5
                4/6
                4/7
                4/8


--spreading-factor N
                Spreading Factor
                SF6
                SF7
                SF8
                SF9
                SF10
                SF11
                SF12

--mtu N
                MTU Size (0 for variable size, max 255 bytes)

--tx-power N
                Power Amplifier
                0 to 14 dbm
--rx-gain N
                LNA Gain
                G1
                G2
                G3
                G4
                G5
                G6

```

Messages:
```
struct Header
{
    uint8_t msgId;
    uint8_t trId;
};

struct RxDataNotification
{
    Header hdr;
    uint8_t rssi;
    uint8_t size;
    uint8_t data[];
};

struct TxDataNotification
{
    Header hdr;
    uint8_t rssi;
    uint8_t size;
    uint8_t data[];
};

```