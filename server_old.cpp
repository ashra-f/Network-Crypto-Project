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


#define SERVER_PORT  54321
#define MAX_PENDING  5
#define MAX_LINE     256

// Server Variables
struct sockaddr_in srv;
char buf[MAX_LINE];
socklen_t buf_len, addr_len;
int nRet;
int nClient[10] = { 0, };
int nSocket;
std::string infoArr[3];

sqlite3* db;
char* zErrMsg = 0;
const char* sql;
int rc;
std::string resultant;
std::string* ptr = &resultant;


typedef struct 
{ 
    int socket;
    int id;
    std::string user;
    std::string password; 
}userInfo;

void* temp = malloc(sizeof(userInfo));
userInfo u;

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
std::string extractInfo(char*, std::string);
bool extractInfo(char*, std::string*, std::string);
void* serverCommands(void*);
static int callback(void*, int, char**, char**);

std::string getPassword(char line[], int n) {
    
    int spaceLocation = n + 2;
    int i = spaceLocation;
    std::string info = "";

    while (line[i] != '\n') {
        if (line[i] == NULL)
            return "";
        if (line[i] == ' ')
            return info;
        info += line[i];
        //(std::string*)user += line[i];
        i++;
    }
    //user = info;
    return info;
}

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

    std::string command;
    
    
    temp = &u;

    for (int nIndex = 0; nIndex < 5; nIndex++)
    {
        if (nClient[nIndex] > 0)
        {
            if (FD_ISSET(nClient[nIndex], &fr))
            {
                //Read the data from client
                char sBuff[256] = { 0, };
                int nRet = recv(nClient[nIndex], sBuff, 256, 0);
                if (nRet < 0)
                {
                    //This happens when client closes connection abruptly
                    std::cout << std::endl << "Error at client socket";
                    close(nClient[nIndex]);
                    nClient[nIndex] = 0;
                }
                else
                {

                    command = buildCommand(sBuff);
                    std::cout << command << std::endl;

                    //std::cout << std::endl << "Received data from:" << nClient[nIndex] << "[Message:" << sBuff << "]";
                    send(nClient[nIndex], "Recieved Message", 17, 0);

                    if (command == "LOGIN") {
                        std::string info = extractInfo(sBuff, command);
                        u.user = info;
                        int passLength = command.length() + info.length();
                        std::string passInfo = getPassword(sBuff, passLength);
                        
                        u.password = passInfo;
                        u.socket = nIndex;

                        std::cout << "Assigned user info. Username: " << info << " Socket Index: " << u.socket << std::endl;
                        
                        std::string commandSql = "SELECT IIF(EXISTS(SELECT * FROM users WHERE user_name = '" + info + "' AND password = '" + passInfo + "') , 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
                        sql = commandSql.c_str();
                        sqlite3_exec(db, sql, callback, 0, &zErrMsg);
                        
                        

                        if (resultant == "USER_PRESENT"){
                            std::cout << "Logging in... " << std::endl;

                            commandSql = "SELECT ID FROM users WHERE user_name = '" + info + "' AND password = '" + passInfo + "'";
                            sql = commandSql.c_str();
                            sqlite3_exec(db, sql, callback, 0, &zErrMsg);
                            u.id = stoi(resultant);
                            pthread_create(&thread_handles, NULL, serverCommands, temp);
                            std::cout << "after pthread creation" << std::endl;
                        }
                        else{
                            std::cout << "Username or Password Invalid!" << std::endl;
                        }
                    }
                    else if (command == "QUIT") {
                        std::cout << "Quit command!" << std::endl;
                        send(nClient[nIndex], "You sent the QUIT command!", 27, 0);
                        close(nClient[nIndex]);
                        nClient[nIndex] = 0;
                    }
                    else {
                        std::cout << std::endl << "Received data from:" << nClient[nIndex] << "[Message: " << sBuff << " size of array: "<< strlen(sBuff) << "] Error 400" << std::endl;
                    }
                    break;
                }
            }
        }
    }
    std::cout << "out of data for loop" << std::endl;
}



int main(int argc, char* argv[]) {



    // Database Variables
    

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


    // Checks if the root exists in the database. If no user is found, create it
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE  users.user_name='root'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (resultant == "USER_NOT_PRESENT") {
        // Create the root user:
        fprintf(stdout, "Root user is not present. Attempting to add the user.\n");

        // Adds the root user
        sql = "INSERT INTO users VALUES (1, 'root23@gmail.com', 'Root', 'User', 'root', 'root01', 100);";
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
        std::cout << "The user already exists already present in the users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        std::cout << "Error returned Resultant = " << resultant << std::endl;
    }

    // Checks if Mary exists in the database. If no user is found, create it
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE  users.user_name='mary'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (resultant == "USER_NOT_PRESENT") {
        fprintf(stdout, "Mary is not present. Attempting to add the user.\n");

        // Adds mary
        sql = "INSERT INTO users VALUES (2, 'mary231@gmail.com', 'Mary', 'Poppins', 'mary', 'mary01', 100);";
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
        std::cout << "The user already exists already present in the users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        std::cout << "Error returned Resultant = " << resultant << std::endl;
    }

    // Check if John exists in the database. If no user is found, create it
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE  users.user_name='john'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (resultant == "USER_NOT_PRESENT") {
        fprintf(stdout, "John is not present. Attempting to add the user.\n");

        // Adds john
        sql = "INSERT INTO users VALUES (3, 'john_ce23@gmail.com', 'John', 'Cena', 'john', 'john01', 100);";
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
        std::cout << "The user already exists already present in the users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        std::cout << "Error returned Resultant = " << resultant << std::endl;
    }

    // Check if Moe exists in the database. If no user is found, create it
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE  users.user_name='moe'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (resultant == "USER_NOT_PRESENT") {
        fprintf(stdout, "John is not present. Attempting to add the user.\n");

        // Adds moe
        sql = "INSERT INTO users VALUES (4, 'moe23@gmail.com', 'Moe', 'Szyslak', 'moe', 'moe01', 100);";
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
        std::cout << "The user already exists already present in the users table.\n";
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

    // Set Socket Options
    int nOptVal = 1;
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

    // Build address data structure
    srv.sin_family = AF_INET;
    srv.sin_port = htons(SERVER_PORT);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(&(srv.sin_zero), 0, 8);


    


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
    //tv.tv_sec = 1;
    //tv.tv_usec = 0;

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
            else
            {
                //Check what existing client got the new data
                std::cout << "in else for handle data" << std::endl;
                HandleDataFromClient();
            }
        }
    }



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
std::string extractInfo(char line[], std::string command) {
    int l = command.length();
    int spaceLocation = l + 1;
    int i = spaceLocation;
    std::string info = "";

    while (line[i] != '\n') {
        if (line[i] == NULL)
            return "";
        if (line[i] == ' ')
            return info;
        info += line[i];
        i++;
    }
    return info;
}

void* serverCommands(void* userData) {
    std::cout << "pthread created" << std::endl;
    std::cout << "Username: " << ((userInfo*)userData)->user << std::endl;
    std::cout << "no" << std::endl;
    int clientIndex = ((userInfo*)userData)->socket;
    int clientID = nClient[((userInfo*)userData)->socket];
    std::cout << "assigned user" << std::endl;
    nClient[clientIndex] = -1;
    std::cout << clientID << std::endl;
    int buf_len;
    std::string u = ((userInfo*)userData)->user;
    int idINT = ((userInfo*)userData)->id;
    std::string id = std::to_string(idINT);
    std::string command;
    bool rootUsr;
    if (idINT == 1){
        rootUsr = true;
    }
    else{
        rootUsr = false;
    }

    std::cout << "Client Socket: " << clientID << std::endl;
    std::cout << "User ID: " << idINT;
    //int k = &clientID;

    while (1) {

        //nRet = select(nMaxFd, &fr, NULL, NULL, &tv);

        char Buff[256] = { 0, };
        //int nRet = recv(clientID, Buff, 255, 0);
        std::cout << Buff << " In the while loop" << std::endl;
        /*if (nRet < 0)
        {
            //This happens when client closes connection abruptly
            std::cout << std::endl << "Error at client socket";
            close(clientID);
            clientID = 0;
        }
        else
        {*/
            while ((buf_len = (recv(clientID, Buff, sizeof(Buff), 0)))) {
                //Print out recieved message
                std::cout << "SERVER> Recieved message: " << Buff << std::endl;
                //Parse message for initial command
                command = buildCommand(Buff);
                std::cout << command << std::endl;
                /*if (command == "LOGIN") {
                    u = extractInfo(sBuff, command, &user);
                    std::cout << u << " logged in!" << std::endl;
                    send(clientID, "You have logged in!", 20, 0);
                    
                    // this will need to be set to true if the usernmae and password match
                    login = true;

                }*/

                if (command == "BUY") {
                    std::cout << "Buy command!" << std::endl;
                    //send(clientID, "You sent the BUY command!", 26, 0);


                    // Checks if the client used the command properly
                    if (!extractInfo(Buff, infoArr, command)) {
                        send(clientID, "403 message format error: Missing information\n EX. Command: BUY crypto_name #_to_buy price userID", sizeof(Buff), 0);
                        std::cout << "extraction Error" << std::endl;
                    }
                    else {
                        // Check if selected user exists in users table 
                        std::string selectedUsr = infoArr[3];
                        std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=" + (std::string)id + "), 'PRESENT', 'NOT_PRESENT') result;";
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                        std::cout << "RC is equal to: " << rc << std::endl;

                        //Check if SQL executed correctly
                        if (rc != SQLITE_OK) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            //send(clientID, "SQL error", 10, 0);
                        }
                        else if (resultant == "PRESENT") {
                            // USER EXISTS
                            fprintf(stdout, "User Exists in Users Table.\n");

                            // Calculate crypto price
                            double cryptoPrice = std::stod(infoArr[1]) * std::stod(infoArr[2]);
                            std::cout << "Crypto Price: " << cryptoPrice << std::endl;

                            // Get the usd balance of the user
                            sql = "SELECT usd_balance FROM users WHERE users.ID=" + (std::string)id;
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                            std::string usd_balance = resultant;
                            std::cout << "Current User Balance: " << usd_balance << std::endl;

                            //Check if SQL executed correctly
                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                //send(clientID, "SQL error", 10, 0);
                            }
                            else if (stod(usd_balance) >= cryptoPrice) {
                                // User has enough in balance to make the purchase
                                // Update usd_balance with new balance
                                double difference = stod(usd_balance) - cryptoPrice;
                                std::string sql = "UPDATE users SET usd_balance=" + std::to_string(difference) + " WHERE ID =" + id + ";";
                                rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                                std::cout << "User Balance Updated: " << difference << std::endl;

                                //Check if SQL executed correctly
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    //send(clientID, "SQL error", 10, 0);
                                }

                                // Add new record or update record to crypto table
                                // Checks if record already exists in cryptos
                                sql = "SELECT IIF(EXISTS(SELECT 1 FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    //send(clientID, "SQL error", 10, 0);
                                }
                                else if (resultant == "RECORD_PRESENT") {
                                    // A record exists, so update the record
                                    sql = "UPDATE cryptos SET crypto_balance= crypto_balance +" + infoArr[1] + " WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "';";
                                    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                    std::cout << "Added " << infoArr[1] << " crypto to " << infoArr[0] << " for " << id << std::endl;

                                    //Check if SQL executed correctly
                                    if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                        //send(clientID, "SQL error", 10, 0);
                                    }
                                }
                                else {
                                    // A record does not exist, so add a record
                                    sql = "INSERT INTO cryptos(crypto_name, crypto_balance, user_id) VALUES ('" + infoArr[0] + "', '" + infoArr[1] + "', '" + id + "');";
                                    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                    std::cout << "New record created:\n\tCrypto Name: " << infoArr[0] << "\n\tCrypto Balance: " << infoArr[1] << "\n\tUserID: " << id << std::endl;

                                    //Check if SQL executed correctly
                                    if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                        //send(clientID, "SQL error", 10, 0);
                                    }
                                }

                                // Get the new usd_balance
                                sql = "SELECT usd_balance FROM users WHERE users.ID=" + id;
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                usd_balance = resultant;

                                //Check if SQL executed correctly
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    //send(clientID, "SQL error", 10, 0);
                                }

                                // Get the new crypto_balance
                                sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                                //Check if SQL executed correctly
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    //send(clientID, "SQL error", 10, 0);
                                }
                                std::string crypto_balance = resultant;

                                // The command completed successfully, return 200 OK, the new usd_balance and new crypto_balance
                                std::string tempStr = "200 OK\n   BOUGHT: New balance: " + crypto_balance + " " + infoArr[0] + ". USD balance $" + usd_balance;
                                send(clientID, tempStr.c_str(), sizeof(buf), 0);
                            }
                            else {
                                std::cout << "SERVER> Not enough balance. Purchase Aborted." << std::endl;
                                send(clientID, "403 message format error: not enough USD", sizeof(Buff), 0);
                            }
                        }
                        else {
                            // USER DOES NOT EXIST
                            fprintf(stdout, "SERVER> User Does Not Exist in Users Table. Aborting Buy\n");
                            std::string tempStr = "403 message format error: user " + id + " does not exist";
                            send(clientID, tempStr.c_str(), sizeof(Buff), 0);
                        }
                    }
                    
                    std::cout << "SERVER> Successfully executed BUY command\n\n";
                }
                else if (command == "SELL") {
                    std::cout << "Sell command!" << std::endl;
                    // Check if the client used the command properly
                    if (!extractInfo(Buff, infoArr, command)) {
                        std::cout << "Invalid command: Missing information" << std::endl;
                        send(clientID, "403 message format error: Missing information\n EX. Command: SELL crypto_name crypto_price crypto_amnt userID", sizeof(Buff), 0);
                    }
                    else {
                        std::string selectedUsr = infoArr[3];

                        // Check if the selected user exists in users table 
                        std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=" + id + "), 'PRESENT', 'NOT_PRESENT') result;";
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                        if (rc != SQLITE_OK) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            //send(clientID, "SQL error", 10, 0);
                        }
                        else if (resultant == "PRESENT") {
                            // Check if the user owns the selected coin
                            sql = "SELECT IIF(EXISTS(SELECT 1 FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                //send(clientID, "SQL error", 10, 0);
                            }
                            else if (resultant == "RECORD_NOT_PRESENT") {
                                std::cout << "SERVER> User doesn't own the selected coin. Aborting Sell\n";
                                send(clientID, "403 message format error: User does not own this coin.", sizeof(Buff), 0);
                            }
                            else {
                                // Check if the user has enough of the selected coin to sell
                                double numCoinsToSell = std::stod(infoArr[1]);
                                // Get the number of coins the user owns of the selected coin
                                sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    //send(clientID, "SQL error", 10, 0);
                                }

                                double crypto_balance = std::stod(resultant);
                                // Not enough coins in balance to sell
                                if (crypto_balance < numCoinsToSell) {
                                    std::cout << "SERVER> Attempting to sell more coins than the user has. Aborting sell.\n";
                                    send(clientID, "403 message format error: Attempting to sell more coins than the user has.", sizeof(Buff), 0);
                                }
                                else {
                                    // Get dollar amount to sell
                                    double cryptoPrice = std::stod(infoArr[1]) * std::stod(infoArr[2]);

                                    /* Update users table */
                                    // Add new amount to user's balance
                                    sql = "UPDATE users SET usd_balance= usd_balance +" + std::to_string(cryptoPrice) + " WHERE users.ID='" + id + "';";
                                    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);

                                    if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                        //send(clientID, "SQL error", 10, 0);
                                    }

                                    /* Update cryptos table */
                                    // Remove the sold coins from cryptos
                                    sql = "UPDATE cryptos SET crypto_balance= crypto_balance -" + std::to_string(numCoinsToSell) + " WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "';";
                                    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);

                                    if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                        //send(clientID, "SQL error", 10, 0);
                                    }


                                    // Get new usd_balance
                                    sql = "SELECT usd_balance FROM users WHERE users.ID=" + id;
                                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                    std::string usd_balance = resultant;

                                    // Get new crypto_balance
                                    sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + id + "';";
                                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                    std::string crypto_balance = resultant;

                                    // Sell command completed successfully
                                    std::string tempStr = "200 OK\n   SOLD: New balance: " + crypto_balance + " " + infoArr[0] + ". USD $" + usd_balance;
                                    send(clientID, tempStr.c_str(), sizeof(Buff), 0);
                                }
                            }
                        }
                        else {
                            fprintf(stdout, "SERVER> User Does Not Exist  in Users Table. Aborting Sell.\n");
                            send(clientID, "403 message format error: user does not exist.", sizeof(Buff), 0);
                        }
                    }
                    std::cout << "SERVER> Successfully executed SELL command\n\n";
                    //send(clientID, "You sent the SELL command!", 27, 0);
                }
                else if (command == "LIST") {
                    //std::cout << "List command!" << std::endl;
                    if (idINT == 1) {
                        std::cout << "List command." << std::endl;
                        resultant = "";
                        // List all records in cryptos table for user_id = 1
                        std::string sql = "SELECT * FROM cryptos";

                        /* Execute SQL statement */
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                        if (rc != SQLITE_OK) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            //send(nClient, "SQL error", 10, 0);
                        }

                        std::string sendStr;

                        if (resultant == "") {
                            sendStr = "200 OK\n   No records in the Crypto Database.";
                        }
                        else {
                            sendStr = "200 OK\n   The list of records in the Crypto database:\nCryptoID  Crypto_Name Crypto_Amount  UserID\n   " + resultant;
                        }
                        send(clientID, sendStr.c_str(), sizeof(Buff), 0);
                    }
                    else {
                        std::cout << "List command." << std::endl;
                        resultant = "";
                        // List all records in cryptos table for user_id = 1
                        std::string sql = "SELECT * FROM cryptos WHERE cryptos.user_id=" + id;

                        /* Execute SQL statement */
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                        if (rc != SQLITE_OK) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            //send(nClient, "SQL error", 10, 0);
                        }

                        std::string sendStr;

                        if (resultant == "") {
                            sendStr = "200 OK\n   No records in the Crypto Database.";
                        }
                        else {
                            sendStr = "200 OK\n   The list of records in the Crypto database:\nCryptoID  Crypto_Name Crypto_Amount  UserID\n   " + resultant;
                        }
                        send(clientID, sendStr.c_str(), sizeof(Buff), 0);
                    }
                    //send(clientID, "You sent the LIST command!", 27, 0);
                }
                else if (command == "BALANCE") {
                    std::cout << "Balance command!" << std::endl;
                    send(clientID, "You sent the BALANCE command!", 30, 0);
                }
                else if (command == "QUIT") {
                    std::cout << "Quit command!" << std::endl;
                    send(clientID, "You sent the QUIT command!", 27, 0);
                    nClient[clientIndex] = 0;
                    close(clientID);
                    pthread_exit(userData);
                    return userData;
                    //std::cout << "The pthread has be slain" << std::endl;
                    //break;
                }
                else if (command == "SHUTDOWN" && rootUsr) {
                    std::cout << "Shutdown command!" << std::endl;
                    send(clientID, "You sent the SHUTDOWN command!", 31, 0);
                }
                else if (command == "LOGOUT") {
                    std::cout << "Logout command!" << std::endl;
                    send(clientID, "You sent the LOGOUT command!", 29, 0);
                    nClient[clientIndex] = clientID;
                    pthread_exit(userData);
                    return userData;
                }
                else if (command == "DEPOSIT") {
                    std::cout << "Deposit command!" << std::endl;
                    send(clientID, "You sent the DEPOSIT command!", 30, 0);
                }
                else if (command == "WHO" && rootUsr) {
                    std::cout << "Who command!" << std::endl;
                    send(clientID, "You sent the WHO command!", 26, 0);
                }
                else if (command == "LOOKUP") {
                    std::cout << "Lookup command!" << std::endl;
                    send(clientID, "You sent the LOOKUP command!", 29, 0);
                }

                // Default response to invalid command
                else {
                    std::cout << "SERVER> Command not recognized" << std::endl;
                    send(clientID, "400 invalid command", 20, 0);
                }
            }
            //std::cout << std::endl << "Received data from:" << clientID << "[Message:" << sBuff << "]";
            //send(clientID, "Recieved Message", 17, 0);
            std::cout << std::endl << "Error at client socket";
            nClient[clientIndex] = 0;
            close(clientID);
            pthread_exit(userData);
            return userData;
            //break;
        //}

        

    }
}

bool extractInfo(char line[], std::string info[], std::string command) {
    int l = command.length();
    int spaceLocation = l + 1;

    for (int i = 0; i < 3; i++) {
        info[i] = "";

        // Parses the information
        for (int j = spaceLocation; j < strlen(line); j++) {

            if (line[j] == NULL)
                return false;
            if (line[j] == ' ')
                break;
            if (line[j] == '\n')
                break;
            info[i] += line[j];

            // Makes sure that only numbers are entered into the array for index 1, 2 and 3
            if (i > 0) {
                if (((int)line[j] > 57 || (int)line[j] < 46) && (int)line[j] != 47)
                    return false;
            }
        }
        if (info[i] == "") {
            std::fill_n(info, 3, 0);
            return false;
        }

        spaceLocation += info[i].length() + 1;

    }

    return true;

}

static int callback(void* ptr, int count, char** data, char** azColName) {

    if (count == 1) {
        resultant = data[0];
    }
    else if (count > 1) {
        for (int i = 0; i < count; i++) {
            
            if (resultant == "") {
                resultant = data[i];
            }
            else {
                resultant = resultant + " " + data[i];
            }

            // new line btwn every record
            if (i == 3)
            {
                resultant += "\n  ";
            }

        }
    }
    return 0;
}
