//
// async_client.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "http_client.h"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>

AsyncHTTPSClient::AsyncHTTPSClient(
        REQUEST_MODE mode,
        boost::asio::io_service& io_service,
        const std::string& server, 
        const std::string& path,
        const HeaderMap& headers,
        const std::string& data)
    : resolver_(io_service),
      ctx(boost::asio::ssl::context::tlsv12),
      socket_(io_service, ctx),
      server_(server)
{
   ctx.set_default_verify_paths();

    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback(
        boost::bind(&AsyncHTTPSClient::verify_certificate, this, _1, _2));

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::stringstream request_stream;
    if ( mode == REQUEST_MODE_GET ) {
        request_stream << "GET " << path << " HTTP/1.1\r\n";
    } else {
        request_stream << "POST " << path << " HTTP/1.1\r\n";
    }
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    if ( mode == REQUEST_MODE_POST) {
        request_stream << "content-type: application/json\r\n";
        request_stream << "content-length: " << data.length() << "\r\n";
    }
    for (const auto& item: headers) {
        request_stream << item.first << ": " << item.second << "\r\n";
    }
    request_stream << "Connection: close\r\n";
    request_stream << "\r\n";
    if ( data != "" ) {
        request_stream << data;
    }

    std::string msg = request_stream.str();
    std::cout << "Message: " << msg << std::endl;
    std::ostream frequest_stream(&request_);
    frequest_stream << msg;

}

void AsyncHTTPSClient::StartRequest() {
    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    tcp::resolver::query query(server_, "https");
    resolver_.async_resolve(query,
            boost::bind(&AsyncHTTPSClient::handle_resolve, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::iterator));
}


bool AsyncHTTPSClient::verify_certificate(bool preverified,
        boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

    return preverified;
}

void AsyncHTTPSClient::handle_resolve(const boost::system::error_code& err,
        tcp::resolver::iterator endpoint_iterator)
{
    if (!err)
    {
        // Attempt a connection to each endpoint in the list until we
        // successfully establish a connection.
        boost::asio::async_connect(
                socket_.lowest_layer(),
                endpoint_iterator,
                boost::bind(&AsyncHTTPSClient::handle_connect, this,
                    boost::asio::placeholders::error));
    }
    else
    {
        std::cout << "Error (resolve): " << err.message() << "\n";
    }
}

void AsyncHTTPSClient::handle_connect(const boost::system::error_code& err)
{
    if (!err)
    {

      socket_.async_handshake(boost::asio::ssl::stream_base::client,
                boost::bind(&AsyncHTTPSClient::handle_handshake, this,
                            boost::asio::placeholders::error));
    }
    else
    {
        std::cout << "Error (connect): " << err.message() << "\n";
    }
}

void AsyncHTTPSClient::handle_handshake(const boost::system::error_code& error)
{
    if (!error)
    {
        // The connection was successful. Send the request.
        boost::asio::async_write(socket_, request_,
                boost::bind(&AsyncHTTPSClient::handle_write_request, this,
                    boost::asio::placeholders::error));
    }
    else
    {
        std::cout << "Handshake failed: " << error.message() << "\n";
    }
}

void AsyncHTTPSClient::handle_write_request(const boost::system::error_code& err)
{
    if (!err)
    {
        // Read the response status line. The response_ streambuf will
        // automatically grow to accommodate the entire line. The growth may be
        // limited by passing a maximum size to the streambuf constructor.
        boost::asio::async_read_until(socket_, response_, "\r\n",
                boost::bind(&AsyncHTTPSClient::handle_read_status_line, this,
                    boost::asio::placeholders::error));
    }
    else
    {
        std::cout << "Error (request): " << err.message() << "\n";
    }
}

void AsyncHTTPSClient::handle_read_status_line(const boost::system::error_code& err)
{
    if (!err)
    {
        // Check that response is OK.
        std::istream response_stream(&response_);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            std::cout << "Invalid response\n";
            return;
        }
        if (status_code != 200)
        {
            std::cout << "Response returned with status code ";
            std::cout << status_code << "\n";
            return;
        }

        // Read the response headers, which are terminated by a blank line.
        boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
                boost::bind(&AsyncHTTPSClient::handle_read_headers, this,
                    boost::asio::placeholders::error));
    }
    else
    {
        std::cout << "Error (status line): " << err.message() << "\n";
    }
}

void AsyncHTTPSClient::handle_read_headers(const boost::system::error_code& err)
{
    if (!err)
    {
        // Process the response headers.
        std::istream response_stream(&response_);
        std::string header;
        while (std::getline(response_stream, header) && header != "\r") {
            auto pos = header.find(":");
            if ( pos != std::string::npos) {
                std::string header_name = header.substr(0,pos);
                // Strip ": " 
                std::string header_value = header.substr(pos+2);
                message.headers[header_name] = header_value;
            }
        }

        // Write whatever content we already have to output.
        if (response_.size() > 0)
            message.content << &response_;

        // Start reading remaining data until EOF.
        boost::asio::async_read(socket_, response_,
                boost::asio::transfer_at_least(1),
                boost::bind(&AsyncHTTPSClient::handle_read_content, this,
                    boost::asio::placeholders::error));
    }
    else
    {
        std::cout << "Error (headers): " << err.message() << "\n";
    }
}

void AsyncHTTPSClient::handle_read_content(const boost::system::error_code& err)
{
    if (!err)
    {
        // Write all of the data that has been read so far.
        message.content << &response_;

        // Continue reading remaining data until EOF.
        boost::asio::async_read(socket_, response_,
                boost::asio::transfer_at_least(1),
                boost::bind(&AsyncHTTPSClient::handle_read_content, this,
                    boost::asio::placeholders::error));
    }
    else
    {
        OnComplete(message);
    }

}
