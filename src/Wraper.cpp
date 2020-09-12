//
// Created by lhumphreys on 11/09/2020.
//

#include <CPPWSRRWraper.h>
#include "WebSocketPP.h"

Server::Server() {
    this->underlying = new WebServer;
}
Server::~Server() {
    delete (WebServer*) this->underlying;
}

ASIOClient::ASIOClient() {
    this->underlying = new WebClient ;
}

ASIOClient::~ASIOClient() {
    delete (WebClient*) this->underlying;
}

IOService::IOService() {
    this->underlying = new WebIOService;
}
IOService::~IOService() {
    delete (WebIOService*) this->underlying;
}
