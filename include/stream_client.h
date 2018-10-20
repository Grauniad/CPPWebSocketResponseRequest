#ifndef DEV_TOOLS_CPP_LIBS_WEBSOCKETS_STREAM_CLIENT_H__
#define DEV_TOOLS_CPP_LIBS_WEBSOCKETS_STREAM_CLIENT_H__

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>
#include <atomic>
#include <thread>
#include <future>

/**
 * Subscribes to a websocket and triggers a call-back for each message.
 *
 * The run function starts its own event loop, and would ordinarily
 * be called from a dedicated worker thread.
 */
class StreamClient {
public:
    /**
     * C'tor - variant for connecting to a SubsriptionServer
     *
     * connect to the specified url, and logon with the specified request and message
     */
    StreamClient(
        std::string url,
        std::string subName,
        std::string subBody
        );

    virtual ~StreamClient();

    /**
     * Indicates if the query is currently live
     */
    bool Running();

    /**
     * Wait until the server is running...
     */
    bool WaitUntilRunning();

protected: 
    /**
     * Run the event loop - (start the query)
     */
    void Run();

    /**
     * Stop the event loop
     */
    void Stop();
    
    /**
     * Call-back triggered when a new message is received.
     *
     * NOTE: This call-back is triggered from the IO thread.
     */
    virtual void OnMessage(const std::string& msg) = 0;

private:
    typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;


    /**
     * Call-back to notify us of a successful connection
     */
    void on_open(websocketpp::connection_hdl hdl);

    /**
     * Call-back to notify us of a successful subscription connection
     */
    void on_sub_open(websocketpp::connection_hdl hdl);

    /**
     * Call-back triggered by the arrival of a new message
     */
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg);

    typedef websocketpp::client<websocketpp::config::asio_client> client;

    std::atomic<bool>   running;
    client m_endpoint;
    client::connection_ptr con;
    std::string        subName;
    std::string        subBody;
    std::string        url;
    std::promise<bool> startFlag;
    std::future<bool>  futureStart;
    std::promise<bool> stopFlag;
    std::future<bool>  futureStop;

};

#endif
