#include "http_request.h"
#include <logger.h>

HTTPRequest::HTTPRequest(boost::asio::io_service& io_service,
                         const std::string& server,
                         const std::string& path)
   : AsyncHTTPSClient(REQUEST_MODE_GET, io_service, server, path, {}, ""),
     statusFuture(statusFlag.get_future()),
     ready(false),
     notify(false)
{
    StartRequest();
}

HTTPRequest::~HTTPRequest() {
    WaitForMessage();
}

HTTPRequest::HTTPRequest(boost::asio::io_service& io_service,
                         const std::string& server,
                         const std::string& path,
                         const std::string& data,
                         const HeaderMap& headers)
   : AsyncHTTPSClient(REQUEST_MODE_POST, io_service, server, path, headers, data)
   , statusFuture(statusFlag.get_future())
   , ready(false)
   , notify(false)
{
    StartRequest();
}

const HTTPMessage& HTTPRequest::WaitForMessage() {
    statusFuture.wait();
    return GetMessage();
}

void HTTPRequest::OnComplete(HTTPMessage& msg) {
    SLOG_FROM(LOG_VERBOSE,"HTTPRequest::OnComplete",
         "Got response: " << msg.content.str());

    std::unique_lock<std::mutex> notify_lock(notifyMutex);

    ready = true;


    statusFlag.set_value(msg.status_code);

    if (notify) {
        notification();
    }
}

void HTTPRequest::OnMessage(const Task& _notification)

{
    std::unique_lock<std::mutex> notify_lock(notifyMutex);

    notify = true;
    notification = _notification;

    if (ready) {
        notification();
    }

}
