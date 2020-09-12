#include "network.h"

#include "utils.h"

#include <iostream>

network::Request::Request(boost::asio::streambuf &&buffer)
{
    std::istream stream(&buffer);
    stream >> method >> urlPath;
}

const std::string &network::Request::GetMethod() const
{
    return method;
}

const std::string &network::Request::GetUrlPath() const
{
    return urlPath;
}

network::Response::Response(status code_, const std::string &body_, const std::string &contentType_)
    : code(code_), body(body_), contentType(contentType_)
{
}

network::Response &network::Response::operator=(network::Response&& resp_)
{
    this->code = resp_.code;
    this->body = std::move(resp_.body);
    this->contentType = std::move(resp_.contentType);
    return *this;
}

boost::asio::streambuf &network::Response::GetHTTPResponse(bool withBody)
{
    std::iostream stream(&buffer);
    stream.clear();

    stream << "HTTP/1.1 " << static_cast<size_t>(code) << " " << code << httpEndl;
    stream << "Server: Highload Static Server" << httpEndl;
    stream << "Date: " << utils::GetTimeNow() << httpEndl;
    stream << "Connection: Keep-Alive" << httpEndl;
    if (!contentType.empty() || !body.empty())
    {
        stream << "Content-Length: " << body.size() << httpEndl;
        stream << "Content-Type: " << contentType << httpEndl;
    }
    stream << httpEndl;
    if (withBody) 
    {
        stream << body;
    }

    return buffer;
}

network::status network::Response::GetStatusCode()
{
    return code;
}