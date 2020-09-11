#ifndef DEV_TOOLS_CPP_LIBS_WEBSOCKETS_STREAM_CLIENT_H__
#define DEV_TOOLS_CPP_LIBS_WEBSOCKETS_STREAM_CLIENT_H__

#include <atomic>
#include <thread>
#include <future>
#include <CPPWSRRWraper.h>

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

    void Ping(unsigned int timeout);

    struct InvalidUrlException {};

    /**
     * Call-back triggered when a new message is received.
     *
     * NOTE: This call-back is triggered from the IO thread.
     */
    virtual void OnMessage(const std::string& msg) = 0;

    // TODO: Make protected again
    void StopNonBlock();

protected: 
    /**
     * Run the event loop - (start the query)
     */
    void Run();

    /**
     * Stop the event loop
     */
    void Stop();

private:
    std::atomic<bool>   running;
    ASIOClient          m_endpoint;
    ASIOClient::ConnPtr con;
    std::string         subName;
    std::string         subBody;
    std::string         hello;
    std::string         url;
    std::promise<bool>  startFlag;
    std::future<bool>   futureStart;
    std::promise<bool>  stopFlag;
    std::future<bool>   futureStop;

};

#endif
