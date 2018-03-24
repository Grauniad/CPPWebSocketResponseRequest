#include "stream_client.h"

#include <iostream>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

using namespace std;

StreamClientThread::StreamClientThread(const std::string& url)
   : StreamClient(url),
     io_thread([=] () -> void { this->Run(); })
{
}

StreamClientThread::~StreamClientThread()
{
    Stop();
    io_thread.join();
}

StreamClient::StreamClient(const std::string& url) 
   : running(false), con(nullptr)
{
    m_endpoint.set_access_channels(websocketpp::log::alevel::all);
    m_endpoint.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    m_endpoint.init_asio();

    // Register our handlers
    m_endpoint.set_tls_init_handler(bind(&StreamClient::on_tls_init,this,::_1));
    m_endpoint.set_message_handler(bind(&StreamClient::on_message,this,::_1,::_2));
    m_endpoint.set_open_handler(bind(&StreamClient::on_open,this,::_1));

    websocketpp::lib::error_code ec;
    con = m_endpoint.get_connection(url, ec);

    if (ec) {
        m_endpoint.get_alog().write(websocketpp::log::alevel::app,ec.message());
    }

    m_endpoint.connect(con);
}

context_ptr StreamClient::on_tls_init(websocketpp::connection_hdl) {
    typedef boost::asio::ssl::context context;

    context_ptr ctx = 
        websocketpp::lib::make_shared<context>(context::tlsv1);

    try {
        ctx->set_options(context::default_workarounds);
    } catch (std::exception& e) {
        std::cout << "TLS Failure: " << std::endl;
        std::cout << e.what() << std::endl;
    }

    return ctx;
}

void StreamClient::on_open(websocketpp::connection_hdl hdl) {
    m_endpoint.send(hdl, "", websocketpp::frame::opcode::text);
}

void StreamClient::on_message(websocketpp::connection_hdl hdl, message_ptr msg) 
{
    OnMessage(msg->get_payload());
}

void StreamClient::Run() {
    running = true;
    m_endpoint.run();
    running = false;
}

void StreamClient::Stop() {
    if ( Running() && con.get() ) {
        m_endpoint.close(con,websocketpp::close::status::going_away,"");
    }
}

bool StreamClient::Running() {
    return running;
}
