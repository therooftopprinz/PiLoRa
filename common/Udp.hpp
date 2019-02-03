#ifndef __UDP_HPP__
#define __UDP_HPP__

#include <sys/socket.h>
#include <Buffer.hpp>

namespace socket
{

struct IpPort
{
    uint32_t addr;
    uint16_t port;
};

struct ISocket
{
    virtual int bind(const IpPort& pAddr) = 0;
    virtual ssize_t sendto(const common::Buffer& pData, const IpPort& pAddr, int flags) = 0;
    virtual ssize_t recvfrom(const common::Buffer& pData, const IpPort& pAddr, int flags) = 0;
};

struct IUdpFactory
{
    virtual ISocket create() = 0;
};

}

#endif // __UDP_HPP__