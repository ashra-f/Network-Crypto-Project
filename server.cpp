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


// SQLite3 related headers/definitions
#include "sqlite3.h"

#define SERVER_PORT  29270
#define MAX_PENDING  5
#define MAX_LINE     256


// Functions
std::string buildCommand(char*);
bool extractInfo(char*, std::string*, std::string);
static int callback(void*, int, char**, char**);


/////////////////
// Main Fuction//
/////////////////

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
    } else {
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


    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (resultant == "USER_NOT_PRESENT") {
        // Create a user if one doesn't already exist
        fprintf(stdout, "No user is present in the users table. Attempting to add a new user.\n");

        sql = "INSERT INTO users VALUES (1, 'cis427@gmail.com', 'John', 'Smith', 'J_Smith', 'password', 100);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

        if( rc != SQLITE_OK ) {
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



    // Server Variables
    struct sockaddr_in srv;
    char buf[MAX_LINE];
    socklen_t buf_len, addr_len;
    int nRet;
    int nClient;
    int nSocket;
    std::string infoArr[4];
    std::string command = "";



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

            /* 
                THE BUY COMMAND:
                    1. Check if the command was used properly
                        - Return an error if not used properly
                        - Otherwise, continue
                    2. Check if the client-selected user exists in the users table
                        - Return an error if the user does not exist
                        - Otherwise, continue
                    3. Calculate the crypto transaction price
                    4. Get the user's usd balance
                    5. Check if the user can afford the transaction
                        - If they cannot, return an error
                        - Otherwise, continue
                    6. Update the user's balance in the users table
                    7. Check the cryptos table if there already exists a record of the selected crypto
                        - If there exists a record, update the record
                        - Otherwise, create a new record
                    8. The command completed successfully, return 200 OK, the new usd_balance and new crypto_balance
            */

            if (command == "BUY") {

                // Checks if the client used the command properly
                if(!extractInfo(buf, infoArr, command)) {
                    send(nClient, "403 message format error: Missing information\n EX. Command: BUY crypto_name #_to_buy price userID", sizeof(buf), 0);
                }
                else {
                    // Check if selected user exists in users table 
                    std::string selectedUsr = infoArr[3];
                    std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=" + selectedUsr + "), 'PRESENT', 'NOT_PRESENT') result;";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                    //Check if SQL executed correctly
                    if( rc != SQLITE_OK ) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        send(nClient, "SQL error", 10, 0);
                    }
                    else if (resultant == "PRESENT") {
                        // USER EXISTS
                        fprintf(stdout, "User Exists in Users Table.\n");

                        // Calculate crypto price
                        double cryptoPrice = std::stod(infoArr[1]) * std::stod(infoArr[2]);
                        std::cout << "Crypto Price: " << cryptoPrice << std::endl;

                        // Get the usd balance of the user
                        sql = "SELECT usd_balance FROM users WHERE users.ID=" + selectedUsr;
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                        std::string usd_balance = resultant;
                        std::cout << "Current User Balance: " << usd_balance << std::endl;

                        //Check if SQL executed correctly
                         if( rc != SQLITE_OK ) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            send(nClient, "SQL error", 10, 0);
                        }
                        else if (stod(usd_balance) >= cryptoPrice) {
                            // User has enough in balance to make the purchase
                            // Update usd_balance with new balance
                            double difference = stod(usd_balance) - cryptoPrice;
                            std::string sql = "UPDATE users SET usd_balance=" + std::to_string(difference) + " WHERE ID =" + selectedUsr + ";";
                            rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                            std::cout << "User Balance Updated: " << difference << std::endl;

                            //Check if SQL executed correctly
                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "SQL error", 10, 0);
                            }

                            // Add new record or update record to crypto table
                            // Checks if record already exists in cryptos
                            sql = "SELECT IIF(EXISTS(SELECT 1 FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "SQL error", 10, 0);
                            }
                            else if (resultant == "RECORD_PRESENT"){
                                // A record exists, so update the record
                                sql = "UPDATE cryptos SET crypto_balance= crypto_balance +" + infoArr[1] + " WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                std::cout << "Added " << infoArr[1] << " crypto to " << infoArr[0] << " for " << selectedUsr << std::endl;

                                //Check if SQL executed correctly
                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "SQL error", 10, 0);
                                }
                            }
                            else {
                                // A record does not exist, so add a record
                                sql = "INSERT INTO cryptos(crypto_name, crypto_balance, user_id) VALUES ('" + infoArr[0] + "', '" + infoArr[1] + "', '" + selectedUsr + "');";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                std::cout << "New record created:\n\tCrypto Name: " << infoArr[0] << "\n\tCrypto Balance: " << infoArr[1] << "\n\tUserID: " << selectedUsr << std::endl;

                                //Check if SQL executed correctly
                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "SQL error", 10, 0);
                                }
                            }

                            // Get the new usd_balance
                            sql = "SELECT usd_balance FROM users WHERE users.ID=" + selectedUsr;
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                            usd_balance = resultant;

                            //Check if SQL executed correctly
                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "SQL error", 10, 0);
                            }

                            // Get the new crypto_balance
                            sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                            
                            //Check if SQL executed correctly
                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "SQL error", 10, 0);
                            }
                            std::string crypto_balance = resultant;

                            // The command completed successfully, return 200 OK, the new usd_balance and new crypto_balance
                            std::string tempStr = "200 OK\n   BOUGHT: New balance: " + crypto_balance + " " + infoArr[0] + ". USD balance $" + usd_balance;
                            send(nClient, tempStr.c_str(), sizeof(buf), 0);
                        }
                        else {
                            std::cout << "SERVER> Not enough balance. Purchase Aborted." << std::endl;
                            send(nClient, "403 message format error: not enough USD", sizeof(buf), 0);
                        }
                    }
                    else {
                        // USER DOES NOT EXIST
                        fprintf(stdout, "SERVER> User Does Not Exist in Users Table. Aborting Buy\n");
                        std::string tempStr = "403 message format error: user " + selectedUsr + " does not exist";
                        send(nClient, tempStr.c_str(), sizeof(buf), 0);
                    }
                }
                std::cout << "SERVER> Successfully executed BUY command\n\n";
            }
             /* 
                THE SELL COMMAND:
                    1. Check if the command was used properly
                        - Return an error if not used properly. Otherwise continue
                    2. Check if the client-selected user exists in the users table
                        - Return an error if the user does not exist. Otherwise, continue
                    3. Check if the user owns the selected coin
                        - Return an error if they do not own the coin. Otherwise, continue
                    4. Check if the user has enough of the selected coin to sell
                        - Return an error if they don't. Otherwise, continue
                    5. Update the users table
                        a. Increase the user's usd balance
                    6. Update the cryptos table
                        a. Decrease the user's crypto balance
                    7. If this stage is reached, the sell command has completed successfully
                        - return 200 ok, the new cryptos balance, and the new usd balance
            */

            else if (command == "SELL") {
                // Check if the client used the command properly
                if(!extractInfo(buf, infoArr, command)) {
                    std::cout << "Invalid command: Missing information" << std::endl;
                    send(nClient, "403 message format error: Missing information\n EX. Command: SELL crypto_name crypto_price crypto_amnt userID", sizeof(buf), 0);
                }
                else {
                    std::string selectedUsr = infoArr[3];

                    // Check if the selected user exists in users table 
                    std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=" + selectedUsr + "), 'PRESENT', 'NOT_PRESENT') result;";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                    if( rc != SQLITE_OK ) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        send(nClient, "SQL error", 10, 0);
                    }
                    else if (resultant == "PRESENT") {
                        // Check if the user owns the selected coin
                        sql = "SELECT IIF(EXISTS(SELECT 1 FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                        if( rc != SQLITE_OK ) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            send(nClient, "SQL error", 10, 0);
                        }
                        else if (resultant == "RECORD_NOT_PRESENT") {
                            std::cout << "SERVER> User doesn't own the selected coin. Aborting Sell\n";
                            send(nClient, "403 message format error: User does not own this coin.", sizeof(buf), 0);
                        }
                        else {
                            // Check if the user has enough of the selected coin to sell
                            double numCoinsToSell = std::stod(infoArr[1]);
                            // Get the number of coins the user owns of the selected coin
                            sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "SQL error", 10, 0);
                            }

                            double crypto_balance = std::stod(resultant);
                            // Not enough coins in balance to sell
                            if (crypto_balance < numCoinsToSell) {
                                std::cout << "SERVER> Attempting to sell more coins than the user has. Aborting sell.\n";
                                send(nClient, "403 message format error: Attempting to sell more coins than the user has.", sizeof(buf), 0);
                            }
                            else {
                                // Get dollar amount to sell
                                double cryptoPrice = std::stod(infoArr[1]) * std::stod(infoArr[2]);

                                /* Update users table */
                                // Add new amount to user's balance
                                sql = "UPDATE users SET usd_balance= usd_balance +" + std::to_string(cryptoPrice) + " WHERE users.ID='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);

                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "SQL error", 10, 0);
                                }

                                /* Update cryptos table */
                                // Remove the sold coins from cryptos
                                sql = "UPDATE cryptos SET crypto_balance= crypto_balance -" + std::to_string(numCoinsToSell) + " WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);

                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "SQL error", 10, 0);
                                }


                                // Get new usd_balance
                                sql = "SELECT usd_balance FROM users WHERE users.ID=" + selectedUsr;
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                std::string usd_balance = resultant;

                                // Get new crypto_balance
                                sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                std::string crypto_balance = resultant;

                                // Sell command completed successfully
                                std::string tempStr = "200 OK\n   SOLD: New balance: " + crypto_balance + " " +  infoArr[0] + ". USD $" + usd_balance;
                                send(nClient, tempStr.c_str(), sizeof(buf), 0);
                            }
                        }
                    }
                    else {
                        fprintf(stdout, "SERVER> User Does Not Exist  in Users Table. Aborting Sell.\n");
                        send(nClient, "403 message format error: user does not exist.", sizeof(buf), 0);
                    }
                }
                std::cout << "SERVER> Successfully executed SELL command\n\n";
            }
            else if (command == "LIST") {
                std::cout << "List command." << std::endl;
                resultant = "";
                // List all records in cryptos table for user_id = 1
                std::string sql = "SELECT * FROM cryptos WHERE cryptos.user_id=1";

                /* Execute SQL statement */
                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                if (rc != SQLITE_OK) {
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    send(nClient, "SQL error", 10, 0);
                }

                std::string sendStr;

                if (resultant == "") {
                    sendStr = "200 OK\n   No records in the Crypto Database.";
                }
                else{
                    sendStr = "200 OK\n   The list of records in the Crypto database:\nCryptoID  Crypto_Name Crypto_Amount  UserID\n   " + resultant;
                }
                send(nClient, sendStr.c_str(), sizeof(buf), 0);
            }
            else if (command == "BALANCE") {
                std::cout << "Balance command." << std::endl;
                // check if user exists
                std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=1), 'PRESENT', 'NOT_PRESENT') result;";

                /* Execute SQL statement */
                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                if( rc != SQLITE_OK ) {
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    send(nClient, "SQL error", 10, 0);
                }
                else if (resultant == "PRESENT") {
                    // outputs balance for user 1
                    sql = "SELECT usd_balance FROM users WHERE users.ID=1";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    std::string usd_balance = resultant;

                    // get full user name
                    sql = "SELECT first_name FROM users WHERE users.ID=1";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    std::string user_name = resultant;

                    sql = "SELECT last_name FROM users WHERE users.ID=1";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    user_name += " " + resultant;

                    std::string tempStr = "200 OK\n   Balance for user " + user_name + ": $" + usd_balance;
                    send(nClient, tempStr.c_str(), sizeof(buf), 0);
                }
                else {
                    std::cout << "SERVER> User does not exist. Aborting Balance.\n";
                    send(nClient, "User does not exist.", sizeof(buf), 0);
                }
            }

            // Turns off the server and terminates all connections
            else if (command == "SHUTDOWN") {
                send(nClient, "200 OK", 7, 0);
                std::cout << "Shutdown command." << std::endl;
                sqlite3_close(db);
                std::cout << "Closed DB" << std::endl;
                close(nClient);
                std::cout << "Closed Client Connection: " << nClient << std::endl;
                close(nSocket);
                std::cout << "Closed Server socket: " << nSocket << std::endl;
                exit(EXIT_SUCCESS);
            }

            // Closes client when client quits
            else if (command == "QUIT") {
            std::cout << "Quit command." << std::endl;
            send(nClient, "200 OK", 7, 0);
            close(nClient);

            break;
            }

            // Default response to invalid command
            else {
                std::cout << "SERVER> Command not recognized" << std::endl;
                send(nClient, "400 invalid command", 20, 0);
            }
        }
    }
    close(nClient);
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
bool extractInfo(char line[], std::string info[], std::string command) {
    int l = command.length();
    int spaceLocation = l + 1;

    for (int i = 0; i < 4; i++) {
        info[i] = "";

        // Parses the information
        for (int j = spaceLocation; j < strlen(line); j++) {
            
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
            std::fill_n(info, 4, 0);
            return false;
        }

        spaceLocation += info[i].length() + 1;

    }

    return true;

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
