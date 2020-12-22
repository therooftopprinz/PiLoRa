#include <signal.h>
#include <iostream>
#include <memory>
#include <regex>
#include <logless/Logger.hpp>
#include <SX1278.hpp>
#include <hwapi/HwApi.hpp>
#include <App.hpp>

void sig_handler(int signum)
{
    Logger::getInstance().flush();
    exit(1);
}

int main(int argc, const char* argv[])
{
    signal(SIGINT, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGABRT, sig_handler);

    std::regex arger("^--(.+?)=(.+?)$");
    std::smatch match;
    std::map<std::string, std::string> options; 

    // Logger::getInstance().logful();

    for (int i=1; i<argc; i++)
    {
        auto s = std::string(argv[i]);
        if (std::regex_match(s, match, arger))
        {
            options.emplace(match[1].str(), match[2].str());
        }
        else
        {
            throw std::runtime_error(std::string("invalid argument: `") + argv[i] + "`");
        }
    }

    std::unique_ptr<bfc::IUdpFactory> udpFactory = std::make_unique<bfc::UdpFactory>();
    app::Args args(options);
    hwapi::setup();
    app::App app(*udpFactory, args);
    return app.run();
}