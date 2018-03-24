#ifndef DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_ASYNCREQSVRREQUEST_H_
#define DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_ASYNCREQSVRREQUEST_H_

#include <string>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

struct ReplyMessage {
public:
    ReplyMessage(): state_(PENDING), code(0) {}

    enum STATE {
        PENDING,
        IN_FLIGHT,
        COMPLETE,
        DISCONNECT,
        INVALID_URI,
        REJECTED
    };
    std::string content;
    std::string error;
    STATE       state_;
    int         code;
};

#include "websocketpp/client.hpp"
#include <websocketpp/config/asio_no_tls_client.hpp>

class AsyncReqSvrRequest {
public:
    AsyncReqSvrRequest(
            boost::asio::io_service& io_service,
            const std::string& uri,
            const std::string& reqName,
            const std::string& data);


    virtual ~AsyncReqSvrRequest();

    typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
    typedef websocketpp::client<websocketpp::config::asio_client> Client;
protected:

    void StartRequest();


    // Callback triggered Once we have the response
    virtual void OnComplete(ReplyMessage& reply) = 0;

    /**
     * Access to the message being read.
     *
     * Use before OnComplete is undefined, and stupid.
     */
    const ReplyMessage& GetMessage() { return reply_; }

private:
    // Call-back to notify of us a new message received
    void OnMessage(websocketpp::connection_hdl hdl, message_ptr msg);

     // Call-back to notify us of a successful connection
    void OnOpen(websocketpp::connection_hdl hdl);

     // Call-back to notify us of a closed connection
    void OnClose(websocketpp::connection_hdl hdl);

     // Call-back to notify us of a failure in the connection
    void OnFail(websocketpp::connection_hdl hdl);

    // Close the connection (if its currently open)
    void CloseConnection(
            int16_t reasonCode,
            std::string reason);

    Client client_;
    Client::connection_weak_ptr con_;

    ReplyMessage reply_;
    std::string uri_;
    std::string payload_;
};

#endif /* DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_ASYNCREQSVRREQUEST_H_ */
