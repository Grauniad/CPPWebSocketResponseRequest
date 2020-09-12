#ifndef CPPWEBSOCKETRESPONSEREQUEST_WEBSOCKETPP_H
#define CPPWEBSOCKETRESPONSEREQUEST_WEBSOCKETPP_H

#include <CPPWSRRWraper.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <memory>
#include <boost/asio.hpp>

using WebServer = websocketpp::server<websocketpp::config::asio>;
using WebConnHdl = websocketpp::connection_hdl;
using WebClient = websocketpp::client<websocketpp::config::asio_client>;
using WebIOService = boost::asio::io_service;

namespace  {
    inline WebServer& ToWeb(Server& s) { return *reinterpret_cast<WebServer*>(s.underlying);}
    inline WebServer* ToWeb(Server* s) { return reinterpret_cast<WebServer*>(s->underlying);}
    inline WebConnHdl& ToWeb(ConnectHdl& c) { return reinterpret_cast<WebConnHdl&>(c.hdl);}
    inline WebClient& ToWeb(ASIOClient& c) { return *reinterpret_cast<WebClient*>(c.underlying);}
    inline WebClient::connection_ptr ToWeb(ASIOClient::ConnPtr& c) {
        return reinterpret_pointer_cast<WebClient::connection_ptr::element_type>(c);
    }
    inline WebIOService& ToWeb(IOService& s) { return *reinterpret_cast<WebIOService*>(s.underlying); }
    inline WebIOService* ToWeb(IOService* s) { return reinterpret_cast<WebIOService*>(s->underlying); }

    inline ConnectHdl MakeWrapper(const WebConnHdl& c) { return ConnectHdl {.hdl = c}; }
    inline ASIOClient::ConnPtr MakeWrapper(const WebClient::connection_ptr & webPtr) {
        return ASIOClient::ConnPtr (webPtr);
    }
}
#endif
