#if 0
#ifndef __LJY_CONFIG_SERVLET_H__
#define __LJY_CONFIG_SERVLET_H__

#include "servlet.h"

namespace ljy {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(ljy::http::HttpRequest::ptr request
                   , ljy::http::HttpResponse::ptr response
                   , ljy::http::HttpSession::ptr session) override;
};

}
}

#endif
#endif