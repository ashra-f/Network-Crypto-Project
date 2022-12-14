# CIS 427 Crypto Project

This programming assignment is designed to introduce you to the socket interface and client-
server applications. Minimum knowledge in database is needed including insert, update and 
delete is needed. If no previous background in databases, please talk to the instructor. This 
assignment weights 10 % of your final grade.  

For this assignment, you will design and implement an online crypto currency trading application 
using network sockets.  You will write both the client and server portions of this application.  
The client and server processes will communicate using TCP sockets and will implement the 
protocol discussed below.


## How to compile:
To start with, you are going to want to download/unzip these files and set them aside:
```
- server.cpp
- client.cpp
- Makefile
```

Then, download the SQLite dependencies:
```
- sqlite3.c
- sqlite3.h
```

Once downloaded, use Bitvise (or similar SSH Client) to connect to your umich server. Once connected, transfer all the files to a directory in your umich server using sftp. Make sure to transfer all five.


Once Transferred, you're going to need to compile the files.
First, navigate to the directory containing the source files. Once in the directory, run the command below:
```
make -f Makefile
```

This will compile an object file for sqlite3, and compile the server and client run files.


How to execute:
To run the server, enter the following code in the terminal while in the directory the file is in:
```
./server
```

To run the client, enter one of the following commands in a separate terminal:
```
./client localhost or ./client 127.0.0.1
```

You should now have both a server and client running and connected to each other.

## Implemented Commands:
### 1. Buy Command: purchases crypto for user
#### How to use the command:
``` 
BUY cryptoName cryptoAmount cryptoPrice userID
```

### 2. Sell Command: sells crypto for user
#### How to use the command:
```
SELL cryptoName cryptoAmount cryptoPrice userID
```

### 3. Balance Command: display balance of user
#### How to use the command:
```
BALANCE
```

### 4. List: lists a user's owned crypto coins
#### How to use the command:
```
LIST
```

### 5. Quit: disconnects from the server by closing the current client-server connection sockets
#### How to use the command:
```
QUIT
```

### 6. Shutdown: tells the server to close all open connections and exit
#### How to use the command:
```
SHUTDOWN
```

## Known Bugs:
Occasionally, the port is still in use after shutting down and rerunning the server. This may be due to external applications or processes using the port, or not waiting long enough after closing the server. While this is not classified as a bug, it may cause issues during execution, so it is noted here.

## Member Responsibilities:
Code was worked on collaboratively through multiple group meetings. Each member focused on a different part of the project, and everyone took part in bug testing and fixing:
```
Ashraf Hammoud:
- Buy, sell, list, and balance command implementation
- SQL query commands
```
```
Spencer Schneider:
- Command parsing and data extraction
- Makefile
```
```
Brandon Shady:
- Server/Client creation and configuration
- Database & SQLite3 integration for C++
```

