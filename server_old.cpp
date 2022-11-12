// Standard C++ headers
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


// Server Port/Socket/Addr related headers
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "sqlite3.h"


#define SERVER_PORT  5432
#define MAX_PENDING  5
#define MAX_LINE     256

// Server Variables
struct sockaddr_in srv;
char buf[MAX_LINE];
socklen_t buf_len, addr_len;
int nRet;
int nClient[10] = { 0, };
int nSocket;
std::string infoArr[4];

fd_set fr;
fd_set fw;
fd_set fe;
int nMaxFd = 0;

//std::string command = "";
//std::string u = "";
//void* user = &u;
pthread_t thread_handles;
long thread;



// Functions
std::string buildCommand(char*);
std::string extractInfo(char*, std::string, void*);
void* serverCommands(void*);
static int callback(void*, int, char**, char**);

void HandleNewConnection()
{
    //nNewClient will be a new file descriptor
    //and now the client communication will take place 
    //using this file descriptor/socket only
    int nNewClient = accept(nSocket, (struct sockaddr*)&srv, &addr_len);
    //If you accept the value in second parameter, then it
    //will be 

    if (nNewClient < 0) {

        perror("Error during accepting connection");
        /*
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
        */
    }
    else {

        void* temp = &nNewClient;

        std::cout << nNewClient << std::endl;

        pthread_create(&thread_handles, NULL, serverCommands, temp);

        int nIndex;
        for (nIndex = 0; nIndex < 5; nIndex++)
        {
            if (nClient[nIndex] == 0)
            {
                nClient[nIndex] = nNewClient;
                if (nNewClient > nMaxFd)
                {
                    nMaxFd = nNewClient + 1;
                }
                break;
            }
        }

        if (nIndex == 5)
        {
            std::cout << std::endl << "Server busy. Cannot accept anymore connections";
        }

        std::cout << "Client connected on socket: " << nClient << std::endl << std::endl;
        send(nClient[nIndex], "You have successfully connected to the server!", 47, 0);
    }

}

void HandleDataFromClient()
{
    for (int nIndex = 0; nIndex < 5; nIndex++)
    {
        if (nClient[nIndex] > 0)
        {
            if (FD_ISSET(nClient[nIndex], &fr))
            {
                //Read the data from client
                char sBuff[255] = { 0, };
                int nRet = recv(nClient[nIndex], sBuff, 255, 0);
                if (nRet < 0)
                {
                    //This happens when client closes connection abruptly
                    std::cout << std::endl << "Error at client socket";
                    close(nClient[nIndex]);
                    nClient[nIndex] = 0;
                }
                else
                {
                    std::cout << std::endl << "Received data from:" << nClient[nIndex] << "[Message:" << sBuff << "]";
                    send(nClient[nIndex], "Recieved Message", 17, 0);
                    break;
                }
            }
        }
    }
}





/*int
main()
{
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int buf_len, addr_len;
    int s, new_s;
    std::string command = "";
    std::string user = "";
    pthread_t* thread_handles;
    long thread;
    /* build address data structure
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);
    /* setup passive open
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
        perror("simplex-talk: bind");
        exit(1);
    }
    listen(s, MAX_PENDING);
    /* wait for connection, then receive and print text *
    while (1) {
        if ((new_s = accept(s, (struct sockaddr*)&sin, &addr_len)) < 0) {
            perror("simplex-talk: accept");
            exit(1);
        }
        while (buf_len = recv(new_s, buf, sizeof(buf), 0))
            fputs(buf, stdout);
            command = buildCommand(buf);
        if (command == "LOGIN") {
            user = extractInfo(buf, command);
            pthread_create(&thread_handles[thread], NULL, serverCommands, user);
        }
        close(new_s);
    }
}*/

int main(int argc, char* argv[]) {



    // Database Variables
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    const char* sql;
    std::string resultant;
    std::string* ptr = &resultant;


    // Open Database and Connect to Database
    rc = sqlite3_open("cis427_crypto.sqlite", &db);


    // Check if Database was opened successfully
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }
    else {
        fprintf(stderr, "Opened database successfully\n");
    }


    // Create sql users table creation command
    sql = "create table if not exists users\
    (\
        ID INTEGER PRIMARY KEY AUTOINCREMENT,\
        email TEXT NOT NULL,\
        first_name TEXT,\
        last_name TEXT,\
        user_name TEXT NOT NULL,\
        password TEXT,\
        usd_balance DOUBLE NOT NULL\
    );";

    // Execute users table creation
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);


    // Create sql cryptos table creation command
    sql = "create table if not exists cryptos (\
        ID INTEGER PRIMARY KEY AUTOINCREMENT,\
        crypto_name varchar(10) NOT NULL,\
        crypto_balance DOUBLE,\
        user_id TEXT,\
        FOREIGN KEY(user_id) REFERENCES users(ID)\
    );";

    // Execute cryptos table creation
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);


    //Check if user 1 exists in the database. If no user found, create new user
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE  users.ID=1), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";

    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);


    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (resultant == "USER_NOT_PRESENT") {
        // Create a user if one doesn't already exist
        fprintf(stdout, "No user is present in the users table. Attempting to add a new user.\n");

        sql = "INSERT INTO users VALUES (1, 'cis427@gmail.com', 'John', 'Smith', 'J_Smith', 'password', 100);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new user was added successfully.\n");
        }
    }
    else if (resultant == "USER_PRESENT") {
        std::cout << "A user is already present in the users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        std::cout << "Error returned Resultant = " << resultant << std::endl;
    }






    // Setup passive open // Initialize the socket
    nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (nSocket < 0) {
        std::cout << "Socket not Opened\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Socket Opened: " << nSocket << std::endl;
    }


    // Build address data structure
    srv.sin_family = AF_INET;
    srv.sin_port = htons(SERVER_PORT);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(&(srv.sin_zero), 0, 8);


    // Set Socket Options
    int nOptVal = 0;
    int nOptLen = sizeof(nOptVal);
    nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOptVal, nOptLen);
    if (!nRet) {
        std::cout << "The setsockopt call successful\n";
    }
    else {
        std::cout << "Failed setsockopt call\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
    }


    //Bind the socket to the local port
    nRet = (bind(nSocket, (struct sockaddr*)&srv, sizeof(srv)));
    if (nRet < 0) {
        std::cout << "Failed to bind to local port\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Successfully bound to local port\n";
    }


    //Listen to the request from client
    nRet = listen(nSocket, MAX_PENDING);
    if (nRet < 0) {
        std::cout << "Failed to start listen to local port\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Started listening to local port\n";
    }


    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    nMaxFd = nSocket + 1;






    while (1)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        //Set the FD_SET.
        //This need to be done every time
        FD_ZERO(&fr);
        FD_SET(nSocket, &fr);
        for (int nIndex = 0; nIndex < 5; nIndex++)
        {
            if (nClient[nIndex] > 0)
            {
                FD_SET(nClient[nIndex], &fr);
            }
        }

        nRet = select(nMaxFd, &fr, NULL, NULL, &tv);
        //After above call, every bit is reset by select call
        //in fr
        if (nRet < 0)
        {
            std::cout << std::endl << "select api call failed. Will exit";
            return (EXIT_FAILURE);
        }
        else if (nRet == 0)
        {
            std::cout << std::endl << "No client at port waiting for an active connection/new message";
        }
        else
        {
            //There is some client waiting either to connect
            //or some new data came from existing client.
            if (FD_ISSET(nSocket, &fr))
            {
                //Handle New connection
                HandleNewConnection();
            }
            /*else
            {
                //Check what existing client got the new data
                HandleDataFromClient();
            }*/
        }
    }






    /*
    // Wait for connection, then receive and print text
    while (1) {
        if ((nClient = accept(nSocket, (struct sockaddr*)&srv, &addr_len)) < 0) {
            perror("Error during accepting connection");
            sqlite3_close(db);
            std::cout << "Closed DB" << std::endl;
            close(nSocket);
            std::cout << "Closed socket: " << nSocket << std::endl;
            exit(EXIT_FAILURE);
        }
        else {
            std::cout << "Client connected on socket: " << nClient << std::endl << std::endl;
            send(nClient, "You have successfully connected to the server!", 47, 0);
        }
        while ((buf_len = (recv(nClient, buf, sizeof(buf), 0)))) {
            //Print out recieved message
            std::cout << "SERVER> Recieved message: " << buf;
            //Parse message for initial command
            command = buildCommand(buf);
            if (command == "LOGIN") {
                u = extractInfo(buf, command, &user);
                std::cout << *(std::string*)user << std::endl;
                pthread_create(&thread_handles, NULL, serverCommands, user);
            }
            // Default response to invalid command
            else {
                std::cout << "SERVER> Command not recognized" << std::endl;
                send(nClient, "400 invalid command", 20, 0);
            }
        }
    }
    */


    for (int l = 0; l < 11; l++) {
        close(nClient[l]);
    }

    //close(nClient);



    sqlite3_close(db);
    std::cout << "Closed DB" << std::endl;
    close(nSocket);
    std::cout << "Closed socket: " << nSocket << std::endl;
    exit(EXIT_SUCCESS);
}

// Parses command from buffer sent from client 
std::string buildCommand(char line[]) {
    std::string command = "";
    size_t len = strlen(line);
    for (size_t i = 0; i < len; i++) {
        if (line[i] == '\n')
            continue;
        if (line[i] == ' ')
            break;
        command += line[i];
    }
    return command;
}

// Enters the command info into an array. This array contains the type of coin, amount of coin, price per unit of coin, and the user ID.
// Returns true if successful, otherwise returns false 
std::string extractInfo(char line[], std::string command, void* user) {
    int l = command.length();
    int spaceLocation = l + 1;
    int i = spaceLocation;
    std::string info = "";

    while (line[i] != '\n') {
        info += line[i];
        //(std::string*)user += line[i];
        i++;
    }
    //user = info;
    return info;

}

void* serverCommands(void* user) {
    std::cout << "pthread created" /* << static_cast<std::string*>(user)*/ << std::endl;
    int clientID = reinterpret_cast<int>(*(int*)user);
    std::cout << clientID << std::endl;
    int buf_len;
    std::string u;
    std::string command;
    //int k = &clientID;

    while (1) {

        //nRet = select(nMaxFd, &fr, NULL, NULL, &tv);

        char sBuff[255] = { 0, };
        int nRet = recv(clientID, sBuff, 255, 0);
        std::cout << sBuff << " In the while loop" << std::endl;
        if (nRet < 0)
        {
            //This happens when client closes connection abruptly
            std::cout << std::endl << "Error at client socket";
            close(clientID);
            clientID = 0;
        }
        else
        {
            while ((buf_len = (recv(clientID, sBuff, sizeof(sBuff), 0)))) {
                //Print out recieved message
                std::cout << "SERVER> Recieved message: " << sBuff;
                //Parse message for initial command
                command = buildCommand(sBuff);
                std::cout << command << std::endl;
                if (command == "LOGIN") {
                    u = extractInfo(sBuff, command, &user);
                    std::cout << u << " logged in!" << std::endl;
                    send(clientID, "You have logged in!", 20, 0);
                    //pthread_create(&thread_handles, NULL, serverCommands, user);
                }
                // Default response to invalid command
                else {
                    std::cout << "SERVER> Command not recognized" << std::endl;
                    send(clientID, "400 invalid command", 20, 0);
                }
            }
            //std::cout << std::endl << "Received data from:" << clientID << "[Message:" << sBuff << "]";
            //send(clientID, "Recieved Message", 17, 0);
            break;
        }

        

    }
}

static int callback(void* ptr, int count, char** data, char** azColName) {

    std::string* resultant = (std::string*)ptr;

    if (count == 1) {
        *resultant = data[0];
    }
    else if (count > 1) {
        for (int i = 0; i < count; i++) {

            if (*resultant == "") {
                *resultant = data[i];
            }
            else {
                *resultant = *resultant + " " + data[i];
            }

            // new line btwn every record
            if (i == 3)
            {
                *resultant += "\n  ";
            }

        }
    }
    return 0;
}