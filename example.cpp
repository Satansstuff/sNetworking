#include "sNet/network.hpp"
int main(void)
{
    sNet::server s;
    auto a = s.poll<int>(2);
    s.registerHandle([&](std::string s) 
    {
            // Dostuff
    }, 5);
    std::cout << std::get<0>(a) << std::endl;
        
    return 0;
    
}