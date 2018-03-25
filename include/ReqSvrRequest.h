#ifndef DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQSVRREQUEST_H_
#define DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQSVRREQUEST_H_

#include <future>
#include "IOneShotConnectionConsumer.h"

// TODO: Once we've done the initial migration, move this out to
//       somewhere less in the way.
struct ReplyMessage {
public:
    ReplyMessage(): state_(PENDING), code_(0) {}
    enum STATE {
        PENDING,
        IN_FLIGHT,
        COMPLETE,
        DISCONNECT,
        INVALID_URI,
        REJECTED
    };
    std::string content_;
    std::string error_;
    STATE       state_;
    int         code_;
};

class ReqSvrRequest: public IOneShotConnectionConsumer {
public:
    /**
     * Configure a new request.
     *
     * NOTE: To actually trigger the request this configuration will
     *       need to be provided to some kind of client. (Usually the
     *       WebPPSingleThreadOneShotClient
     *
     * @param reqName   The name of the request to send
     * @param data      The payload (usually the request JSON)
     */
    ReqSvrRequest (const std::string& reqName, const std::string& data);

    virtual ~ReqSvrRequest();


    /**
     * Waits (blocks the current thread) for request to complete before
     * returning the result
     *
     * In the event of a failure an instance ReqSvrRequestError will be thrown
     *
     * NOTE: If the request configuration is not passed to come kind of client
     *       to actually trigger the request, this will block forever.
     */
    virtual const ReplyMessage& WaitForMessage();

    // TODO: Tidy these errors up...
    struct ReqSvrRequestError {
        ReqSvrRequestError(std::string msg);
        std::string msg_;
    };

    template <ReplyMessage::STATE state>
    struct StateSpecificError: public ReqSvrRequestError {
        StateSpecificError(std::string msg): ReqSvrRequestError(std::move(msg)) {}
    };
    using InvalidURIError = StateSpecificError<ReplyMessage::INVALID_URI>;
    using ServerDisconnectedError = StateSpecificError<ReplyMessage::DISCONNECT>;

protected:
    std::promise<int> statusFlag;
    std::future<int>  statusFuture;
    ReplyMessage      message;

private:
    // Interface required by the IOneShotConnectionConsumer
    const std::string& onOpen () override;

    void onInvalidRequest (std::string err) override;

    void onTerminate () override;

    void onComplete () override;

    void onMessage (std::string) override;

    // Data
    std::string payload;

};

#endif /* DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQSVRREQUEST_H_ */
