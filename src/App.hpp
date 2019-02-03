#ifndef __APP_HPP__
#define __APP_HPP__

#include <IHwApi.hpp>
#include <Logger.hpp>

namespace app
{
class App
{
public:
    using Options = std::map<std::string, std::string>;
    // App(IUdpSockFactory& pUdpFactory)
    App(const Options& pOptions)
        // : mUdpFactory(mUdpFactory,)
        : mLogger("App")
    {
        mLogger << logger::DEBUG << "App::App";
    }

    int run()
    {
        mLogger << logger::DEBUG << "App::run";
        return 0;
    }

private:
    logger::Logger mLogger;
};

}
#endif //__APP_HPP__