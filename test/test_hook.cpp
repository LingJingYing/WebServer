#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "hook.h"
#include "log.h"
#include "iomanager.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void test_sleep() {
    ljy::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        LJY_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        LJY_LOG_INFO(g_logger) << "sleep 3";
    });
    LJY_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "180.101.49.12", &addr.sin_addr.s_addr);

    LJY_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    LJY_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    LJY_LOG_INFO(g_logger) << "send";
    rt = send(sock, data, sizeof(data), 0);
    LJY_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    char *buff = new char[40960];
    memset(buff, 0, 40960);
    //buff.resize(4096);

    while(0 < (rt = recv(sock, buff, 40960, 0))){
        LJY_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

        //buff.resize(rt);
        printf("%s", buff);
        memset(buff, 0, 40960);
    }
    printf("\n");
    delete[] buff;

}


int main(int argc, char** argv) {
    //test_sleep();
    ljy::IOManager iom(1, "test");
    iom.schedule(test_sock);
    return 0;
}
