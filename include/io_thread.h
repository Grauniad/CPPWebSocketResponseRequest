#ifndef DEV_TOOLS_CPP_LIBS_WEBSOCKETS_IO_THREAD_H__
#define DEV_TOOLS_CPP_LIBS_WEBSOCKETS_IO_THREAD_H__

#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <IPostable.h>

#include "ReqSvrRequest.h"
#include "WebPPSingleThreadOneShotClient.h"

/**
 * Spawns a new IO thread, which can be used to create
 * new requests
 */
class IOThread: public IPostable {
public:
    /**
     * Start the new thread
     */ 
    IOThread();

    ~IOThread() override;

    /**
     * Request content via a one-short request to a ReqServer
     */
    std::shared_ptr<ReqSvrRequest> Request(
            const std::string& uri,
            const std::string& requestName,
            const std::string& jsonData);


    void PostTask(const Task& t) override;
private:
    void IOLoop();

    /**
     * Stop the thread. 
     *
     * This function call will block until the IO thread as been
     * terminated.
     */
    void Stop();

    boost::asio::io_service io_service;
    std::thread io_thread;
    WebPPSingleThreadOneShotClient requestClient;
};

#endif
