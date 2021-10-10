#if 0
#ifndef __LJY_STATUS_SERVLET_H__
#define __LJY_STATUS_SERVLET_H__

#include "servlet.h"

namespace ljy {
namespace http {

class StatusServlet : public Servlet {
public:
    StatusServlet();
    virtual int32_t handle(ljy::http::HttpRequest::ptr request
                   , ljy::http::HttpResponse::ptr response
                   , ljy::http::HttpSession::ptr session) override;
};

}
}

#endif
#endif