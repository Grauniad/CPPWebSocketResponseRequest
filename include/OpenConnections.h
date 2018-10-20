#ifndef CPPWEBSOCKETRESPONSEREQUEST_OPENCONNECTIONS_H
#define CPPWEBSOCKETRESPONSEREQUEST_OPENCONNECTIONS_H
#include <ReqServer.h>

class OpenConnectionsList {
public:
    void Add(SubscriptionHandler::RequestHandle hdl);

    void Publish(const std::string& msg);
private:
    std::vector<SubscriptionHandler::RequestHandle> hdls;
};

#endif //CPPWEBSOCKETRESPONSEREQUEST_OPENCONNECTIONS_H
