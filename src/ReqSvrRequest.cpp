#include "ReqSvrRequest.h"
#include "logger.h"


std::shared_ptr<ReqSvrRequest> ReqSvrRequest::NewRequest(
    boost::asio::io_service& io_service,
    const std::string& uri,
    const std::string& reqName,
    const std::string& data)
{
    std::shared_ptr<ReqSvrRequest> request(
            new ReqSvrRequest(io_service, uri, reqName, data));

    request->keepAlive = request;

    request->StartRequest();

    return request;
}

ReqSvrRequest::ReqSvrRequest(
    boost::asio::io_service& service,
    const std::string& uri,
    const std::string& reqName,
    const std::string& data)
  : AsyncReqSvrRequest(service, uri, reqName, data)
  , statusFuture(statusFlag.get_future())
  , io_service(service)
{
}

ReqSvrRequest::~ReqSvrRequest() {
}

const ReplyMessage& ReqSvrRequest::WaitForMessage() {
    statusFuture.wait();
    const auto& response = GetMessage();
    switch(response.state_)
    {
    case ReplyMessage::COMPLETE:
    case ReplyMessage::REJECTED:
        // all good - safe to return
        break;
    case ReplyMessage::INVALID_URI:
        throw InvalidURIError{ response.error };
        break;
    case ReplyMessage::DISCONNECT:
        throw ServerDisconnectedError{ response.error };
        break;

    // These shouldn't happen, even in the network error case - they indicate a
    // programmatic error in the Async handler;
    case ReplyMessage::IN_FLIGHT:
    case ReplyMessage::PENDING:
        LOG_FROM(
            LOG_ERROR,
            "ReqSvrRequest::WaitForMessage",
            "ReqSvrRequest completed with a non terminal state!");
        throw ServerDisconnectedError{ response.error };
        break;
    }
    return GetMessage();
}

void ReqSvrRequest::OnComplete(ReplyMessage& reply) {
    statusFlag.set_value(1);
    ReleaseKeepAlive();
}

void ReqSvrRequest::ReleaseKeepAlive() {
    // Keep alive may be the only thing keeping the object alive, but we're in
    // the middle of a callback from the Client object.
    //
    // We therefore push the keepAlive token into a tasklet in the io_service's
    // queue. Sometime after this callback is complete the tasklet will be run
    // and the final reference released - safely killing the object when it is
    // not being used.
    //
    // Note that if for some reason the io_service is stopped, we'll be kept
    // alive until the service is reset, or destroyed. This should be perfectly
    // acceptable...
    std::shared_ptr<ReqSvrRequest> flushKeepAlive = std::move(keepAlive);
    io_service.post([flushKeepAlive] () -> void {
        // Nothing to do - io_service will now tidy the tasklet, releasing our
        // keepalive...
    });
}
