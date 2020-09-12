#include "server.h"

#include "utils.h"
#include "network.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/asio/signal_set.hpp>
#include <boost/bind.hpp>

namespace
{

class Session final : public std::enable_shared_from_this<Session>
{
 public:
    Session(boost::asio::io_context &ioContext_, boost::asio::ip::tcp::socket &&socket_)
        : strand(ioContext_.get_executor()), socket(std::move(socket_))
    {
        boost::asio::socket_base::keep_alive option(true);
        socket.set_option(option);
    }

    ~Session()
    {
        Stop();
    }

	void Start(store::FileStore &fileStore) 
	{
        boost::system::error_code ec;
		auto endpoint = socket.remote_endpoint(ec);
        std::string remote = endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
        if (ec)
        {
            BOOST_LOG_TRIVIAL(error) << "bad socket descriptor or socket is not connected error: " << ec.message() << ", errno: " << ec.value();
            Stop();
            return;
        }
        BOOST_LOG_TRIVIAL(info) << "remote endpoint: " << remote;

        boost::asio::spawn(strand, 
            [this, self = shared_from_this(), remote, &fileStore] (auto yield) 
            {
                while (socket.is_open())
                {
                    try
                    {
                        boost::asio::streambuf buffer;
                        boost::asio::async_read_until(socket, buffer, network::httpEndl, yield);
                        network::Request req(std::move(buffer));
                        
                        std::string path = utils::MakePathByUrl(req.GetUrlPath());
                        network::Response resp;
                        if (req.GetMethod() == network::get || req.GetMethod() == network::head)
                        {
                            auto [status, content, fileType] = fileStore.Get(path);
                            BOOST_LOG_TRIVIAL(debug) << "get file with status: " << static_cast<size_t>(status) << ", type: " << fileType;
                            switch (status)
                            {
                            case store::status::ok:
                            {
                                resp = network::Response(network::status::ok, content, utils::GetContentType(fileType));
                                break;
                            }
                            case store::status::forbidden:
                            {
                                resp = network::Response(network::status::forbidden);
                                break;
                            }
                            case store::status::notFound:
                            {
                                resp = network::Response(network::status::not_found);
                                break;
                            }
                            default:
                                resp = network::Response(network::status::bad_request);
                            }
                        }
                        else
                        {
                            resp = network::Response(network::status::method_not_allowed);
                        }

                        boost::asio::async_write(socket, resp.GetHTTPResponse(), yield);
                        BOOST_LOG_TRIVIAL(info) << "write '" << resp.GetStatusCode() << "' response to socket for method " 
                                << req.GetMethod() << ", path: " << path;
                    }
                    catch (const boost::system::system_error &e)
                    {
                        BOOST_LOG_TRIVIAL(error) << "remote: " << remote << ", error: " << e.what() << ", errno: " << e.code().value();
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
 	boost::asio::strand<boost::asio::io_context::executor_type> strand;
 	boost::asio::ip::tcp::socket socket;
};

} // namespace

StaticServer::StaticServer(const config::Settings& settings_) noexcept
	: settings(settings_), fileStore(settings_.globalPath)
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

        while(!ioContext.stopped())
        {
            ec.clear();
            boost::asio::ip::tcp::socket socket(ioContext);
            acceptor.async_accept(socket, yield[ec]);
            if (!ec)
            {
             	std::make_shared<Session>(ioContext, std::move(socket))->Start(fileStore);
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
    while(!ioContext.stopped()) 
    {
        //boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM, SIGQUIT);
        //signals.async_wait(boost::bind(&StaticServer::Stop, this));
    }

	return EXIT_SUCCESS;
}

void StaticServer::Stop()
{
    ioContext.stop();
    for (auto &worker: workers)
    {
        worker.join();
    }
    BOOST_LOG_TRIVIAL(info) << "stop static server";
}