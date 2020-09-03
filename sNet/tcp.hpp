#include <iostream>
#include <vector>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "packet.hpp"
namespace sNet
{
    namespace tcp
    {
        class TcpClient
        {
        private:
            int sock = -1, port = 0;
            struct sockaddr_in serv_addr;
            struct hostent *server;
        public:
            TcpClient()
            {
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if(sock < 0 ) 
                {
                    std::cerr << "[FATAL] Failed to open socket" << std::endl;
                    std::exit(-1);
                }
            }
            bool Connect(std::string _ip, unsigned short _port)
            {
                port = _port;
                server = gethostbyname(_ip.c_str());
                if(server == nullptr)
                {
                    std::cerr << "[Socket] Failed to connect to server" << std::endl;
                    return false;
                }
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
                serv_addr.sin_port = htons(port);
                
                if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
                {   
                    std::cerr << "[Socket| Failed to connect" << std::endl;
                    return false;
                }
            }
            bool Send(packet p)
            {

            }
            std::vector<packet> Recv()
            {

            }
            void Shutdown()
            {
                close(sock);
            }
        };
        class TcpServer
        {

        };
    }
}