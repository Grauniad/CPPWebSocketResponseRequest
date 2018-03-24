#ifndef DEV_TOOLS_CPP_LIBS_WEBSOCKETS_HTTP_REQUEST_H__
#define DEV_TOOLS_CPP_LIBS_WEBSOCKETS_HTTP_REQUEST_H__

#include <http_client.h>
#include <future>
#include <IPostable.h>
#include <mutex>

class HTTPMessageProvider { 
public:
    virtual const HTTPMessage& WaitForMessage() = 0;

    typedef std::function<void ()> Task;
    virtual void OnMessage(const Task& task) = 0;
};

class HTTPRequest: public HTTPMessageProvider, public AsyncHTTPSClient
{
public: 
    /**
     * Perform a standard GET request
     */
    HTTPRequest(boost::asio::io_service& io_service,
                const std::string& server,
                const std::string& path);

    ~HTTPRequest();

    /**
     * Perform a standard POST request
     */
    HTTPRequest(boost::asio::io_service& io_service,
                const std::string& server,
                const std::string& path,
                const std::string &data,
                const HeaderMap&  headers);

    /**
     * Waits for request to complete before returning the result
     */
    virtual const HTTPMessage& WaitForMessage();

    /**
     * Execute a task when the response has been received. If the ressponse has
     * already happened the task will be triggered immediately
     */
     virtual void OnMessage(const Task& notification);


private:
    virtual void OnComplete(HTTPMessage& msg);

    std::promise<int> statusFlag;
    std::future<int>  statusFuture;

    std::mutex           notifyMutex;

    // Only  modifiable under mutex lock 
    IPostable::Task      notification;
    bool                 ready;
    bool                 notify;
};

#endif
