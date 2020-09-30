#include "sNet/network.hpp"
int main(void)
{
    sNet::server s;
    
    s.registerHandle([&](std::string s) 
    {
            // Dostuff
    }, 5);
    s.send(2, 125, 33, 45.5);
    auto a = s.poll<int, int, int, double>(2);

    std::cout << std::get<0>(a) << std::endl;

    std::cout << std::get<0>(a) * 2 << std::endl;

    return 0;
}