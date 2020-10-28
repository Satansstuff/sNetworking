#include "sNet/network.hpp"
int main(void)
{
    sNet::server s;
    std::vector<int> v = {1,2,3,4,5};
    s.RegisterHandle([&](std::string s) 
    {
            // Dostuff
    }, 5);
    s.Send(2, 125, 33, 45.5);
    s.Send(3, 5, 5, 5.5);
    s.Send(7, v);
    auto a = s.Poll<int, int, int, double>(2);
    std::vector<int> hm = s.PollVec<int>(7);
    std::cout << std::get<3>(a) << std::endl;
    a = s.Poll<int, int, int, double>(3);
    std::cout << std::get<0>(a) << std::endl;
    sNet::client("192.168.1.1", 80);
    for(auto &val : hm)
    {
        std::cout << val << " ";
    }
    std::cout << "\n";
    return 0;
}