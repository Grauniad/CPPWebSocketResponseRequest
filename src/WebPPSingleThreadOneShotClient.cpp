//
// Created by lhumphreys on 31/03/18.
//

#include "WebPPSingleThreadOneShotClient.h"

WebPPSingleThreadOneShotClient::WebPPSingleThreadOneShotClient(
        boost::asio::io_service& io)
{
    client_.init_asio(&io);
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.set_access_channels(websocketpp::log::alevel::fail);

    client_.set_message_handler(
            websocketpp::lib::bind(
                    &WebPPSingleThreadOneShotClient::OnWebPPMessage,
                    this,
                    websocketpp::lib::placeholders::_1,
                    websocketpp::lib::placeholders::_2));

    client_.set_open_handler(
            websocketpp::lib::bind(
                    &WebPPSingleThreadOneShotClient::OnWebPPOpen,
                    this,
                    websocketpp::lib::placeholders::_1));

    client_.set_close_handler(
            websocketpp::lib::bind(
                    &WebPPSingleThreadOneShotClient::OnWebPPClose,
                    this,
                    websocketpp::lib::placeholders::_1));

    client_.set_fail_handler(
            websocketpp::lib::bind(
                    &WebPPSingleThreadOneShotClient::OnWebPPFail,
                    this,
                    websocketpp::lib::placeholders::_1));
}

size_t WebPPSingleThreadOneShotClient::newConnection (
        const std::string &uri,
        std::shared_ptr<IOneShotConnectionConsumer> consumer)
{
    size_t hdl = 0;
    websocketpp::lib::error_code ec;
    auto con = client_.get_connection(uri, ec);

    if (ec || con.get() == nullptr) {
        consumer->onInvalidRequest(ec.message());
    } else {
        hdl = conns_.NewConn(consumer, con.get());
        client_.connect(con);
    }

    return hdl;
}

void WebPPSingleThreadOneShotClient::OnWebPPOpen(
        websocketpp::connection_hdl hdl)
{
    auto con = client_.get_con_from_hdl(hdl);
    auto consumer = conns_.GetConsumer(con);
    if (consumer.get()) {
        const std::string& payload = consumer->onOpen();
        client_.send(hdl, payload, websocketpp::frame::opcode::text);
    }
}

void WebPPSingleThreadOneShotClient::OnWebPPMessage(
        websocketpp::connection_hdl hdl,
        WebPPSingleThreadOneShotClient::message_ptr msg)
{
    auto con = client_.get_con_from_hdl(hdl);
    auto consumer = conns_.GetConsumer(con);
    if (consumer.get()) {
        const std::string payload = msg->get_payload();
        consumer->onMessage(std::move(payload));

        // One shot protocol - kill the connection
        con->close(0, "bye bye");
    }
}

void WebPPSingleThreadOneShotClient::OnWebPPClose(
        websocketpp::connection_hdl hdl)
{
    auto con = client_.get_con_from_hdl(hdl);
    auto consumer = conns_.GetConsumer(con);
    if (consumer.get()) {
        consumer->onComplete();
        conns_.ReleaseConn(con);
    }
}

void WebPPSingleThreadOneShotClient::OnWebPPFail(
        websocketpp::connection_hdl hdl)
{
    auto con = client_.get_con_from_hdl(hdl);
    auto consumer = conns_.GetConsumer(con);
    if (consumer.get()) {
        consumer->onTerminate();
        conns_.ReleaseConn(con);
    }

}
