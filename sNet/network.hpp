#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <sstream>
#include <string.h>
#include <thread>
namespace sNet
{

    //https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
    // ^ <3
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
    template<protocol T>
    class client
    {
        
    };
    class server
    {
    private:

        mutable std::mutex mutex;
        mutable std::mutex tsend;
        mutable std::mutex remove;
        /*
            Race condition med poll och callback. måste använda ett lås
            för att läsa och skriva. Kanske skriva en wrapper över mappen?
        */
        std::unordered_map<unsigned short, std::string> rawdata;
        std::unordered_map<unsigned short, std::function<void(std::string)>> callbacks;
        std::vector<char*> to_send;
        std::thread *thr = nullptr;

        auto getData(unsigned i) &
        {
            //Måste veta om det inns mer än ett paket där
            //Returnera C_str?
            //Som i clearData
        
            std::lock_guard<std::mutex> lock_guard_name(mutex);
            return rawdata[i];
        }
        void addToSend(char* data)
        {
            std::lock_guard<std::mutex> lock_guard_name(tsend);
            to_send.push_back(data);
        }
        void clearData(unsigned index)
        {
            std::lock_guard<std::mutex> lock_guard_name(remove);
            size_t s = 0;
            for(unsigned i = 0; i < rawdata[i].size() + 1; i++)
            {
                if(rawdata[index][i] == '<' && rawdata[index][i + 1] == '>')
                {
                    rawdata[index].erase(0, s + 1);
                    return;
                }
            }
            rawdata[index].clear();
            return;
        }
    public:
        server(const server &other)
        {
            std::lock_guard<std::mutex>(other.mutex);
            std::lock_guard<std::mutex>(other.tsend);
            std::lock_guard<std::mutex>(other.mutex);
        }
        server()
        {
            srand(time(NULL));
            rawdata[2] = "2 5 2.15";
        }
        template<protocol s>
        bool launch()
        {
            if constexpr(s == TCP)
            {
                thr = new std::thread([&]()
                {
                    rawdata[5] = "fok";
                    std::cout << "whatthefuck?" << std::endl;
                });
                thr->detach();
                
            }
            else
            {
                
            }
            return true;
        }
        template<typename... Args>
        void send(unsigned short key, Args... to_send)
        {
            std::stringstream ss;
            ss << key;
            (ss << ... << to_send);
            (ss << "<>");
            auto data = ss.str();

        }
        template <typename... Ts>
        std::tuple<Ts...> poll(short i)
        {
            std::tuple<Ts...> ret;
            //auto size = std::tuple_size<decltype(ret)>::value;
            //using type = typename std::tuple_element<0, std::tuple<Ts...>>::type;
            if(callbacks.find(i) != callbacks.end() || 
                getData(i).empty()) return ret;
            std::string parse(getData(i));
            std::istringstream ss(parse);
 //           unsigned counter = 0;
            for_each(ret, [&](auto& item) 
            {
                ss >> item;
            });
            //Hmmmm?
            clearData(i);
            return ret;
        }
        void registerHandle(std::function<void(std::string)> func, short key)
        {
            callbacks[key] = func;
        }
        ~server()
        {
            if(thr != nullptr && thr->joinable())
            {
                thr->join();
                delete thr;
                thr = nullptr;
            }
        }
    };
    class network
    {
    private:
        std::vector<server> servers;
    public:
        auto addServer()
        {
            servers.push_back(server());
            return &servers.back();
        }
    };
}