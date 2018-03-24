#include "AsyncReqSvrRequest.h"

#include <logger.h>
#include <boost/bind.hpp>

#include "SimpleJSON.h"

AsyncReqSvrRequest::AsyncReqSvrRequest(
    boost::asio::io_service& io_service,
    const std::string& uri,
    const std::string& reqName,
    const std::string& data)
  : uri_(uri)
{
    client_.init_asio(&io_service);

    std::stringstream payloadStream;
    payloadStream << reqName << " " << data;
    payload_ = payloadStream.str();
}

AsyncReqSvrRequest::~AsyncReqSvrRequest() {
}

void AsyncReqSvrRequest::StartRequest() {
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.set_access_channels(websocketpp::log::alevel::fail);

    client_.set_message_handler(
            websocketpp::lib::bind(
                &AsyncReqSvrRequest::OnMessage,
                this,
                websocketpp::lib::placeholders::_1,
                websocketpp::lib::placeholders::_2));

    client_.set_open_handler(
            websocketpp::lib::bind(
                &AsyncReqSvrRequest::OnOpen,
                this,
                websocketpp::lib::placeholders::_1));

    client_.set_close_handler(
            websocketpp::lib::bind(
                &AsyncReqSvrRequest::OnClose,
                this,
                websocketpp::lib::placeholders::_1));

    client_.set_fail_handler(
            websocketpp::lib::bind(
                &AsyncReqSvrRequest::OnFail,
                this,
                websocketpp::lib::placeholders::_1));

    websocketpp::lib::error_code ec;
    auto con = client_.get_connection(uri_, ec);
    con_ = con;

    if (ec || con.get() == nullptr) {
        reply_.state_ = ReplyMessage::INVALID_URI;
        reply_.error = ec.message();
        OnComplete(reply_);
    } else {
        client_.connect(con);
    }
}

void AsyncReqSvrRequest::OnMessage(
        websocketpp::connection_hdl hdl,
        message_ptr msg)
{
    static const std::string ERROR_FLAG = "ERROR ";
    NewIntField(errorNumber);
    NewStringField(errorText);
    typedef SimpleParsedJSON<errorNumber,errorText> ErrorMsg;
    static ErrorMsg errorParser;

    // Assume it was a standard response, and store it in reply
    reply_.content = msg->get_payload();
    reply_.state_ = ReplyMessage::COMPLETE;

    // But must also check that it wasn't a standard error...
    const std::string& payload = reply_.content;
    if ( payload.length() > ERROR_FLAG.length() &&
         payload.substr(0, ERROR_FLAG.length()) == ERROR_FLAG)
    {
        const char* const errorObjStr = payload.c_str() + ERROR_FLAG.length();
        std::string parseError;
        errorParser.Clear();

        if (errorParser.Parse(errorObjStr, parseError)) {
            reply_.code = errorParser.Get<errorNumber>();
            reply_.content = errorParser.Get<errorText>();
            reply_.state_ = ReplyMessage::REJECTED;
        }
    }
    // One shot protocol - kill the connection
    CloseConnection(0, "bye bye");
}

void AsyncReqSvrRequest::OnOpen(websocketpp::connection_hdl hdl) {
    reply_.state_ = ReplyMessage::IN_FLIGHT;
    client_.send(hdl, payload_, websocketpp::frame::opcode::text);
}

void AsyncReqSvrRequest::OnFail(websocketpp::connection_hdl hdl) {
    if ( reply_.state_ == ReplyMessage::PENDING )
    {
        LOG_FROM(LOG_ERROR, "AsyncReqSvrRequest::OnFail", "Connection error!");
        reply_.state_= ReplyMessage::DISCONNECT;
        reply_.error = "Failed to contact server";

        // We never opened the session, so we're never going to get the close
        // call-back. At this point we're terminal, declare done...
        OnComplete(reply_);
    } else {
        LOG_FROM(LOG_ERROR, "AsyncReqSvrRequest::OnFail", "Unexpected error!");
    }
}

void AsyncReqSvrRequest::OnClose(websocketpp::connection_hdl hdl) {
    switch (reply_.state_)
    {
    // Legitimate close...
    case ReplyMessage::COMPLETE:
    case ReplyMessage::REJECTED:
        OnComplete(reply_);
        break;

    // unsolicited close..
    case ReplyMessage::IN_FLIGHT:
    {
        LOG_FROM(LOG_ERROR,
                "AsyncReqSvrRequest::OnClose",
                "In flight disconnect!");
        reply_.state_= ReplyMessage::DISCONNECT;
        reply_.error = "Disconnected from server";

        CloseConnection(-2, "Failure during server communication");
        OnComplete(reply_);
        break;
    }

    // Shouldn't be possible - we've already closed...
    case ReplyMessage::DISCONNECT:
        LOG_FROM(LOG_ERROR, "AsyncReqSvrRequest::OnClose", "Close after disconnect!");
        break;

    // Shouldn't be possible - we never opened the connection...
    case ReplyMessage::INVALID_URI:
        LOG_FROM(LOG_ERROR, "AsyncReqSvrRequest::OnClose", "Close after invalid URI!");
        break;
    case ReplyMessage::PENDING:
        LOG_FROM(LOG_ERROR, "AsyncReqSvrRequest::OnClose", "Close whilst PENDING!");
        break;
    }
}

void AsyncReqSvrRequest::CloseConnection(
    int16_t reasonCode,
    std::string reason)
{
    auto con = con_.lock();
    if (con.get() != nullptr)
    {
        switch (con->get_state())
        {
        case websocketpp::session::state::closed:
        case websocketpp::session::state::closing:
            // already closed / closing...
            break;
        case websocketpp::session::state::connecting:
        case websocketpp::session::state::open:
            // Instruct the connection to close off.
            websocketpp::lib::error_code ec;
            con->close(reasonCode, reason, ec);
            // We're shutting down - if we get hit by a race condition and
            // someone else shuts down first that's fine: just drop the error
            // code...
        }
    }
}
