//D
// Created by lhumphreys on 16/08/2020.
//

#include "ClientUtils.h"
#include <io_thread.h>

#include <fstream>
#include <sstream>
#include <SimpleJSON.h>

namespace
{
    std::string GetLocalURI(const std::string& port) {
        return (std::string{"ws://localhost:"} + port);
    }

    std::string DoRequest(const std::string& uri,
                          const std::string& reqName,
                          const std::string& msg)
    {
        std::string result;

        IOThread client;
        auto request = client.Request(uri, reqName, msg);
        try {
            const auto& response = request->WaitForMessage();
            if (response.state_ == ReplyMessage::REJECTED) {
                std::stringstream buf;
                buf << "[REJECTED] Server Rejected our request: " << std::endl
                    << "    Server URI: " << uri << std::endl
                    << "    Code:       " << response.code_ << std::endl
                    << "    Error:      " << response.content_ << std::endl;
                result = buf.str();
            } else {
                result = response.content_;
            }
        } catch (ReqSvrRequest::InvalidURIError& invalidUri) {
            std::stringstream buf;
            buf << "[INVALID_URI] Invaldi URI requested: " << std::endl
                << "    Server URI: " << uri << std::endl
                << "    Error:      " << invalidUri.msg_ << std::endl;
            result = buf.str();
        } catch (ReqSvrRequest::ServerDisconnectedError& disconnectedError) {
            std::stringstream buf;
            buf << "[DISCONNECT] Failed to establish connection" << std::endl
                << "    Server URI: " << uri << std::endl
                << "    Error:      " << disconnectedError.msg_ << std::endl;
            result = buf.str();
        }

        return result;
    }
}

std::string ClientUtils::FileRequestLocal(
        const std::string& port,
        const std::string& reqName,
        const std::string& fname)
{
    std::string result;
    std::ifstream input(fname);
    if (input.good()) {
        std::string msg{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        result = DoRequest(GetLocalURI(port), reqName, msg);
    } else {
        result = "[BAD_FILE] ";
        result += fname;
    }

    return result;
}

std::string ClientUtils::JSONRequestLocal(const std::string &port,
                                          const std::string &reqName,
                                          const std::map<std::string, std::string> &values)
{
    SimpleJSONBuilder builder;
    for (const auto& pair: values) {
        builder.Add(pair.first, pair.second);
    }

    return DoRequest(GetLocalURI(port), reqName, builder.GetAndClear());
}
