#include <iostream>
#include <memory>
#include <regex>
#include <SX1278.hpp>
#include <IHwApi.hpp>
#include <App.hpp>

int main(int argc, const char* argv[])
{
    logger::Logger log("main");
    log << logger::DEBUG << "App::App";
    
    std::regex arger("^--(.+?)=(.+?)$");
    std::smatch match;
    std::map<std::string, std::string> options; 

    for (int i=1; i<argc; i++)
    {
        auto s = std::string(argv[i]);
        if (std::regex_match(s, match, arger))
        {
            log << logger::DEBUG << "key:" << match[1].str() << " value:" << match[2].str();
            options.emplace(match[1].str(), match[2].str());
        }
        else
        {
            log << logger::DEBUG << "match failed";
            throw std::runtime_error(std::string("invalid argument: `") + argv[i] + "`");
        }
    }

    std::unique_ptr<net::IUdpFactory> udpFactory;
    app::Args args(options);
    app::App app(*udpFactory, args);
    auto rv = app.run();
    logger::LoggerServer::getInstance().waitEmpty();
    return rv;
}