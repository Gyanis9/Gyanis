#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "core/IOManager.h"
#include "base/Log.h"


using namespace Gyanis::base;
using namespace Gyanis::core;

auto g_logger = LOG_ROOT();

int sock = 0;

void test_fiber()
{
    LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "183.2.172.17", &addr.sin_addr.s_addr);

    if (!connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        IOManager::GetThis()->addEvent(sock, IOManager::READ, []()
        {
            LOG_INFO(g_logger) << "read callback";
        });
        IOManager::GetThis()->addEvent(sock, IOManager::WRITE, []()
        {
            LOG_INFO(g_logger) << "write callback";
            IOManager::GetThis()->cancelEvent(sock, IOManager::READ);
            close(sock);
        });
    }
    else
    {
        LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1()
{
    std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
    IOManager iom(2);
    iom.schedule(&test_fiber);
}


void test_timer()
{
    const auto iom = std::make_shared<IOManager>(2);
    const uint64_t id = iom->addTimer(1000, [&]()
    {
        static int i = 0;
        LOG_INFO(g_logger) << "hello timer i=" << i;
        if (++i == 3)
        {
            iom->reset(id, 2000, true);
        }
    }, true);
    LOG_INFO(g_logger) << "id: " << id;
}

int main()
{
    // test1();
    test_timer();
}
