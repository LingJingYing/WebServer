#include "http_server.h"
#include "log.h"

static ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

#define XX(...) #__VA_ARGS__


//ljy::IOManager::ptr worker;
void run() {
    //g_logger->setLevel(ljy::LogLevel::INFO);
    //sylar::http::HttpServer::ptr server(new sylar::http::HttpServer(true, worker.get(), sylar::IOManager::GetThis()));
    ljy::http::HttpServer::ptr server(new ljy::http::HttpServer(true));
    ljy::Address::ptr addr = ljy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
#if 1
    auto sd = server->getServletDispatch();
    sd->addServlet("/ljy/xx", [](ljy::http::HttpRequest::ptr req
                ,ljy::http::HttpResponse::ptr rsp
                ,ljy::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/ljy/*", [](ljy::http::HttpRequest::ptr req
                ,ljy::http::HttpResponse::ptr rsp
                ,ljy::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/ljyx/*", [](ljy::http::HttpRequest::ptr req
                ,ljy::http::HttpResponse::ptr rsp
                ,ljy::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });
#endif
    server->start();
}

int main(int argc, char** argv) {
    ljy::IOManager iom(2, "main");
    //worker.reset(new ljy::IOManager(3, "worker"));
    iom.schedule(run);
    return 0;
}
