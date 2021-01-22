/*********************************************************************
** Program name: ftserver.c
* 
** Description: ftserver.c is a server that can give a client a
* text file or directory listing. Listens on a port given through
* a command line argument. ftserver is meant to be used with ftclient,
* which will send ftserver a request, with data to be sent on ftclient's
* choice. ftserver then connects to ftclient and fulfills the request
* or sends an error message.
* 
* Usage: ./ftserver [port]
* 
* Sources: Some of this code is repurposed from CS344 taught by Ben Brewster.
* The code was used for the one-time pad project.
*********************************************************************/

#include "ftfunctions.h"

int main(int argc, char *argv[])
{
    int status; // Wait status
    int listenSocketFD = startup(argc, argv);
    
    // Run server until user issues SIGINT
	while (1)
	{
	    // Wait for connections
        int controlFD = createSocketFD(listenSocketFD);
        int pid = fork();
        // if fork failed
        if (pid < 0)
        {
            close(controlFD);
            fprintf(stderr, "fork() returned error\n");
            continue;
        }
        // child processes this code
        else if (pid == 0)
        {   
            // Client connected
            printf("Alert: New connection from ");
            // Get the request string from client, extract the data port
            // A request string contains the data port, command and command arguments
            char* buffer = getRequest(controlFD);
            char* dataPortStr = getDataPortStr(buffer);
            int dataPort = atoi(dataPortStr);
            
            // Return error message if an unknown command was issued
            if (strstr(buffer, "UNKNOWN") != NULL)
            {
                printf("Invalid command issued from client. Terminating connection.");
                char errorString[] = "ERROR: Invalid command. Try -l (list) or -g <FILENAME> (get)\n";
                sendMsg(errorString, controlFD);
            }
            // Valid command
            else
            {
                // Acknowledge valid command (client will be waiting for an okay or error message)
                char validCmd[] = "CONTINUE";
                sendMsg(validCmd, controlFD);
                // Extract client's host information to connect on the data port
                char ipstr[INET6_ADDRSTRLEN];
                getClientHostInfo(controlFD, ipstr);
                printf("host: %s. Requested data port: %i\n", ipstr, dataPort);
                // Connect to the client on the data port
                int dataFD = clientConnect(ipstr, dataPortStr);
                printf("Data connection established on port %i\n", dataPort);
                // Process the request and send data back
                processRequest(buffer, controlFD, dataFD);
                sleep(1);
                // Close connection on data socket
                close(dataFD);
            }
            // Close connection on control socket
            close(controlFD);
            // Request done; clean up allocated data
            printf("** Request fulfilled; end of connection **\n");
            free(buffer);
            free(dataPortStr);
        }
        else
        {
            // Parent has no need for the control socket's connection
            close(controlFD);
            // Wait for any finished children
            waitpid(-1, &status, WNOHANG);
        }
    }
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
