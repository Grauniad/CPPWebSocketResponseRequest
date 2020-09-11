//
// Created by lhumphreys on 31/03/18.
//

#include "WebPPSingleThreadOneShotClient.h"
#include "WebSocketPP.h"

#include "websocketpp/client.hpp"
#include "IOneShotConnectionConsumer.h"
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <openssl/asn1.h>

using boost::asio::ip::tcp;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
struct OneShotWebWrap {

    static void OnWebPPMessage(
            WebPPSingleThreadOneShotClient* cli,
            websocketpp::connection_hdl hdl,
            message_ptr msg)
    {
        auto con = ToWeb(cli->client_).get_con_from_hdl(hdl);
        auto consumer = cli->conns_.GetConsumer(con);
        if (consumer.get()) {
            const std::string payload = msg->get_payload();
            consumer->onMessage(std::move(payload));

            // One shot protocol - kill the connection
            con->close(0, "bye bye");
        }
    }

    static void OnWebPPOpen(
            WebPPSingleThreadOneShotClient* cli,
            websocketpp::connection_hdl hdl)
    {
        auto con = ToWeb(cli->client_).get_con_from_hdl(hdl);
        auto consumer = cli->conns_.GetConsumer(con);
        if (consumer.get()) {
            const std::string& payload = consumer->onOpen();
            ToWeb(cli->client_).send(hdl, payload, websocketpp::frame::opcode::text);
        }
    }

    static void OnWebPPClose(
            WebPPSingleThreadOneShotClient* cli,
            websocketpp::connection_hdl hdl)
    {
        auto con = ToWeb(cli->client_).get_con_from_hdl(hdl);
        auto consumer = cli->conns_.GetConsumer(con);
        if (consumer.get()) {
            consumer->onComplete();
            cli->conns_.ReleaseConn(con);
        }
    }

    static void OnWebPPFail(
            WebPPSingleThreadOneShotClient* cli,
            websocketpp::connection_hdl hdl)
    {
        auto con = ToWeb(cli->client_).get_con_from_hdl(hdl);
        auto consumer = cli->conns_.GetConsumer(con);
        if (consumer.get()) {
            consumer->onTerminate();
            cli->conns_.ReleaseConn(con);
        }

    }
};

WebPPSingleThreadOneShotClient::WebPPSingleThreadOneShotClient(
        IOService& io)
{
    auto& cli = ToWeb(client_);
    cli.init_asio(ToWeb(&io));
    cli.clear_access_channels(websocketpp::log::alevel::all);
    cli.set_access_channels(websocketpp::log::alevel::fail);

    cli.set_message_handler(
            websocketpp::lib::bind(
                    &OneShotWebWrap::OnWebPPMessage,
                    this,
                    websocketpp::lib::placeholders::_1,
                    websocketpp::lib::placeholders::_2));

    cli.set_open_handler(
            websocketpp::lib::bind(
                    &OneShotWebWrap::OnWebPPOpen,
                    this,
                    websocketpp::lib::placeholders::_1));

    cli.set_close_handler(
            websocketpp::lib::bind(
                    &OneShotWebWrap::OnWebPPClose,
                    this,
                    websocketpp::lib::placeholders::_1));

    cli.set_fail_handler(
            websocketpp::lib::bind(
                    &OneShotWebWrap::OnWebPPFail,
                    this,
                    websocketpp::lib::placeholders::_1));
}

size_t WebPPSingleThreadOneShotClient::newConnection (
        const std::string &uri,
        std::shared_ptr<IOneShotConnectionConsumer> consumer)
{
    size_t hdl = 0;
    websocketpp::lib::error_code ec;
    auto con = ToWeb(client_).get_connection(uri, ec);

    if (ec || con.get() == nullptr) {
        consumer->onInvalidRequest(ec.message());
    } else {
        hdl = conns_.NewConn(consumer, con.get());
        ToWeb(client_).connect(con);
    }

    return hdl;
}

