#include <iostream>
#include "GIA/gia.h"
constexpr int16_t PORT = 32700;
constexpr int32_t THREAD_POOL_SIZE = 8;
constexpr int32_t MAX_BUFFER_SIZE = 65536;

int main()
{
    try
    {
        boost::asio::io_context io_context;
        gia::server srv(io_context, PORT, MAX_BUFFER_SIZE, THREAD_POOL_SIZE);
        
        std::cout << "Server started on port " << PORT 
                  << " with " << THREAD_POOL_SIZE << " worker threads\n";
             
        srv.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}