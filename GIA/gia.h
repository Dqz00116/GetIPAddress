#pragma once

#include "json.hpp"
#include <optional>
#include <set>
#include <boost/asio.hpp>

namespace gia
{
    namespace data
    {
        std::optional<nlohmann::json> loadJson(const char* path);    
    }
    
    typedef boost::asio::ip::tcp boost_tcp;

    static std::string make_http_response(const std::string& context);
    
    class session : public std::enable_shared_from_this<session>
    {
    public:
        session(boost_tcp::socket socket, const std::set<std::string>& white_liet) :
        m_socket(std::move(socket)), m_map_ref(white_liet) {}
        
        void start();

    private:
        boost_tcp::socket m_socket ;
        boost::asio::streambuf m_buffer;
        const std::set<std::string>& m_map_ref;
    };

    class server
    {
    public:
        explicit server(boost::asio::io_context& io_context, const nlohmann::json& config) : m_io_context(io_context), m_acceptor(io_context) 
        {
            setup(config);
            
            m_acceptor.open(boost_tcp::v4());
            m_acceptor.bind(boost_tcp::endpoint(boost_tcp::v4(), m_port));
            m_acceptor.listen(boost::asio::socket_base::max_listen_connections);
            
            m_acceptor.set_option(boost_tcp::acceptor::reuse_address(true));
            m_acceptor.set_option(boost::asio::socket_base::receive_buffer_size(m_max_buffer_size));
            m_acceptor.set_option(boost::asio::socket_base::send_buffer_size(m_max_buffer_size));
            
            init_logger();
            start_accept();
        } 

        ~server()
        {
            m_acceptor.close();
            m_io_context.stop();
        }

        const std::set<std::string>& getWitheList();
        void run();
        
    private:
        void init_logger() noexcept;
        void setup(const nlohmann::json& config);
        void start_accept();
        
        int16_t m_port {0};
        int32_t m_thread_pool_size {0};
        int32_t m_max_buffer_size {0};
        
        boost_tcp::acceptor m_acceptor;
        boost::asio::io_context& m_io_context;
        std::set<std::string> m_white_list;
    };
}
