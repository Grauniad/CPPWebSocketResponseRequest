//
// Created by lhumphreys on 31/03/18.
//

#ifndef CPPWEBSOCKETRESPONSEREQUEST_WEBPPCLIENT_H
#define CPPWEBSOCKETRESPONSEREQUEST_WEBPPCLIENT_H

#include "websocketpp/client.hpp"
#include "IOneShotConnectionConsumer.h"
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <openssl/asn1.h>

// TODO: Don't pollute the global space...
using boost::asio::ip::tcp;

/*
 * Adapter to the webpp client, providing a much simpler interface.
 *
 * We can simplify this interface by requiring that all interactions
 * (including construction / destruction) occur on the io_service thread
 *
 * This restriction means that the adapter is not useful as a gneral purpose
 * tool in applications, and is instead intended for use by the IOThread
 */
class WebPPSingleThreadOneShotClient
{
public:
    /*
     * @param io The io service to run the client on.
     *
     * Remember: We must be constructed on the same thread that the
     *           io service is running on: and all invocations must be
     *           on that thread.
     */
    explicit WebPPSingleThreadOneShotClient(boost::asio::io_service& io);

    /*
     * Open a new request response connection to the specified uri
     */
    size_t newConnection(
            const std::string& uri,
            std::shared_ptr<IOneShotConnectionConsumer> consumer);

private:
    // TODO: If we declare to webpp that we are using a thread safe access
    //       idium then we can prevent a bunch of locking
    typedef websocketpp::client<websocketpp::config::asio_client> Client;
    typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
    Client client_;

    /*
     * START: Callbacks from the WebPP client...
     */

    // Call-back to notify of us a new message received
    void OnWebPPMessage(websocketpp::connection_hdl hdl, message_ptr msg);

    // Call-back to notify us of a successful connection
    void OnWebPPOpen(websocketpp::connection_hdl hdl);

    // Call-back to notify us of a closed connection
    void OnWebPPClose(websocketpp::connection_hdl hdl);

    // Call-back to notify us of a failure in the connection
    void OnWebPPFail(websocketpp::connection_hdl hdl);

    /*
     * END: Callbacks from the WebPP client...
     */

    // TODO: Stand alone class, make it unit testable...
    // Store the active connection by handle
    class Connections {
    public:
        Connections(): nextHdl_(0) {}

        struct Item {
            std::shared_ptr<IOneShotConnectionConsumer> consumer;
            size_t hdl;
            const void* connHdl;
        };

        size_t NewConn(
                std::shared_ptr<IOneShotConnectionConsumer> com,
                const void* connHdl)
        {
            size_t hdl = nextHdl_++;
            conns_.push_back(Item{com, hdl, connHdl});
            return hdl;
        }

        bool ReleaseConn(size_t hdl) {
            bool released = false;
            for ( auto it = conns_.begin();
                 !released && it < conns_.end();
                 ++it)
            {
                const auto& item = (*it);
                if (item.hdl == hdl) {
                    conns_.erase(it);
                    released = true;
                }
            }
            return released;
        }

        bool ReleaseConn(Client::connection_ptr hdl) {
            bool released = false;
            for ( auto it = conns_.begin();
                  !released && it < conns_.end();
                  ++it)
            {
                const auto& item = (*it);
                if (item.connHdl == hdl.get()) {
                    conns_.erase(it);
                    released = true;
                }
            }
            return released;
        }

        std::shared_ptr<IOneShotConnectionConsumer>
                GetConsumer(Client::connection_ptr hdl)
        {
            std::shared_ptr<IOneShotConnectionConsumer> result(nullptr);
            for ( auto it = conns_.begin();
                  result.get() == nullptr  && it < conns_.end();
                  ++it)
            {
                const auto& item = (*it);
                if (item.connHdl == hdl.get()) {
                    conns_.end();
                    result = item.consumer;
                }
            }
            return result;
        }

    private:
        size_t nextHdl_;
        // TODO: Reserve (or similar...)
        std::vector<Item> conns_;
    };

    Connections conns_;
};


#endif //CPPWEBSOCKETRESPONSEREQUEST_WEBPPCLIENT_H
