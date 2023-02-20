#include <stdlib.h>
#include <thread>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

//./bin/StompWCIClient.o 127.0.0.1 7777

int main(int argc, char *argv[])
{
    // TODO: implement the STOMP client
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    volatile bool isLoggedIn = false;
    string pendingCommand = "";
    while (!isLoggedIn)
    {
        string line;
        if (pendingCommand.length()==0){
            const short bufsize = 1024;
            char buf[bufsize];
            std::cin.getline(buf, bufsize);
            line = buf;
        }
        else{
            line = pendingCommand;
            pendingCommand = "";
        }
        int index = line.find(' ');
        string command = line.substr(0, index);
        string lineContent = line.substr(index + 1, line.length() - command.length() - 1);
        if (command != "login")
            std::cout << "Please log in before making requests" << std::endl;
        else
        {
            // connect to server
            int index1 = lineContent.find(':');
            string serverHost = lineContent.substr(0, index1);
            int index2 = lineContent.find(' ');
            char serverPortString[index2 - index1 - 1];
            lineContent.copy(serverPortString, index2 - index1 - 1, index1 + 1);
            short serverPort = atoi(serverPortString);
            ConnectionHandler connectionHandler(serverHost, serverPort);
            if (!connectionHandler.connect())
            { // connection failed
                std::cout << "Could not connect to server" << std::endl;
                connectionHandler.close();
            }
            else
            {
                // connection was successsful
                StompProtocol stompProtocol(connectionHandler);
                if (!stompProtocol.handleLogIn(lineContent))
                    connectionHandler.close();
                else if (stompProtocol.getIsLoggedIn())
                {
                    // login was successfully sent
                    isLoggedIn = true;
                    std::thread serverThread(&StompProtocol::handleServerResponse, &stompProtocol);
                    serverThread.detach();
                    while (stompProtocol.getIsLoggedIn())
                    {
                        const short bufsize = 1024;
                        char buf[bufsize];
                        std::cin.getline(buf, bufsize);
                        string line(buf);
                        stompProtocol.process(line, pendingCommand);
                        isLoggedIn = stompProtocol.getIsLoggedIn();
                    } // connection closed
                }
            }
        }
    }
}
