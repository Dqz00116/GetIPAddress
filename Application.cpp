#include <iostream>
#include "GIA/gia.h"

int main()
{
    const auto config = gia::data::loadJson("./config.json");
    if (!config.has_value())
    {
        std::cerr << "Failed to load config file. " << "\n";
        return EXIT_FAILURE;
    }

    const auto& json = config.value();
    
    try
    {
        boost::asio::io_context io_context;
        gia::server srv(io_context, json);
        srv.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}