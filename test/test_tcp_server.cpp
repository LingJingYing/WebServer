#include "tcp_server.h"
#include "iomanager.h"
#include "log.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void run() {
    auto addr = ljy::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = sylar::UnixAddress::ptr(new sylar::UnixAddress("/tmp/unix_addr"));
    std::vector<ljy::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    ljy::TcpServer::ptr tcp_server(new ljy::TcpServer);
    std::vector<ljy::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    ljy::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
