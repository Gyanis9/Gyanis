#include "net/http/HttpServer.h"
#include "base/Log.h"

auto g_logger = LOG_ROOT();
std::shared_ptr<Gyanis::core::IOManager> worker;

void run()
{
    const auto address = Gyanis::net::Address::LookupAnyIPAddress("0.0.0.0:8090");
    if (!address)
    {
        LOG_ERROR(g_logger) << "get address error";
        return;
    }
    const auto http_server = std::make_shared<Gyanis::net::http::HttpServer>(false, worker.get());
    constexpr bool ssl = false;
    while (!http_server->bind(address, ssl))
    {
        LOG_ERROR(g_logger) << "bind " << *address << " fail";
        sleep(1);
    }

    if (ssl)
    {
    }

    http_server->start();
}

int main(int argc, char** argv)
{
    Gyanis::core::IOManager iom(1);
    worker = std::make_shared<Gyanis::core::IOManager>(12);
    iom.schedule(run);
    return 0;
}
