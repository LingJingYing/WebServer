#include "socket.h"
#include "iomanager.h"
#include "log.h"
#include "string.h"
#include "hook.h"
#include "http.h"

static ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void test_socket() {
    
    ljy::IPAddress::ptr addr = ljy::Address::LookupAnyIPAddress("0.0.0.0");
    if(addr) {
        LJY_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LJY_LOG_ERROR(g_logger) << "get address fail";
        return;
    }



    const char buff[] = 
                "GET / HTTP/1.1\r\n"
                "connection: keep-alive\r\n"
                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
                "Cache-Control: max-age=0\r\n"
                "Host: 127.0.0.1:8020\r\n"
                "Sec-Fetch-Dest: document\r\n"
                "Sec-Fetch-Mode: navigate\r\n"
                "Sec-Fetch-Site: none\r\n"
                "Sec-Fetch-User: ?1\r\n"
                "Upgrade-Insecure-Requests: 1\r\n"
                "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:92.0) Gecko/20100101 Firefox/92.0\r\n\r\n";
    LJY_LOG_DEBUG(g_logger) << buff;
    ljy::Socket::ptr sock = ljy::Socket::CreateTCP(addr);
    addr->setPort(8020);
    LJY_LOG_INFO(g_logger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        LJY_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        LJY_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        LJY_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    char *buf = new char[40960];
    memset(buf, 0, 40960);
//while(true){
    while(0 < (rt = sock->recv(buf, 40960))){
        LJY_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

        //buff.resize(rt);
        printf("%s", buf);
        memset(buf, 0, 40960);
    }
    printf("\n");
//}
sleep(60);
    delete[] buf;
}


int main(int argc, char** argv) {
    ljy::IOManager iom;
    iom.schedule(&test_socket);
    //iom.schedule(&test2);
    return 0;
}
