#include "server.h"

#include <istream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

struct Request
{
    explicit Request(boost::asio::streambuf &&buffer)
    {
        std::istream stream(&buffer);
        stream >> method >> urlPath;
    }

    std::string method;
    std::string urlPath;

    inline static std::string until = "\r\n";
    inline static std::string get = "GET";
    inline static std::string head = "HEAD";
};

namespace
{

class Session final : public std::enable_shared_from_this<Session>
{
 public:
    Session(boost::asio::io_context &ioContext_, boost::asio::ip::tcp::socket &&socket_)
        : ioContext(ioContext_), socket(std::move(socket_))
    {
    }

    ~Session()
    {
        Stop();
    }

	void Start() 
	{
        boost::system::error_code ec;
		auto endpoint = socket.remote_endpoint(ec);
        std::string remote = endpoint.address().to_string() +  ':' + std::to_string(endpoint.port());
        if (ec)
        {
            BOOST_LOG_TRIVIAL(error) << "bad socket descriptor or socket is not connected error: " << ec.message() << ", errno: " << ec.value();
            Stop();
            return;
        }
        BOOST_LOG_TRIVIAL(info) << "remote endpoint: " << remote;

        boost::asio::spawn(ioContext, 
            [this, self = shared_from_this(), remote] (auto yield) 
            {
                while (socket.is_open())
                {
                    try
                    {
                        boost::asio::streambuf buffer;
                        boost::asio::async_read_until(socket, buffer, Request::until, yield);
                        Request req(std::move(buffer));

                        BOOST_LOG_TRIVIAL(info) << "receive request with method: " << req.method << ", path: " << req.urlPath;

                        //boost::asio::async_write(socket, buffer, yield);
                    }
                    catch (const boost::system::system_error &e)
                    {
                        BOOST_LOG_TRIVIAL(error) << "remote: " << remote << ", error: " << e.what()
                            << ", errno: " << e.code().value();
                        Stop();
                    }
                }

            }
        );
	}

    void Stop()
    {
        if (socket.is_open())
        {
            boost::system::error_code ec;
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket.close(ec);
            BOOST_LOG_TRIVIAL(info) << "close socket";
        }
    }

 private:
 	boost::asio::io_context &ioContext;
 	boost::asio::ip::tcp::socket socket;
};

}// namespace

StaticServer::StaticServer(const config::Settings& settings_) noexcept
	: settings(settings_) 
{}

StaticServer::~StaticServer()
{
	Stop();
}

int StaticServer::Start()
{
    boost::asio::spawn(ioContext, [&](boost::asio::yield_context yield) {

        boost::asio::ip::tcp::acceptor acceptor(ioContext, {boost::asio::ip::tcp::v4(), settings.port});
        boost::system::error_code ec;
        acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
        	BOOST_LOG_TRIVIAL(error) << "couldn't set SO_REUSEADDR: " << ec.message() << ", errno: " << ec.value();
            return;
        }

        for (;;)
        {
            ec.clear();
            boost::asio::ip::tcp::socket socket(ioContext);
            acceptor.async_accept(socket, yield[ec]);

            if (!ec)
            {
             	std::make_shared<Session>(ioContext, std::move(socket))->Start();
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "error: " << ec.message() << ", errno: " << ec.value();
            }
        }

    });

    for (size_t i = 0; i < settings.workersCount; ++i)
    {
    	workers.emplace_back([this]() {
            try
            {
                ioContext.run();
            }
            catch (const boost::system::system_error &e)
            {
                BOOST_LOG_TRIVIAL(error) << "ioContext error: " << e.what() << ", errno: " << e.code().value();
            }
        });
    }

	BOOST_LOG_TRIVIAL(info) << "run stat server with " << settings.workersCount << " worker threads on port: " << settings.port;

    while(true) {};

	return EXIT_SUCCESS;
}

void StaticServer::Stop()
{
	ioContext.stop();
    for (auto &worker: workers)
    {
        worker.join();
    }
}