#include "http.h"
#include "log.h"

void test_request() {
    ljy::http::HttpRequest::ptr req(new ljy::http::HttpRequest);
    req->setHeader("host" , "www.sylar.top");
    req->setBody("hello sylar");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    ljy::http::HttpResponse::ptr rsp(new ljy::http::HttpResponse);
    rsp->setHeader("X-X", "sylar");
    rsp->setBody("hello sylar");
    rsp->setStatus((ljy::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
