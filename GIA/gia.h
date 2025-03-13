#pragma once

#include <boost/asio.hpp>

namespace gia
{
    typedef boost::asio::ip::tcp b_tcp;

    static std::string make_http_response(const std::string& context);
    
    class session : public std::enable_shared_from_this<session>
    {
    public:
        session(b_tcp::socket socket) : m_socket(std::move(socket)) {}
        
        void start();

    private:
        b_tcp::socket m_socket ;
        boost::asio::streambuf m_buffer;
    };

    class server
    {
    public:
        server(boost::asio::io_context& io_context, int16_t port, int32_t buffer_size, int32_t thread_pool_size) :
            m_io_context(io_context),
            m_acceptor(io_context, b_tcp::endpoint(b_tcp::v4(), port)),
            m_thread_pool_size(thread_pool_size)
        {
            m_acceptor.set_option(b_tcp::acceptor::reuse_address(true));
            m_acceptor.set_option(boost::asio::socket_base::receive_buffer_size(buffer_size));
            m_acceptor.set_option(boost::asio::socket_base::send_buffer_size(buffer_size));
            
            start_accept();
        }

        ~server()
        {
            m_acceptor.close();
            m_io_context.stop();
        }
        
        void run();
        
    private:
        void start_accept();
        
        b_tcp::acceptor m_acceptor;
        boost::asio::io_context& m_io_context;
        int32_t m_thread_pool_size {0};
    };
}
