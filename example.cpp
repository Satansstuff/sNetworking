#include "sNet/network.hpp"
int main(void)
{
    sNet::server s;
    
    s.RegisterHandle([&](std::string s) 
    {
            // Dostuff
    }, 5);
    s.Send(2, 125, 33, 45.5);
    s.Send(5, 5, 5, 5.5);
    auto a = s.Poll<int, int, int, double>(2);
    
    std::cout << std::get<3>(a) << std::endl;
    a = s.Poll<int, int, int, double>(2);
    std::cout << std::get<0>(a) << std::endl;
    sNet::client("192.168.1.1", 80);
    return 0;
}