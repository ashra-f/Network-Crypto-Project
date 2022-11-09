# CIS427-Crypto-Project

## How to compile

So, you want to try running this here code? Follow me, and I'll tell you how.

To start with, your going to want to download these files:
```
server.cpp
client.cpp
sqlite3.o
sqlite3.h
```

Once downloaded, use Bitvise (or similar SSH Client) to transfer the files to you umich server. Mak sure to transfer all four.

Once Transfered, your going to need to compile the .cpp files.
To do so is relatively simple.

client.cpp
> g++ client.cpp -o client

server.cpp
> g++ server.cpp -std=c++11 -ldl -pthread sqlite3.o -o server

After compiling both of the files, you can now run them using
```
./server
./client
```

### Hopefully this was enough help to figure it out. If not,
### contact me on discord and I might be there (or not, idk).
### Hopefully the discord webhook will also give updates when git repo is updated. Still messing around with it...
