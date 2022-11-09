// Standard C++ headers
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


// Server Port/Socket/Addr related headers
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_PORT 29270
#define MAX_LINE 256

int main(int argc, char* argv[]) {
    struct hostent* hp;
    struct sockaddr_in srv;
    char* host;
    char buf[MAX_LINE];
    int nClientSocket;
    int len;
    int nRet;


    // Check if user input host
    if (argc == 2) {
        host = argv[1];
    }
    else {
        std::cout << "Error > Did not give host argument\nUsage: ./client <host>\n\n";
        exit(EXIT_FAILURE);
    }


    // Translate host name into peer's IP address
    hp = gethostbyname(host);
    if (!hp) {
        std::cout << "Host Unknown\n\n";
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Host Known\n\n";
    }


    // active open
    nClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nClientSocket < 0) {
        std::cout << "Socket not Opened\n";
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Socket Opened: " << nClientSocket << std::endl;
    }


    // Build address data structure
    bzero((char*)&srv, sizeof(srv));
    srv.sin_family = AF_INET;
    bcopy(hp->h_addr, (char*)&srv.sin_addr, hp->h_length);
    srv.sin_port = htons(SERVER_PORT);
    memset(&srv.sin_zero, 0, 8);


    nRet = connect(nClientSocket, (struct sockaddr*)&srv, sizeof(srv));
    if (nRet < 0)
    {
        std::cout << "Connection failed\n";
        close(nClientSocket);
        std::cout << "Closed socket: " << nClientSocket << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Connected to the server\n";
        char buf[255] = { 0 };
        
        // Receive is a blocking call
        recv(nClientSocket, buf, 255, 0);
        std::cout << buf << std::endl;

        while (std::cout << "CLIENT> ") {
            fgets(buf, 256, stdin);
            char lastCommand[256];
            strncpy(lastCommand, buf, sizeof(buf));

            send(nClientSocket, buf, 256, 0);
            recv(nClientSocket, buf, 256, 0);
            std::cout << "CLIENT> " << buf << std::endl << std::endl;
            
            if (strcmp(lastCommand, "QUIT\n") == 0) {
               close(nClientSocket);
               std::cout << "Closed socket: " << nClientSocket << std::endl;
               exit(EXIT_SUCCESS);
            }
        }
    }
}