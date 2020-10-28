#include "sNet/network.hpp"
int main(void)
{
    sNet::server s;
    std::vector<float> v = {1,2.5,3,4,5};

    s.Send(2, 125, 33, 45.5);
    s.Send(3, 5, 5, 5.5);
    s.Send(7, v);
    s.RegisterHandle([&]() 
    {
        // Ska implementeras från recv
    }, 7);
    auto a = s.Poll<int, int, int, double>(2);
    auto hm = s.PollVec<float>(7);
    std::cout << std::get<3>(a) << std::endl;
    a = s.Poll<int, int, int, double>(3);
    std::cout << std::get<0>(a) << std::endl;
    sNet::client("192.168.1.1", 80);
    for(auto &s : hm)
    {
        std::cout << s << " ";
    }
    std::cout << "\n";
    return 0;
}