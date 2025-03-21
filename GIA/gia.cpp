#include "gia.h"

#include <iostream>
#include <fstream>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

constexpr int16_t PORT = 32700;
constexpr int32_t THREAD_POOL_SIZE = 8;
constexpr int32_t MAX_BUFFER_SIZE = 65536;

namespace gia
{
    namespace data
    {
        std::optional<nlohmann::json> loadJson(const char* path)
        {
            std::ifstream f(path);
            if (!f) 
            {
                return std::nullopt;
            }
            return nlohmann::json::parse(f);       
        }
    }

    std::string make_http_response(const std::string& context)
    {
        const std::string response_body = context;
        const std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: " + std::to_string(response_body.size()) + "\r\n"
        "\r\n";
        
        return headers + response_body;
    }

    void session::start()
    {
        auto self = shared_from_this();
        boost::asio::async_read_until(
            m_socket, m_buffer, "\r\n\r\n",
            [this, self](boost::system::error_code ec, std::size_t)
            {
                if (ec)
                {
                    BOOST_LOG_TRIVIAL(error) << "Read error: " << ec.message();
                    return;
                }
                
                try
                {
                    const auto ip = m_socket.remote_endpoint().address().to_string();
                    const bool bInWhiteList = self->m_map_ref.contains(ip);
                    const auto context = std::to_string(bInWhiteList);    
                    std::string response = make_http_response(context);
                    
                    auto resp_ptr = std::make_shared<std::string>(std::move(response));
                    boost::asio::async_write(
                        m_socket, boost::asio::buffer(*resp_ptr),
                        [self, resp_ptr, bInWhiteList](boost::system::error_code ec, std::size_t)
                        {
                            if (!ec)
                            {
                                BOOST_LOG_TRIVIAL(info) << "Receive request from "
                                    << self->m_socket.remote_endpoint().address().to_string() << "."
                                    << " IsWhileList: " << bInWhiteList;
                                
                                boost::system::error_code ignore_ec;
                                self->m_socket.shutdown(boost_tcp::socket::shutdown_both, ignore_ec);
                            }
                        });
                }
                catch (const std::exception& e)
                {
                    BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
                }
            });
    }

    void server::setup(const nlohmann::json& config)
    {
        m_port = config.value("port", PORT);
        m_thread_pool_size = config.value("thread_pool_size", THREAD_POOL_SIZE);
        m_max_buffer_size = config.value("max_buffer_size", MAX_BUFFER_SIZE);
        m_white_list = config.value("white_list", std::set<std::string>{});
    }

    void server::init_logger() noexcept
    {
        boost::log::add_console_log(std::cout,
                boost::log::keywords::format = "[%TimeStamp%] [%Severity%]: %Message%"
            );
        
        boost::log::add_file_log(
            boost::log::keywords::file_name = "log_%N.log",
            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
            boost::log::keywords::format = "[%TimeStamp%] [%Severity%]: %Message%",
            boost::log::keywords::auto_flush = true
        );
        
        boost::log::add_common_attributes();
    }

    const std::set<std::string>& server::getWitheList()
    {
        return m_white_list;
    }

    void server::run()
    {

        BOOST_LOG_TRIVIAL(info) << "Server started on port " << m_port 
                 << " with " << m_thread_pool_size << " worker threads";
        
        std::vector<std::thread> threads;
        for(size_t i = 0; i < m_thread_pool_size; ++i)
        {
            threads.emplace_back([this] { m_io_context.run(); });
        }
        
        for (auto& t : threads)
        {
            if(t.joinable())
                t.join();
        }
    }
    
    void server::start_accept()
    {
        if (!m_acceptor.is_open())
            return; 
        
        m_acceptor.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
        {
            if (ec)
            {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return;
                }
                
                BOOST_LOG_TRIVIAL(error) << "Accept error: " << ec.message();
                start_accept();
                return;
            }
            
            std::make_shared<session>(std::move(socket), getWitheList())->start();
            start_accept();
        });
    }
}
