#include "ReqServer.h"


#include <iostream>
#include <string>
#include <sstream>

#include <SimpleJSON.h>
#include <logger.h>
#include <OSTools.h>

/**************************************************************
 *                Utilities
 **************************************************************/

namespace {
    NewIntField(errorNumber);
    NewStringField(errorText);
    typedef SimpleParsedJSON<errorNumber,errorText> ErrorMsg;

    std::string ErrorMessage(const std::string& msg, const int& code) {
        std::string response;
        static ErrorMsg errJSON;
        errJSON.Clear();

        errJSON.Get<errorNumber>() = code;
        errJSON.Get<errorText>() = msg;
        response = "ERROR ";
        response += errJSON.GetJSONString();
        return response;
    }

}

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef Server::message_ptr message_ptr;

// Define a callback to handle incoming messages
void on_message(RequestServer* s, Server* raw_server, websocketpp::connection_hdl hdl, message_ptr msg) {
    try {
        if (msg->get_payload() == "stop-listening") {
            raw_server->stop_listening();
            raw_server->stop();
        } else {
            SLOG_FROM(LOG_VERBOSE,">> ReqServer::on_message",
                "Received message from: " << hdl.lock().get() << endl <<
                "Message: " << msg->get_payload() << endl);
            const std::string& reply = s->HandleMessage(msg->get_payload(),raw_server,hdl);

            if (reply != "") {
                SLOG_FROM(LOG_VERBOSE,"<< ReqServer::on_message",
                    "Sending Message to:" << hdl.lock().get() << endl <<
                    "Message: " << reply << endl);
                raw_server->send(hdl,reply,websocketpp::frame::opcode::TEXT);
            }
        }
    } catch (RequestReplyHandler::FatalError& fatal) {
        websocketpp::lib::error_code ec;
        raw_server->get_con_from_hdl(hdl)->close(
                websocketpp::close::status::internal_endpoint_error,
                "Fatal error_",
                ec);
        SLOG_FROM(LOG_VERBOSE,"RequestServer::on_message",
            "An unrecoverable error_ occurred - shutting down the server");
        s->FatalError(fatal.code, fatal.errMsg);
    }
}

void on_close(RequestServer*s, websocketpp::connection_hdl hdl) {
    SLOG_FROM(LOG_VERBOSE,"ReqServer::on_close",
    "Closing session to: " << hdl.lock().get() << endl);
    s->HandleClose(hdl);
}

/**************************************************************
 *                Data Subscriptions
 **************************************************************/
class Request: public SubscriptionHandler::SubRequest {
public:
    Request(
        const std::string& req,
        Server* s,
        websocketpp::connection_hdl c)

    : request(req), serv(s), conn(c), open(true)
    {
    }

    virtual ~Request() { };

    const char* RequestMessasge() {
        return request.c_str();
    }

    /**
     * Send a data update, a JSON message, down the pipe
     */
    void SendMessage(const std::string& msg) {
        if (Ok()) {
            serv->send(conn,msg,websocketpp::frame::opcode::TEXT);
        }
    }

    bool Ok() const { return open; }

    void Close () {
        open = false;
    }

private:
    std::string                 request;
    Server*                     serv;
    websocketpp::connection_hdl conn;
    bool                        open;
};

/**************************************************************
 *                Request Serve
 **************************************************************/

RequestServer::RequestServer()
   : running(runningFlag.get_future())
   , stopped(stopFlag.get_future())
   , failed(false)
   , failCode(0)
{
    // Set logging settings
    requestServer_.clear_access_channels(websocketpp::log::alevel::all);
    requestServer_.set_access_channels(websocketpp::log::alevel::fail);

    requestServer_.init_asio();
}

void RequestServer::AddHandler(
         const std::string& requestName, 
         std::unique_ptr<RequestReplyHandler> handler)
{
    req_handlers[requestName].reset(handler.release());
}

void RequestServer::AddHandler(
         const std::string& requestName, 
         std::shared_ptr<SubscriptionHandler> handler)
{
    sub_handlers[requestName] = std::move(handler);
}

std::string RequestServer::HandleMessage(
    const std::string& request,
    Server* raw_server,
    websocketpp::connection_hdl hdl)
{

    std::string response = "";
    std::stringstream strRequest(request);
    std::string reqName;
    strRequest >> reqName;

    auto req_it = req_handlers.find(reqName);

    if ( req_it != req_handlers.end()) {
        response = HandleRequestReplyMessage(reqName,request,*(req_it->second));
    } else {
        auto sub_it = sub_handlers.find(reqName);
        if ( sub_it != sub_handlers.end()) {
            response = 
                HandleSubscriptionMessage(reqName, request, *(sub_it->second),raw_server,hdl);
        } else {
            response = ErrorMessage("No such request: " + reqName, 0);
        }
    }
    return response;
}

std::string RequestServer::HandleRequestReplyMessage(
                    const std::string& reqName,
                    const std::string& request,
                    RequestReplyHandler& handler) 
{
    std::string response = "";
    // "REQUEST_NAME { name: JSON_REQUEST, ...}"
    unsigned int offset = reqName.length() + 1;
    const char* jsonRequest = "";
    if (request.length() > offset) {
        jsonRequest = request.c_str() + offset;
    }
    try {
        response = handler.OnRequest(jsonRequest);
    } catch (const RequestReplyHandler::InvalidRequestException& e) {
        response = ErrorMessage(e.errMsg, e.code);
    }

    return response;
}

void RequestServer::HandleClose(websocketpp::connection_hdl hdl) {
    auto it = conn_map.find(StoredHdl(hdl));

    if (it != conn_map.end()) {
        it->second->Close();
        conn_map.erase(it);
    }
}

RequestServer::~RequestServer() {
    Stop();
}

void RequestServer::WaitUntilRunning()
{
    running.wait();
}

void RequestServer::Stop() {
    websocketpp::lib::error_code ec;
    requestServer_.stop_listening(ec);
    requestServer_.stop();
    std::vector<websocketpp::connection_hdl> toClose;
    toClose.reserve(conn_map.size());
    for (auto& pair: conn_map) {
        toClose.push_back(pair.first.hdl);
    }
    for (auto& hdl: toClose) {
        requestServer_.close(hdl, websocketpp::close::status::internal_endpoint_error, "Stopping");
    }
    stopped.wait();
}

void RequestServer::FatalError(int code, std::string message) {
    failed = true;
    failCode = code;
    failMsg = std::move(message);
    websocketpp::lib::error_code ec;
    requestServer_.stop_listening(ec);
    requestServer_.stop();
}

std::string RequestServer::HandleSubscriptionMessage(
                    const std::string& reqName,
                    const std::string& request,
                    SubscriptionHandler& handler,
                    Server*  raw_server,
                    websocketpp::connection_hdl hdl)
{
    std::string response = "";
    // "REQUEST_NAME { name: JSON_REQUEST, ...}"
    unsigned int offset = reqName.length() + 1;
    const char* jsonRequest = "";
    if (request.length() > offset) {
        jsonRequest = request.c_str() + offset;
    }
    try {
        SubscriptionHandler::RequestHandle reqHdl(
            new Request(jsonRequest,raw_server,hdl));
        conn_map.insert({hdl, reqHdl});
        handler.OnRequest(reqHdl);
    } catch (const SubscriptionHandler::InvalidRequestException& e) {
        response = ErrorMessage(e.errMsg, e.code);
    }

    return response;
}


void RequestServer::HandleRequests(unsigned short port) {
    struct OnExit {
        std::promise<bool>& stopFlag_;
        ~OnExit() {
            stopFlag_.set_value(true);
        }
    } stopOnExit{stopFlag};

    try {
        requestServer_.set_message_handler(bind(&on_message,this,&requestServer_,::_1,::_2));
        requestServer_.set_close_handler(bind(&on_close,this,::_1));

        // Allow address re-use so that orphaned sessions from an old server
        // which are about to be killed off don't prevent us starting up.
        // NOTE: This will *not* allow two active server listeners (see
        //       NoDoublePortBind test)...
        requestServer_.set_reuse_addr(true);

        // ... unless we're on Windows, which  can't do POSIX properly.
        // (On WSL NoDoublePortBind will fail since it incorrectly allows the bind)
        if (OS::IsWSLSystem()) {
            requestServer_.set_reuse_addr(false);
        }

        requestServer_.listen(port);

        requestServer_.start_accept();

        PostTask([&] () -> void {
            this->runningFlag.set_value(true);
        });

        // Start the ASIO io_service run loop
        requestServer_.run();
    } catch (websocketpp::exception const & e) {
        FatalError(-1, e.what());
    }

    if (failed)
    {
        throw FatalErrorException {failCode, failMsg};
    }
}

void RequestServer::PostTask(const RequestServer::InteruptHandler& f) {
    boost::asio::io_service& serv = requestServer_.get_io_service();
    
    serv.post(f);
}
