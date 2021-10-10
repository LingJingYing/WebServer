#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include "log.h"
#include "iomanager.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

int sock = 0;

void test_fiber() {
    LJY_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    //sleep(3);

    //close(sock);
    //sylar::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);//将字符串转换成数值型ip地址

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) { //代表连接还在进行中
        LJY_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        ljy::IOManager::GetCurrentScheduler()->addEvent(sock, ljy::IOManager::READ, [](){
            LJY_LOG_INFO(g_logger) << "read callback";
        });
        ljy::IOManager::GetCurrentScheduler()->addEvent(sock, ljy::IOManager::WRITE, [](){
            LJY_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            ljy::IOManager::GetCurrentScheduler()->cancelEvent(sock, ljy::IOManager::READ);
            close(sock);
        });
    } else {
        LJY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    ljy::IOManager iom(2, "test1");
    iom.schedule(&test_fiber);
}

ljy::Timer::ptr s_timer;
void test_timer() {
    ljy::IOManager iom(2, "test_timer");
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        LJY_LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 3) {
            s_timer->reset(2000, true);
        }
        if(i == 10){
            s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv) {
    test1();
    //test_timer();
    return 0;
}
