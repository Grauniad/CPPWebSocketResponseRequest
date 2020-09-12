//
// Created by lhumphreys on 11/09/2020.
//

#ifndef CPPWEBSOCKETRESPONSEREQUEST_WRAPER_H
#define CPPWEBSOCKETRESPONSEREQUEST_WRAPER_H

#include <memory>

struct Server {
    Server();
    virtual ~Server();
    void* underlying;

    Server(const Server& rhs ) = delete;
    Server(Server&& rhs ) = delete;
    Server& operator=(const Server& rhs) = delete;
    Server& operator=(Server&& rhs) = delete;
};

struct IOService {
    IOService();
    virtual ~IOService();
    void* underlying;

    IOService(const IOService& rhs ) = delete;
    IOService(IOService&& rhs ) = delete;
    IOService& operator=(const IOService& rhs) = delete;
    IOService& operator=(IOService&& rhs) = delete;
};

struct ASIOClient {
    ASIOClient();
    virtual ~ASIOClient();
    void* underlying;

    using ConnPtr = std::shared_ptr<void>;
};

struct ConnectHdl {
    std::weak_ptr<void> hdl;
};

#endif //CPPWEBSOCKETRESPONSEREQUEST_WRAPER_H
