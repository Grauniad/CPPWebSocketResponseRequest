#ifndef DEV_TOOLS_CPP_LIBS_WEBSOCKETS_HTTP_CLIENT_H__
#define DEV_TOOLS_CPP_LIBS_WEBSOCKETS_HTTP_CLIENT_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <sstream>
#include <iostream>

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

/**
 * Represents the result of an http request
 */
struct HTTPMessage {
    HTTPMessage() : status_code(0) { }

    unsigned int status_code;
    std::stringstream content;
    std::map<std::string,std::string> headers;
};

class AsyncHTTPSClient
{
public:
    typedef std::map<std::string, std::string> HeaderMap;
protected:

    enum REQUEST_MODE {
        REQUEST_MODE_GET,
        REQUEST_MODE_POST
    };

    /** 
     * Create a pending HTTP request
     *
     * The request can be sent by calling StartRequest
     *
     * @param io_service   The IO service to run the request on
     * @param server       The server to send the request to
     * @param path         The path on the server to request
     *
     */ 
    AsyncHTTPSClient(
                  REQUEST_MODE mode,
                  boost::asio::io_service& io_service,
                  const std::string& server,
                  const std::string& path,
                  const HeaderMap& headers,
                  const std::string& data);

    void StartRequest();

    virtual void OnComplete(HTTPMessage& msg) = 0;

    /**
     * Access to the message being read. 
     *
     * Use before OnComplete is undefined, and stupid.
     */
    const HTTPMessage& GetMessage() { return message; }

private:
    /** CALLBACK: Triggered by sync_handshake
     * ----------------------------------------
     *  TODO
     *
     */
    void handle_handshake(const boost::system::error_code& error);

    /** CALLBACK: Verify an ssl cert
     * ----------------------------------------
     *  TODO
     *
     */
    bool verify_certificate(
            bool preverified,
            boost::asio::ssl::verify_context& ctx);

    /** CALLBACK: Triggered from async_resolve
     * ----------------------------------------
     *
     * ASYNCIO managed to resolve our address, and we have a 
     * set of target servers to hit. Iterate through them, attempting to
     * connect to each.
     *
     * @param err                An error, if the async_resolve failed
     * @param endpoint_iterator  Iterates over the resolved servers
     */
    void handle_resolve(
            const boost::system::error_code& err,
            tcp::resolver::iterator endpoint_iterator);

    /** CALLBACK: Triggered from async_connect
     * ----------------------------------------
     *
     * We have successfully connected to a server, send the request
     * 
     * @param err  An error, if the async_connect failed
     */
    void handle_connect(const boost::system::error_code& err);

    /** CALLBACK: Triggered from async_write
     * ----------------------------------------
     *
     * Connection is ready for us to do a write - send the request
     * 
     * @param err  An error, if the async_write failed
     */
    void handle_write_request(const boost::system::error_code& err);

    /** CALLBACK: Triggered from async_read_until
     * -------------------------------------------
     *
     * Handles the initial response line, then start reading headers by
     * triggering handle_read_headers
     * 
     * @param err  An error, if the async_read_until failed
     */
    void handle_read_status_line(const boost::system::error_code& err);

    /** CALLBACK: Triggered from async_read_until
     * -------------------------------------------
     *
     * Called to read the headers, after a successful call to read_status_line.
     *
     * Once all headers have been read, start reading the content
     * 
     * @param err  An error, if the async_read_until failed
     */
    void handle_read_headers(const boost::system::error_code& err);

    /** CALLBACK: Triggered from async_read_until
     * -------------------------------------------
     *
     * Called to read the content, after a successful call to handle_read_headers.
     *
     * Once all headers have been read, start reading the content
     * 
     * @param err  An error, if the async_read_until failed
     */
    void handle_read_content(const boost::system::error_code& err);


    /**
     * DATA
     */
    HTTPMessage message;
    tcp::resolver resolver_;
    ssl::context ctx;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    const std::string server_;
};

#endif
