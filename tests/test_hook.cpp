#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "core/IOManager.h"
#include "base/Log.h"

using namespace Gyanis::base;
using namespace Gyanis::core;


auto g_logger = LOG_ROOT();

void test_sleep()
{
    IOManager iom(1);
    iom.schedule([]()
    {
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([]()
    {
        sleep(2);
        LOG_INFO(g_logger) << "sleep 3";
    });
    LOG_INFO(g_logger) << "test_sleep";
}


void test_sock()
{
    const int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "183.2.172.177", &addr.sin_addr.s_addr);

    LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if (rt)
    {
        return;
    }

    constexpr char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if (rt <= 0)
    {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if (rt <= 0)
    {
        return;
    }

    buff.resize(rt);
    LOG_INFO(g_logger) << buff;
}


int main()
{
    // test_sleep();
    const auto iom = std::make_shared<IOManager>();
    iom->schedule(test_sock);
    return 0;
}
