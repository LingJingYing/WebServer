#include "http_server.h"
#include "log.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void run(){
    g_logger->setLevel(ljy::LogLevel::INFO);
    ljy::Address::ptr addr = ljy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr){
        LJY_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    ljy::http::HttpServer::ptr http_server(new ljy::http::HttpServer(true));
    while(!http_server->bind(addr)){
        LJY_LOG_ERROR(g_logger) << "bind " << addr << " failed!";
        sleep(1);
    }

    http_server->start();

}

int main(int argc, char** argv){
    ljy::IOManager iom(8, "pretest");
    iom.schedule(run);
    return 0;
}