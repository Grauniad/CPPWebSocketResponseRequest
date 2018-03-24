#ifndef DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQSVRREQUEST_H_
#define DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQSVRREQUEST_H_

#include <future>
#include "AsyncReqSvrRequest.h"

class ReqSvrRequest: public AsyncReqSvrRequest {
public:
    /**
     * Initiate a new request on the IO thread (io_service), and return a handle
     * to the access the result.
     *
     * If the result is not interesting, the handle may be safely discarded -
     * the request will be kept alive until the network communication has been
     * resolved.
     *
     * The result of the request may be access via the blocking WaitForMessage
     * method.
     */
    static std::shared_ptr<ReqSvrRequest> NewRequest(
        boost::asio::io_service& io_service,
        const std::string& uri,
        const std::string& reqName,
        const std::string& data);

    virtual ~ReqSvrRequest();

    /**
     * Waits for request to complete before returning the result
     *
     * In the event of a failure an instance ReqSvrRequestError will be thrown
     */
    virtual const ReplyMessage& WaitForMessage();

    struct ReqSvrRequestError {
        ReqSvrRequestError(std::string msg): msg_(std::move(msg)) {}
        std::string msg_;
    };

    template <ReplyMessage::STATE state>
    struct StateSpecificError: public ReqSvrRequestError {
        StateSpecificError(std::string msg): ReqSvrRequestError(std::move(msg)) {}
    };
    using InvalidURIError = StateSpecificError<ReplyMessage::INVALID_URI>;
    using ServerDisconnectedError = StateSpecificError<ReplyMessage::DISCONNECT>;

protected:
    // Callback triggered Once we have the response
    virtual void OnComplete(ReplyMessage& reply) override;

    std::promise<int> statusFlag;
    std::future<int>  statusFuture;
    boost::asio::io_service& io_service;

private:
    // Life cyle management - see comments in FactoryMethod and ReleaseKeepAlive
    // bodies.
    void ReleaseKeepAlive();
    ReqSvrRequest(
        boost::asio::io_service& io_service,
        const std::string& uri,
        const std::string& reqName,
        const std::string& data);

    std::shared_ptr<ReqSvrRequest> keepAlive;
};

#endif /* DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQSVRREQUEST_H_ */
