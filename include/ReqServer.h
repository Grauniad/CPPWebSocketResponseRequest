#ifndef __DEV_TOOLS_CPP_LIBRARIES_WEBSOCKETS_REQ_SERVER_H__
#define __DEV_TOOLS_CPP_LIBRARIES_WEBSOCKETS_REQ_SERVER_H__

#include <future>
#include <map>
#include <memory>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <IPostable.h>

class RequestReplyHandler {
public:
    virtual ~RequestReplyHandler() {}

    friend class RequestServer;
    virtual std::string OnRequest(const char* request) = 0;

    struct InvalidRequestException {
        int code;
        std::string errMsg;
    };

    // Unrecoverable error - terminate the server
    struct FatalError {
        int code;
        std::string errMsg;
    };
};


class SubscriptionHandler {
public:
    virtual ~SubscriptionHandler() {}

    class SubRequest {
    public:
        virtual void SendMessage(const std::string& msg) = 0;

        virtual const char* RequestMessasge() = 0;

        /**
         * Returns true if the subscription is still active, and can be written
         * to.
         *
         * Valid ONLY on the original request thread.
         */
        virtual bool Ok() const = 0;

        /**
         * Callback to notify that this request is no longer active...
         */
        virtual void Close() = 0;

    };

    typedef std::shared_ptr<SubRequest> RequestHandle;

    struct InvalidRequestException {
        int code;
        std::string errMsg;
    };

    virtual void OnRequest(RequestHandle hdl) = 0;
};

typedef websocketpp::server<websocketpp::config::asio> Server;

/**
 *  The request server 
 */
class RequestServer: public IPostable {
public:
    /**************************************************************
     *                Initialisation Interface
     **************************************************************/
    /**
     * Construct the request server.
     */
    RequestServer();

    virtual ~RequestServer();

    /**
     * Install a new handler, which will be availabel when HandleRequests is
     * called
     */
    void AddHandler(
             const std::string& requestName, 
             std::unique_ptr<RequestReplyHandler> handler);

    void AddHandler(
             const std::string& requestName, 
             std::unique_ptr<SubscriptionHandler> handler);

    /**
     * Run the event loop, handle any incoming requests or posted tasks
     */
    void HandleRequests(unsigned short port);

    /**************************************************************
     *                Event Loop Interface
     **************************************************************/

    /**
     * Queue a task to be performed on the event loop thread.
     */
    typedef IPostable::Task InteruptHandler;
    void PostTask(const InteruptHandler& f);

    /**************************************************************
     *                Utility Methods
     **************************************************************/

    std::string HandleMessage(
         const std::string& request,
         Server*  raw_server,
         websocketpp::connection_hdl hdl);

    /**
     * Blocks until the request server's is up, and the event loop is running.
     */
    void WaitUntilRunning();


    /**
     * Handle an on_close even on an active connection.
     */
    void HandleClose(websocketpp::connection_hdl hdl);

    struct FatalErrorException {
        int code;
        std::string message;
    };

    // Unrecoverable error has occurred - close down the server as gracefully as
    // we can and, then throw an exception out of HandleRequests
    void FatalError(int code, std::string message);
private:
    /**
     * Bail out of the event loop...
     */
    void Stop();

    std::string HandleRequestReplyMessage(
                    const std::string& reqName,
                    const std::string& request,
                    RequestReplyHandler& handler);

    std::string HandleSubscriptionMessage(
                    const std::string& reqName,
                    const std::string& request,
                    SubscriptionHandler& handler,
                    Server*  raw_server,
                    websocketpp::connection_hdl hdl);


    Server requestServer_;
    std::promise<bool> runningFlag;
    std::future<bool>  running;
    std::promise<bool> stopFlag;
    std::future<bool>  stopped;

    std::map<std::string,std::unique_ptr<RequestReplyHandler>> req_handlers;
    std::map<std::string,std::unique_ptr<SubscriptionHandler>> sub_handlers;

    /*
     * Maps an active connection to
     */
    std::map<void*,SubscriptionHandler::RequestHandle> conn_map;

    // Error tracking
    bool        failed;
    int         failCode;
    std::string failMsg;
};


#endif
