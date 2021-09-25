#include "http_session.h"
#include "http_parser.h"
#include "log.h"

namespace ljy {

static ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");

namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) {
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        LJY_LOG_DEBUG(g_logger) << "len=" << len;
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t parseLen = parser->execute(data, len);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - parseLen;//还未解析的长度
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    //开始读取消息体
    int64_t length = parser->getContentLength();
    LJY_LOG_DEBUG(g_logger) << "ContentLength=" << length;
    if(length > 0) {
        std::string body;
        body.resize(length);
        //先读取剩余数据，读取消息头时剩下的
        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if(length > 0) {//再从socket中读取剩下的消息数据
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }

    parser->getData()->init();
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}
