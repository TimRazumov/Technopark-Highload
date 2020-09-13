#include "server.h"

#include "network.h"
#include "utils.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

namespace
{
class Session final : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context &ioContext_, boost::asio::ip::tcp::socket &&socket_)
        : strand(ioContext_.get_executor()), timer(ioContext_), socket(std::move(socket_))
    {
        boost::asio::socket_base::keep_alive option(true);
        socket.set_option(option);
    }

    ~Session()
    {
        Stop();
    }

    void Start(store::FileStore &fileStore, time_t sessionTimeoutMsec)
    {
        boost::system::error_code ec;
        auto endpoint = socket.remote_endpoint(ec);
        std::string remote = endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
        if (ec)
        {
            BOOST_LOG_TRIVIAL(error)
                << "bad socket descriptor or socket is not connected error: " << ec.message()
                << ", errno: " << ec.value();
            Stop();
            return;
        }
        BOOST_LOG_TRIVIAL(info) << "remote endpoint: " << remote;

        boost::asio::spawn(strand,
            [this, self = shared_from_this(), remote, sessionTimeoutMsec, &fileStore](auto yield) {
                // while (socket.is_open())
                //{
                try
                {
                    /*timer.expires_from_now(std::chrono::milliseconds(sessionTimeoutMsec));
                    timer.async_wait([this, self](auto ec)
                    {
                        if (!ec)
                        {
                            boost::system::error_code ignored;
                            socket.cancel(ignored);
                        }
                    });*/

                    boost::asio::streambuf buffer;
                    boost::asio::async_read_until(socket, buffer, network::httpEndl, yield);
                    network::Request req(std::move(buffer));

                    std::string fullPath =
                        fileStore.GetFullPath(utils::MakePathByUrl(req.GetUrlPath()));
                    network::Response resp;
                    if (req.GetMethod() == network::get || req.GetMethod() == network::head)
                    {
                        auto [status, content] = fileStore.Get(fullPath);
                        BOOST_LOG_TRIVIAL(debug)
                            << "load file with status: " << static_cast<size_t>(status)
                            << ", full path: " << fullPath;
                        switch (status)
                        {
                            case store::status::ok:
                            {
                                resp = network::Response(
                                    network::status::ok, content, utils::GetContentType(fullPath));
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

                    bool sendBody = req.GetMethod() == network::get;
                    boost::asio::async_write(socket, resp.GetHTTPResponse(sendBody), yield);
                    BOOST_LOG_TRIVIAL(info)
                        << "write '" << resp.GetStatusCode() << "' response to socket for method "
                        << req.GetMethod() << ", full path: " << fullPath;

                    // timer.cancel();
                }
                catch (const boost::system::system_error &e)
                {
                    BOOST_LOG_TRIVIAL(error) << "remote: " << remote << ", error: " << e.what()
                                             << ", errno: " << e.code().value();
                    Stop();
                }
                //}
            });
    }

    void Stop()
    {
        if (socket.is_open())
        {
            boost::system::error_code ignored;
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
            socket.close(ignored);
            BOOST_LOG_TRIVIAL(info) << "close socket";
        }
    }

private:
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::steady_timer timer;
    boost::asio::ip::tcp::socket socket;
};

}  // namespace

StaticServer::StaticServer(const config::Settings &settings_) noexcept
    : settings(settings_), fileStore(settings_.globalPath)
{
}

int StaticServer::Start()
{
    boost::asio::spawn(ioContext, [&](boost::asio::yield_context yield) {
        boost::asio::ip::tcp::acceptor acceptor(
            ioContext, {boost::asio::ip::tcp::v4(), settings.port});
        boost::system::error_code ec;
        acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            BOOST_LOG_TRIVIAL(error)
                << "couldn't set SO_REUSEADDR: " << ec.message() << ", errno: " << ec.value();
            return;
        }

        while (!ioContext.stopped())
        {
            ec.clear();
            boost::asio::ip::tcp::socket socket(ioContext);
            acceptor.async_accept(socket, yield[ec]);
            if (!ec)
            {
                std::make_shared<Session>(ioContext, std::move(socket))
                    ->Start(fileStore, settings.sessionTimeoutMsec);
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
                BOOST_LOG_TRIVIAL(error)
                    << "ioContext error: " << e.what() << ", errno: " << e.code().value();
            }
        });
    }

    boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM, SIGQUIT);
    signals.async_wait([this](const boost::system::error_code &ec, int signalNum) {
        if (!ec && !ioContext.stopped())
        {
            ioContext.stop();
            BOOST_LOG_TRIVIAL(info) << "stoped static server by signal " << signalNum;
        }
    });

    BOOST_LOG_TRIVIAL(info) << "run stat server with " << settings.workersCount
                            << " worker threads on port: " << settings.port;

    for (auto &worker: workers)
    {
        worker.join();
    }

    return EXIT_SUCCESS;
}