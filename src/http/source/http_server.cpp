#include "http_server.h"
#include "log.h"
//#include "sylar/http/servlets/config_servlet.h"
//#include "sylar/http/servlets/status_servlet.h"

namespace ljy {
namespace http {

static ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive
               ,ljy::IOManager* worker
               ,ljy::IOManager* io_worker
               ,ljy::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker)
    ,m_isKeepalive(keepalive) {
    //m_dispatch.reset(new ServletDispatch);

    m_type = "http";
    //m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    //m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}
/*
void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}
*/
void HttpServer::handleClient(Socket::ptr client) {
    LJY_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        LJY_LOG_DEBUG(g_logger) << "recvRequest";
        auto req = session->recvRequest();
        if(!req) {
            LJY_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }
        LJY_LOG_DEBUG(g_logger) << "rev client " << *req;
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        //rsp->setHeader("Server", getName());
        //m_dispatch->handle(req, rsp, session);
        rsp->setBody("ljy");
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close();
}

}
}
