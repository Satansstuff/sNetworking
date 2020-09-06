#include "sNet/network.hpp"

int main(void)
{
    sNet::network net;
    auto it = net.addServer();
    it->registerHandle([&](auto data) 
    {
       std::cout << data << std::endl;
    }, 3);
    it->send<int, int, std::string, int>(5, 3, 5, "derp", 7);
    auto a = it->poll<int>(2);
    std::cout << std::get<0>(a) << std::endl;
    it->launch<sNet::TCP>();
}