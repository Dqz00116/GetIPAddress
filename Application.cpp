#include <iostream>
#include "GIA/gia.h"

constexpr int16_t PORT = 32700;
constexpr int32_t THREAD_POOL_SIZE = 8;
constexpr int32_t MAX_BUFFER_SIZE = 65536;

int main()
{
    auto config = gia::data::loadJson("./config.json");
    if (!config.has_value())
    {
        std::cerr << "Failed to load config file. " << "\n";
        return EXIT_FAILURE;
    }

    auto json = config.value();
    
    int16_t port = json.value("port", PORT);
    int32_t thread_pool_size = json.value("thread_pool_size", THREAD_POOL_SIZE);
    int32_t max_buffer_size = json.value("max_buffer_size", MAX_BUFFER_SIZE);
    
    try
    {
        boost::asio::io_context io_context;
        gia::server srv(io_context, port, max_buffer_size, thread_pool_size);
        srv.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}