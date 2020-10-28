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
#include <deque>
#include <algorithm>
#include <execution>
#include <future>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
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
    constexpr unsigned chunksize = 2048;

    /*struct net_error
    {
        std::string error = "";
        bool fatal = false;
        bool operator()()
        {
            return fatal;
        }
    };*/



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
        int sock = 0, valread = 0;
        struct sockaddr_in serv_addr;
        /*
            std string blir mycket realloc
        */
        std::string buffer;
        bool initialized = false;
        mutable std::mutex dmutex;
        std::unordered_map<unsigned short, std::string> rawdata;
        struct hostent *hostinfo;
        auto GetData(unsigned i) &
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
        void ClearData(unsigned index)
        {
            std::lock_guard<std::mutex> lock_guard_name(dmutex);
            if(rawdata[index].size() == 0) return;
            size_t packet = rawdata[index].find("<>");
            if(packet != std::string::npos)
            {
                rawdata[index].erase(0, packet + 1);
            }
            /*else
            {
                Potential vuln 
                rawdata[index].clear();
            }*/
        }

        /*
            Not threadsafe... One client per thread pl0x
        */
        bool Internal_send(unsigned short key, std::string &data)
        {
            for(unsigned i = 0; i < data.size(); i++)
            {
                buffer[i] = data[i];
                if(buffer.size() > chunksize)
                {
                    send(sock , buffer.c_str() , buffer.length() , 0 ); 
                    buffer.clear();
                }
            }
            ClearData(key);
            return true;
        }
        
        bool Init(std::string &ip, unsigned short &port)
        {
            buffer = std::string(chunksize);
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
            { 
                std::cerr << "Failed to create client socket\n";
                return false; 
            }
            /*
                TODO Implementera IPv6
            */
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
            inet_aton(ip.c_str(), &serv_addr.sin_addr);
            serv_addr.sin_port = htons(port);
            return true;
        }
    public:

        client() = default;
        client(std::string ip, unsigned short port)
        {
            Init(ip, port) ? initialized = true : initialized = false;
        }
        client(int desc)
        {
            
        }
        bool Connect(std::string ip = "", unsigned short port = 8080)
        {
            if(!initialized)
            {
                initialized = Init(ip,port);
                if(!initialized) return false;
            }
            if (connect(sock , (struct sockaddr *)&serv_addr , sizeof(serv_addr)) < 0)
            {
                std::cerr << "Failed to connect\n";
                return false;
            }
        }
        template<typename ...Args>
        bool Send(unsigned key, Args... to_send)
        {
            std::stringstream ss;
            ss << key << " ";
            ((ss << to_send << " "), ...);
            (ss << "<>");
            auto data = ss.str();
            return Internal_send(key, data);
        }
        bool Send(unsigned short key, std::string derp)
        {
            derp += " <>";
            return Internal_send(key, derp);
        }
        template <typename... Ts>
        std::tuple<Ts...> Poll(short i)
        {
            std::tuple<Ts...> ret;
            if(GetData(i).empty()) return ret;
            std::string parse(GetData(i));
            std::istringstream ss(parse);
            for_each(ret, [&](auto& item) 
            {
                ss >> item;
            });
            //Hmmmm?
            ClearData(i);
            return ret;
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
        std::unordered_map<unsigned short, std::function<void()>> callbacks;
        std::vector<char*> to_send;
        std::thread *thr = nullptr;

        auto GetData(unsigned i) &
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
        void ServerWriteData(int index, char* data)
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
        void AddToSend(char* data)
        {
            std::lock_guard<std::mutex> lock_guard_name(tsend);
            to_send.push_back(data);
        }
        void ClearData(unsigned index)
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
        size_t GetWaitingData(unsigned index)
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
        bool Launch(unsigned short port, const char* ip = "0.0.0.0", unsigned short tickrate = 64)
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
        inline void ShutDown()
        {
            isRunning = false;
        }
        /*
            TODO:
                Vilken klient ska detta skickas till? 
                Alla?

        */
        template<typename... Args>
        void Send(unsigned key, Args... to_send)
        {
            std::stringstream ss;
            ss << key << " ";
            ((ss << to_send << " "), ...);
            (ss << "<>");
            auto data = ss.str();
            //std::cout << data << std::endl;
            AddToSend((char*)data.c_str());

            // Detta ska bort när klienten är klar
            rawdata[key] += data;
        }
        template<typename T>
        void Send(unsigned key, std::vector<T> to_send)
        {
            std::stringstream ss;
            ss << key << " ";
            ss << to_send.size() << " ";
            std::ostringstream oss;
            if(to_send.size() > 500)
            {
                std::copy(std::execution::par, to_send.begin(), to_send.end(), 
                    std::ostream_iterator<T>(oss, " "));
            }
            else
            {
                std::copy(std::execution::seq, to_send.begin(), to_send.end(), 
                    std::ostream_iterator<T>(oss, " "));
            }
            
            
            ss << oss.str();
            ss << "<>";
            auto data = ss.str();
            AddToSend((char*)data.c_str());

            // Detta ska bort när klienten är klar
            rawdata[key] += data;
        }
        template <typename T>
        std::vector<T> PollVec(short i)
        {
            std::vector<T> ret;
            auto data = GetData(i);
            std::istringstream ostrm(data);
            short key;
            size_t len;
            ostrm >> key;
            ostrm >> len;
            ret.resize(len);

            if(len > 500)
            {
                std::copy(std::execution::par, std::istream_iterator<T>(ostrm), std::istream_iterator<T>(),
                    ret.begin());
            }
            else
            {
                std::copy(std::execution::seq, std::istream_iterator<T>(ostrm), std::istream_iterator<T>(),
                    ret.begin());
            }
            {
                data = GetData(i);
                std::lock_guard<std::mutex> lock_guard_name(dmutex);
                data += " <>";
            }
            ClearData(i);

            return ret;
        }
        template <typename... Ts>
        std::tuple<Ts...> Poll(short i)
        {
            std::tuple<Ts...> ret;
            //auto size = std::tuple_size<decltype(ret)>::value;
            std::string parse(GetData(i));
            std::istringstream ss(parse);
            for_each(ret, [&](auto& item) 
            {
                ss >> item;
            });
            //Hmmmm?
            ClearData(i);
            return ret;
        }
        inline void RegisterHandle(std::function<void()> func, short key)
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
        auto AddServer()
        {
            servers.push_back(server());
            return &servers.back();

        }
    };
}
