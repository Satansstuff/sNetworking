#include "sNet/network.hpp"
int main(void)
{
    sNet::server s;
    
    s.registerHandle([&](std::string s) 
    {
            // Dostuff
    }, 5);
    s.send(2, 125, 33, 45.5);
    s.send(5, 5, 5, 5.5);
    auto a = s.poll<int, int, int, double>(2);
    
    std::cout << std::get<0>(a) << std::endl;
    a = s.poll<int, int, int, double>(2);
    std::cout << std::get<0>(a) << std::endl;
    return 0;
}