#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <string.h>
namespace sNet
{
    //https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
    template <
    size_t Index = 0, // start iteration at 0 index
        typename TTuple,  // the tuple type
        size_t Size =
            std::tuple_size_v<
                std::remove_reference_t<TTuple>>, // tuple size
        typename TCallable, // the callable to bo invoked for each tuple item
        typename... TArgs   // other arguments to be passed to the callable 
    >
    void for_each(TTuple&& tuple, TCallable&& callable, TArgs&&... args)
    {
        if constexpr (Index < Size)
        {
            std::invoke(callable, args..., std::get<Index>(tuple));

            if constexpr (Index + 1 < Size)
                for_each<Index + 1>(
                    std::forward<TTuple>(tuple),
                    std::forward<TCallable>(callable),
                    std::forward<TArgs>(args)...);
        }
    }
    enum protocol
    {
        TCP,
        UDP
    };
    class server
    {
    private:
        std::unordered_map<short, std::string> rawdata;
        std::unordered_map<short, std::function<void(std::string)>> callbacks;

    public:
        server()
        {
            srand(time(NULL));
        }
        template<protocol s>
        bool launch()
        {

            
            return true;
        }
        template<typename... Args>
        void send(short key, Args... to_send)
        {
            std::stringstream ss;
            ss << key;
            (ss << ... << to_send);
            auto data = ss.str();
        }
        template <typename... Ts>
        std::tuple<Ts...> poll(short i)
        {
            std::tuple<Ts...> ret;
            //auto size = std::tuple_size<decltype(ret)>::value;
            //using type = typename std::tuple_element<0, std::tuple<Ts...>>::type;
            if(callbacks.find(i) != callbacks.end() || 
                rawdata[i].empty()) return ret;
            std::string parse(rawdata[i]);
            std::istringstream ss(parse);
            for_each(ret, [&](auto& item) 
            {
                ss >> item;
            });
            rawdata[i].clear();
            return ret;
        }
        void registerHandle(std::function<void(std::string)> func, short key)
        {
            callbacks[key] = func;
        }
    };
    class network
    {
    private:
        std::vector<server> servers;
    public:
        struct server_iterator
        {
            server srv;
            server_iterator(server &s)
            {
                srv = s;
            }
            server* operator->() &
            {
                return &srv;
            }
        };
        auto addServer()
        {
            servers.push_back(server());
            server_iterator ret(servers.back());
            return ret;
        }
    };
}