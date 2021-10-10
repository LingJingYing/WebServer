#include "socket.h"
#include "iomanager.h"
#include "log.h"
#include "string.h"
#include "hook.h"

static ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void test_socket() {
    //std::vector<sylar::Address::ptr> addrs;
    //sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //sylar::IPAddress::ptr addr;
    //for(auto& i : addrs) {
    //    SYLAR_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<sylar::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}
    
    ljy::IPAddress::ptr addr = ljy::Address::LookupAnyIPAddress("0.0.0.0");
    if(addr) {
        LJY_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LJY_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    ljy::Socket::ptr sock = ljy::Socket::CreateTCP(addr);
    addr->setPort(8020);
    LJY_LOG_INFO(g_logger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        LJY_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        LJY_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        LJY_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    char *buf = new char[40960];
    memset(buf, 0, 40960);

    while(0 < (rt = sock->recv(buf, 40960))){
        LJY_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

        //buff.resize(rt);
        printf("%s", buf);
        memset(buf, 0, 40960);
    }
    printf("\n");

    delete[] buf;
}

void test2() {
    ljy::IPAddress::ptr addr = ljy::IPAddress::LookupAnyIPAddress("www.baidu.com:80");
    if(addr) {
        LJY_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LJY_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    ljy::Socket::ptr sock = ljy::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        LJY_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        LJY_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = ljy::GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getError()) {
            LJY_LOG_INFO(g_logger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    SYLAR_LOG_INFO(g_looger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    SYLAR_LOG_INFO(g_looger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = ljy::GetCurrentUS();
            LJY_LOG_INFO(g_logger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main(int argc, char** argv) {
    ljy::IOManager iom;
    iom.schedule(&test_socket);
    //iom.schedule(&test2);
    return 0;
}
