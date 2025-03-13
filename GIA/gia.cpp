#include "gia.h"

#include <iostream>

namespace gia
{
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
                    std::cerr << "Read error: " << ec.message() << std::endl;
                    return;
                }
                
                try
                {
                    std::string ip = m_socket.remote_endpoint().address().to_string();
                    std::string response = make_http_response(ip);
                    auto resp_ptr = std::make_shared<std::string>(std::move(response));
                    boost::asio::async_write(
                        m_socket, boost::asio::buffer(*resp_ptr),
                        [self, resp_ptr](boost::system::error_code ec, std::size_t)
                        {
                            if (!ec)
                            {
                                boost::system::error_code ignore_ec;
                                self->m_socket.shutdown(b_tcp::socket::shutdown_both, ignore_ec);
                            }
                        });
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            });
    }

    void server::run()
    {
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
                
                std::cerr << "Accept error: " << ec.message() << std::endl;
                start_accept();
                return;
            }
            
            std::make_shared<session>(std::move(socket))->start();
            start_accept();
        });
    }
}
