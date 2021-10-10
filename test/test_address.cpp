#include "address.h"
#include "log.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void test() {
    std::vector<ljy::Address::ptr> addrs;

    LJY_LOG_INFO(g_logger) << "begin";
    //bool v = ljy::Address::Lookup(addrs, "localhost:3090");
    
    bool v = ljy::Address::Lookup(addrs, "www.baidu.com", AF_INET);

    LJY_LOG_INFO(g_logger) << "end";
    if(!v) {
        LJY_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    /*
    0 - 1.0.0.127:4620 SOCK_STREAM TCP
    1 - 1.0.0.127:4620 SOCK_DGRAM  UDP
    2 - 1.0.0.127:4620 SOCK_RAW    原始套接字
    */
    for(size_t i = 0; i < addrs.size(); ++i) {
        LJY_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();//打印的是网络序
    }

    auto addr = ljy::Address::LookupAny("localhost:4080");
    if(addr) {
        LJY_LOG_INFO(g_logger) << *addr;
    } else {
        LJY_LOG_ERROR(g_logger) << "error";
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<ljy::Address::ptr, uint32_t> > results;

    bool v = ljy::Address::GetInterfaceAddresses(results);
    if(!v) {
        LJY_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        LJY_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = sylar::IPAddress::Create("www.sylar.top");
    auto addr = ljy::IPAddress::Create("127.0.0.8");
    if(addr) {
        LJY_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    //test_ipv4();
    //test_iface();
    test();
    return 0;
}
