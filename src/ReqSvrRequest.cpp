#include "ReqSvrRequest.h"
#include "logger.h"

#include "SimpleJSON.h"


namespace {
    // TODO: This is completely the wrong place. It should be shared with the
    //       ReqSvr class
    static const std::string ERROR_FLAG = "ERROR ";
    NewIntField(errorNumber);
    NewStringField(errorText);
    typedef SimpleParsedJSON<errorNumber,errorText> ErrorMsg;

    struct ErrorFields {
        std::string errorText;
        int errorCode;
    };

    bool isError(const std::string& msg, ErrorFields& error) {
        bool isError = false;
        if ( msg.length() > ERROR_FLAG.length() &&
             msg.substr(0, ERROR_FLAG.length()) == ERROR_FLAG)
        {
            static ErrorMsg parser;
            parser.Clear();

            static std::string parseFailMsg;

            const char* const errorObjStr =
                    msg.c_str() + ERROR_FLAG.length();

            if (parser.Parse(errorObjStr, parseFailMsg)) {
                error.errorCode = parser.Get<errorNumber>();
                error.errorText = parser.Get<errorText>();
                isError = true;
            }
        }
        else {
            isError = false;
        }

        return isError;
    }
}

ReqSvrRequest::ReqSvrRequest(
    const std::string& reqName,
    const std::string& data)
  : statusFuture(statusFlag.get_future())
{
    std::stringstream payloadStream;
    payloadStream << reqName << " " << data;
    payload = payloadStream.str();
}

ReqSvrRequest::~ReqSvrRequest() {
}

const ReplyMessage& ReqSvrRequest::WaitForMessage() {
    statusFuture.wait();
    switch(message.state_)
    {
    case ReplyMessage::COMPLETE:
    case ReplyMessage::REJECTED:
        // all good - safe to return
        break;
    case ReplyMessage::INVALID_URI:
        throw InvalidURIError{ message.error_ };
        break;
    case ReplyMessage::DISCONNECT:
        throw ServerDisconnectedError{ message.error_ };
        break;

    // These shouldn't happen, even in the network error_ case - they indicate a
    // programmatic error_ in the Async handler;
    case ReplyMessage::IN_FLIGHT:
    case ReplyMessage::PENDING:
        LOG_FROM(
            LOG_ERROR,
            "ReqSvrRequest::WaitForMessage",
            "ReqSvrRequest completed with a non terminal state!");
        throw ServerDisconnectedError{ message.error_ };
        break;
    }
    return message;
}

const std::string& ReqSvrRequest::onOpen() {
    message.state_ = ReplyMessage::IN_FLIGHT;
    return payload;
}

void ReqSvrRequest::onInvalidRequest(std::string err) {
    message.state_ = ReplyMessage::INVALID_URI;
    message.error_ = std::move(err);
    statusFlag.set_value(false);
}

void ReqSvrRequest::onComplete() {
    onTerminate();
}

void ReqSvrRequest::onTerminate() {
    switch (message.state_)
    {
    case ReplyMessage::IN_FLIGHT:
    case ReplyMessage::PENDING:
        message.state_ = ReplyMessage::DISCONNECT;
        statusFlag.set_value(false);
        break;

    case ReplyMessage::REJECTED:
    case ReplyMessage::COMPLETE:
    case ReplyMessage::INVALID_URI:
    case ReplyMessage::DISCONNECT:
        // nothing to do
        break;
    }
}

void ReqSvrRequest::onMessage(std::string msg) {
    ErrorFields fields;

    if (isError(msg, fields)) {
        message.state_ = ReplyMessage::REJECTED;
        message.content_ = fields.errorText;
        message.code_ = fields.errorCode;
    } else {
        message.state_ = ReplyMessage::COMPLETE;
        message.content_ = msg;
    }

    statusFlag.set_value(true);
}


ReqSvrRequest::ReqSvrRequestError::ReqSvrRequestError(std::string msg)
   : msg_(std::move(msg))
{

}
