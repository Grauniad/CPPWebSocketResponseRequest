#include "stream_client.h"

#include <iostream>
#include "WebSocketPP.h"


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;

typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;

namespace {
    void on_message(StreamClient* cli, websocketpp::connection_hdl hdl, message_ptr msg)
    {
        cli->OnMessage(msg->get_payload());
    }

    void on_sub_open(WebClient* cli, const std::string* hello, websocketpp::connection_hdl hdl)
    {
        cli->send(hdl, *hello, websocketpp::frame::opcode::text);
    }

    void on_pong_timeout(StreamClient* cli, websocketpp::connection_hdl hdl, std::string msg)
    {
        cli->StopNonBlock();
    }
}

StreamClient::StreamClient(std::string url, std::string subName, std::string subBody)
        : running(false)
        , subName(std::move(subName))
        , subBody(std::move(subBody))
        , url(std::move(url))
        , futureStart(startFlag.get_future())
        , futureStop(stopFlag.get_future())
{
    auto& endpoint = ToWeb(m_endpoint);
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.set_error_channels(websocketpp::log::elevel::all);

    endpoint.init_asio();

    hello = this->subName + " " + this->subBody;

    // Register our handlers
    endpoint.set_message_handler(bind(on_message,this,::_1,::_2));
    endpoint.set_open_handler(bind(on_sub_open, &endpoint,&hello,::_1));
    endpoint.set_pong_timeout_handler(bind(on_pong_timeout,this, ::_1, ::_2));
}

void StreamClient::Run() {
    running = true;
    startFlag.set_value(true);
    websocketpp::lib::error_code ec;
    con = MakeWrapper(ToWeb(m_endpoint).get_connection(url, ec));
    if (ec) {
        std::cout << "Failed to connect [" << url << "]"
                  << ": " << ec.message() << std::endl;
        stopFlag.set_value(true);
        running = false;
        throw InvalidUrlException{};
    } else {
        ToWeb(m_endpoint).connect(ToWeb(con));
        ToWeb(m_endpoint).run();
    }
    stopFlag.set_value(true);
    running = false;
}

void StreamClient::Stop() {
    bool running = Running();
    StopNonBlock();
    if (running) {
        futureStop.get();
    }
}

void StreamClient::StopNonBlock() {
    auto webcon = ToWeb(con);
    if ( Running() && webcon ) {
        websocketpp::lib::error_code ec;
        ToWeb(m_endpoint).close(webcon,websocketpp::close::status::going_away,"", ec);
        ToWeb(m_endpoint).stop();
    }
}

bool StreamClient::Running() {
    return running;
}

bool StreamClient::WaitUntilRunning() {
    return futureStart.get();
}


StreamClient::~StreamClient() {
    this->Stop();
}

void StreamClient::Ping(unsigned int timeout) {
    auto webcon = ToWeb(con);
    webcon->set_pong_timeout(timeout);
    webcon->ping("PING!");
}

