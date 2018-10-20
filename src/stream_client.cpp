#include "stream_client.h"

#include <iostream>
#include <stream_client.h>


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

using namespace std;

StreamClient::StreamClient(std::string url, std::string subName, std::string subBody)
        : running(false)
        , con(nullptr)
        , subName(std::move(subName))
        , subBody(std::move(subBody))
        , url(std::move(url))
        , futureStart(startFlag.get_future())
        , futureStop(stopFlag.get_future())
{
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    m_endpoint.init_asio();

    // Register our handlers
    m_endpoint.set_message_handler(bind(&StreamClient::on_message,this,::_1,::_2));
    m_endpoint.set_open_handler(bind(&StreamClient::on_sub_open,this,::_1));
}

void StreamClient::on_open(websocketpp::connection_hdl hdl) {
    m_endpoint.send(hdl, "", websocketpp::frame::opcode::text);
}

void StreamClient::on_sub_open(websocketpp::connection_hdl hdl) {
    m_endpoint.send(hdl, subName + " " + subBody, websocketpp::frame::opcode::text);
}

void StreamClient::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    OnMessage(msg->get_payload());
}

void StreamClient::Run() {
    running = true;
    startFlag.set_value(true);
    websocketpp::lib::error_code ec;
    con = m_endpoint.get_connection(url, ec);

    if (ec) {
        std::cout << "Failed to connect [" << url << "]"
                  << ": " << ec.message() << std::endl;
    } else {
        m_endpoint.connect(con);
        m_endpoint.run();
    }
    stopFlag.set_value(true);
    running = false;
}

void StreamClient::Stop() {
    if ( Running() && con.get() ) {
        websocketpp::lib::error_code ec;
        m_endpoint.close(con,websocketpp::close::status::going_away,"", ec);
    }
    m_endpoint.get_io_service().stop();
    m_endpoint.get_io_service().reset();

    if (Running() ) {
        futureStop.get();
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

