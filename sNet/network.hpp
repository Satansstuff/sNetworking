#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <sstream>
#include <string.h>
#include <thread>
#include <numeric>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 


/*
    TODO:
    Too many mutex?

*/
namespace sNet
{
    //Defines
    constexpr char emptypacket[] = "nr";
    constexpr size_t chunksize = 2048;





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
    class client
    {

    public:
        bool connect(const char* ip, unsigned short port)
        {
            return true;
        }
        template<typename ...Args>
        bool send(unsigned key, Args... to_send)
        {
            std::stringstream ss;
            ss << key << " ";
            ((ss << to_send << " "), ...);
            (ss << "<>");
            auto data = ss.str();
            return true;
        }
        bool send(std::string derp)
        {
            derp += "<>";
            return true;
        }

    };
    class server
    {
    private:
        bool isRunning = false;
        mutable std::mutex dmutex;
        mutable std::mutex tsend;



        std::unordered_map<unsigned short, client> clients;
        std::unordered_map<unsigned short, std::string> rawdata;
        std::unordered_map<unsigned short, std::function<void(std::string)>> callbacks;
        std::vector<char*> to_send;
        std::thread *thr = nullptr;

        auto getData(unsigned i) &
        {
            //Måste veta om det inns mer än ett paket där
            //Returnera C_str?
            //Som i clearData
        
            std::lock_guard<std::mutex> lock_guard_name(dmutex);
            auto raw = rawdata[i];
            auto size = raw.find("<>");
            if(size == std::string::npos) return std::string(emptypacket);
            return raw.substr(0, size - 1);
        }
        /*
            Untested af

        */
        void serverWriteData(int index, char* data)
        {
            std::lock_guard<std::mutex> lock_guard_name(dmutex);
            std::string dat(data);
            std::string work;
            size_t len = 0;
            size_t itlen = dat.length();
            while(dat.length() > 0)
            {
                
                for(unsigned i = 0; i < itlen; i++)
                {
                    if(dat[i] == '<' && dat[i + 1] == '>' )
                    {
                        len = i + 2;
                        work.push_back('<');
                        work.push_back('>');
                        break;
                    }
                    work.push_back(dat[i]);
                }
                dat.erase(0, len);
                itlen = dat.length();
                auto key = work[0] - '0';
                if(std::isdigit(key))
                {
                    rawdata[key] += work;
                }
                work.clear();
            }

            // Hitta antalet paket och dela upp dem till rätt ställe

        }
        void addToSend(char* data)
        {
            std::lock_guard<std::mutex> lock_guard_name(tsend);
            to_send.push_back(data);
        }
        void clearData(unsigned index)
        {
            std::lock_guard<std::mutex> lock_guard_name(dmutex);
            if(rawdata[index].size() == 0) return;
            size_t packet = rawdata[index].find("<>");
            if(packet != std::string::npos)
            {
                rawdata[index].erase(0, packet + 2);
            }
            /*else
            {
                Potential vuln 
                rawdata[index].clear();
            }*/
        }
    public:
        server(const server &other)
        {
            std::lock_guard<std::mutex>(other.dmutex);
            std::lock_guard<std::mutex>(other.tsend);
        }
        server()
        {
            srand(time(NULL));
        }
        size_t getWaitingData(unsigned index)
        {
            std::lock_guard<std::mutex> lock_guard_name(dmutex);
            auto data = rawdata[index];
            size_t counter = 0;
            size_t start = 0;
            if((start = data.find("<>")) == std::string::npos) return 0;
            counter++;
            for(unsigned i = start + 1; i < data.size();)
            {
                auto pos = data.find("<>");
                pos != std::string::npos ? counter++ : i = data.size() + 1;
                i = pos + 1;
            }
            return counter;
        }
        template<protocol s>
        bool launch(unsigned short port, const char* ip = "0.0.0.0", unsigned short tickrate = 64)
        {
            bool launched = false;
            if constexpr(s == TCP)
            {
                thr = new std::thread([&]()
                {
                    char inbuffer[chunksize];
                    int sock_h, valread, opt = 1;
                    struct sockaddr_in address;
                    auto addrlen = sizeof(address);
                    if ((sock_h = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
                    { 
                        std::cerr << "Failed to open server-socket \n";
                        isRunning = false;
                        launched = false;
                        return;
                    }
                    if (setsockopt(sock_h, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                            &opt, sizeof(opt))) 
                    { 
                        std::cerr << "Failed to set sockopt \n";
                        isRunning = false;
                        launched = false;
                        return;
                    }
                    address.sin_family = AF_INET; 
                    address.sin_addr.s_addr = inet_addr(ip);
                    address.sin_port = htons(port);
                    if (bind(sock_h, (struct sockaddr *)&address,  
                        sizeof(address))<0) 
                    { 
                        std::cerr << "Failed to bind to IP/PORT \n";
                        isRunning = false;
                        launched = false;
                        return;
                    }
                    if (listen(sock_h, 5) < 0) 
                    { 
                        std::cerr << "Failed to listen \n";
                        isRunning = false;
                        launched = false;
                        return;
                    } 

                    double updaterate = 1.0 / (tickrate);
                   
                    /*int listenfd = 0, connfd = 0;
                    struct sockaddr_in serv_addr;
                    auto listenfd = socket(AF_INET, SOCK_STREAM, 0);*/
                    while(isRunning)
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(updaterate));
                    }
                });
                thr->detach();
            }
            else
            {
                
            }
            return launched;
        }
        inline void shutDown()
        {
            isRunning = false;
        }
        /*
            TODO:
                Vilken klient ska detta skickas till? 
                Alla?

        */
        template<typename... Args>
        void send(unsigned key, Args... to_send)
        {
            std::stringstream ss;
            ss << key << " ";
            ((ss << to_send << " "), ...);
            (ss << "<>");
            auto data = ss.str();
            //std::cout << data << std::endl;
            addToSend((char*)data.c_str());
            rawdata[2] += data;
        }
        template <typename... Ts>
        std::tuple<Ts...> poll(short i)
        {
            std::tuple<Ts...> ret;
            //auto size = std::tuple_size<decltype(ret)>::value;
            if(callbacks.find(i) != callbacks.end() || 
                getData(i).empty()) return ret;
            std::string parse(getData(i));
            std::istringstream ss(parse);
            for_each(ret, [&](auto& item) 
            {
                ss >> item;
            });
            //Hmmmm?
            clearData(i);
            return ret;
        }
        inline void registerHandle(std::function<void(std::string)> func, short key)
        {
            if(callbacks.find(key) != callbacks.end()) callbacks[key] = func;
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