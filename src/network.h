#pragma once

#include <iostream>
#include <string>
#include <ctime>

#include <boost/beast/http/status.hpp>
#include <boost/asio/streambuf.hpp>

namespace network
{

const static std::string get = "GET";
const static std::string head = "HEAD";

const static std::string httpEndl = "\r\n";

using boost::beast::http::status;

class Request
{
public:
    explicit Request(boost::asio::streambuf &&buffer);

    const std::string &GetMethod() const;
    const std::string &GetUrlPath() const;

private:
    std::string method;
    std::string urlPath;
};

class Response
{
public:
    Response() = default;
    Response &operator=(Response&&);

    explicit Response(status code_, const std::string &body_ = {}, const std::string &contentType_ = {});

    boost::asio::streambuf &GetHTTPResponse();
    status GetStatusCode();

private:
    status code;
    std::string body;
    std::string contentType;
    boost::asio::streambuf buffer;
};

} // network