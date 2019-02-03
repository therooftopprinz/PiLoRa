#include <SX1278.hpp>
#include <IHwApi.hpp>
#include <iostream>
#include <regex>
#include <App.hpp>

int main(int argc, const char* argv[])
{
    logger::Logger log("main");
    log << logger::DEBUG << "App::App";

    std::cout << "argc: " << argc << "\n";
    
    std::regex arger("^--(.+?)=(.+?)$");
    std::smatch match;
    std::map<std::string, std::string> options; 

    for (int i=1; i<argc; i++)
    {
        auto s = std::string(argv[i]);
        if (std::regex_match(s, match, arger))
        {
            std::cout << "key:" << match[1].str()  << " value:" << match[2].str() << "\n";
            options.emplace(match[1].str(), match[2].str());
        }
        else
        {
            std::cout << "match failed\n";
            throw std::runtime_error(std::string("invalid argument: `") + argv[i] + "`");
        }
    }

    app::App app(options);
    auto rv = app.run();
    logger::LoggerServer::getInstance().waitEmpty();
    return rv;
}