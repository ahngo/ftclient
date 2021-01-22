/*********************************************************************
** Program name: ftfunctions.h
* 
** Description: ftfunctions.h is a header file used for ftserver.c
* Contains functions to be used for socket interaction from initialization,
* listening, accepting connections, sending, and receiving data.
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <dirent.h> 
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/********************************************************************* 
** sendMsg()
* Description: Given a buffer and a valid socket file descriptor,
* ensures all data in the buffer is sent through the socket. In essence,
* uses send() with additional checks to ensure everything in the buffer
* was sent.
* 
* Pre-conditions: A buffer and a socket file descriptor are defined and
* passed to sendMsg().
* Post-conditions: Text in buffer are sent through socketFD or if 
* the socket is invalid or network conditions are erroneous (i.e. server 
* closed), will print out an error message.
*********************************************************************/

void sendMsg(char* buffer, int socketFD)
{
    // current iteration of send()
    int charsSent = 0;
    // total count of chars that have gone through send()
    int totalCharsSent = 0;
    // max data to send per send call
    int maxSendBytes = 1000;
    // remaining data to send
    int remainingBytes = strlen(buffer);
    // bytesToSend = full amount of data
    int bytesToSend = remainingBytes;
    // update maxSendBytes to bytesToSend when it is below 1000
    if (bytesToSend < maxSendBytes)
    {
        maxSendBytes = bytesToSend;
    }
    
    // Save buffer pointer so that send can resume printing to the
    // right place if interrupted
    char* ptr = buffer;
    while (totalCharsSent < bytesToSend)
    {
        charsSent = send(socketFD, ptr, maxSendBytes, 0);
        // Error handling
        if (charsSent < 0)
        {
            error("ERROR writing to socket");
        }
        else if (charsSent == 0)
        {
            error("Unexpected EOF");
        }
        // Update total and ptr as needed
        else
        {
            totalCharsSent += charsSent;
            ptr += charsSent;
        }
    	// Update maxSendBytes as needed so that erroneous data is not sent
    	remainingBytes = bytesToSend - totalCharsSent;
    	if (remainingBytes < maxSendBytes && totalCharsSent < bytesToSend)
    	{
    	    maxSendBytes = remainingBytes;
    	}
    }
    // verified sending
    int checkSend = -5;  // Holds amount of bytes remaining in send buffer
	do
	{
		ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
	}
	while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
	if (checkSend < 0)
	{
	    error("ioctl error");
	}
}

/********************************************************************* 
** recMsg()
* Description: Given a buffer, a valid socket file descriptor, and
* a message size, receives a specific amount of data and places it in
* the buffer. In essence, uses recv() with some error checks and
* checks to ensure that the correct amount of data is received and placed
* in the buffer.
* 
* Pre-conditions: An adequately-sized buffer for incoming messages,
* valid socket file descriptor and number of bytes to receive are defined
* and passed to the function.
* Post-conditions: The buffer will have the specified amount of text
* received through the socket, or print out an error (typically caused
* by erroneous network conditions).
*********************************************************************/

void recMsg(char* buffer, int bytesToReceive, int socketFD)
{
    
    // bytes rec'd from current iteration of recv
    int charsRec = 0;
    // max data to receive at a time
    int maxRecBytes = 1000;
    // Note: from prior experience, about 1000 characters can be received
    // without error.
    if (bytesToReceive < maxRecBytes)
    {
        maxRecBytes = bytesToReceive;
    }
    
    // Place data received through socketFD into the buffer
    charsRec = recv(socketFD, buffer, maxRecBytes, 0);
    
    // Error checking
    if (charsRec < 0)
    {
        error("ERROR receiving from socket");
    }
    else if (charsRec == 0)
    {
        error("ERROR: nothing received");
    }

}

/********************************************************************* 
** clientConnect()
* Description: Sets up a connection given a hostname and port. 
* Will return either an error or a success message.
* 
* Pre-conditions: A server is running on the provided hostname and port
* parameters.
* Post-conditions: A valid socket file descriptor will be returned or
* an error message will be returned.
* 
* Source: most of this code was provided by Ben Brewster in CS344.
*********************************************************************/

int clientConnect(char* hostname, char* port)
{
    // Socket variables
    int socketFD, portNumber;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;

    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(port); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverHostInfo = gethostbyname(hostname); // Convert the machine name into a special form of address
    if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (socketFD < 0) error("CLIENT: ERROR opening socket");

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
        error("CLIENT: ERROR connecting");
    
    // Successful connection    
    return socketFD;
}

/********************************************************************* 
** createSocket()
* Description: Given a port number, sets up a TCP socket and binds it
* to the current host. Returns the socket file descriptor.
* 
* Pre-conditions: None.
* Post-conditions: A file descriptor bound to the local host and given
* port number is created, or an error is returned.
*********************************************************************/


int createSocket(int portNumber)
{
    // Server variables
	int listenSocketFD;
	struct sockaddr_in serverAddress;

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	//portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	
	return listenSocketFD;
}

/********************************************************************* 
** createSocketFD()
* Description: Given a socket file descriptor, accepts connections.
* 
* Pre-conditions: A socket file descriptor is already set to listen
* and given to the function.
* Post-conditions: Blocks until an connection is accepted on the listening
* socket.
*********************************************************************/

int createSocketFD(int socket)
{
    
    struct sockaddr_in clientAddress;
    socklen_t sizeOfClientInfo;
    // Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	int establishedConnectionFD = accept(socket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
	if (establishedConnectionFD < 0) error("ERROR on accept");
	return establishedConnectionFD;
}

/********************************************************************* 
** getRequest()
* Description: Given a connection file descriptor, accepts a string
* that the server can parse for information.
* 
* Pre-conditions: A connection is established.
* Post-conditions: A dynamically allocated string is returned with
* the contents of the client's request.
*********************************************************************/

char* getRequest(int establishedConnectionFD)
{
    int bufferSize = sizeof(char) * 101;
    char* buffer = malloc(bufferSize);
    memset(buffer, '\0', bufferSize);
    recv(establishedConnectionFD, buffer, 100, 0);
    return buffer;
}

/********************************************************************* 
** processRequest()
* Description: Given a string with the client's request, a control
* connection, and data connection, fulfills the request or gives the
* client an error message. Data is sent through the data connection.
* 
* Pre-conditions: A string with the client's request, control connection
* and data connection are established.
* Post-conditions: Fulfills the client's request or returns an error message.
*********************************************************************/

void processRequest(char* buffer, int controlFD, int dataFD)
{
    // Directory Listing
    if ((strstr(buffer, "LIST")) != NULL)
    {
        printf("Client requests directory listing.\n");
        char listing[1024];
        memset(listing, '\0', sizeof(listing));
        struct dirent *de; // directory entry
        // Open current directory
        DIR *dir = opendir(".");
        if (dir == NULL)
        {
            printf("ERROR: Couldn't open current directory.\n");
        }
        // Get names of all current files and add them to a listing
        while ((de = readdir(dir)) != NULL)
        {
            // Ignore "." and ".." 
            if (de->d_name[0] != '.')
            {
                strcat(listing, de->d_name);
                strcat(listing, "\n");
            }
        }
        // Send the directory listing
        printf("Sending list of directory contents.\n");
        sendMsg(listing, dataFD);
    }
    // File transfer
    else if ((strstr(buffer, "GET")) != NULL)
    {
        // Find the file name requested by the client in the request string
        // Two pointers, one at the start of the filename, one at the end
        char* filenamePtr = (strstr(buffer, "FILENAME:"));
        filenamePtr += strlen("FILENAME:");
        char* filenameEndPtr = (strstr(buffer, "FILENAMEEND"));
        
        char filename[255];
        memset(filename, '\0', sizeof(filename));
        int i = 0;
        // Copy the filename from the request
        while (filenamePtr != filenameEndPtr)
        {
            filename[i] = *filenamePtr;
            filenamePtr++;
            i++;
        }
        
        printf("Client requested file: %s\n", filename);
        // Open the file
        FILE* fp = fopen(filename, "r");
        // Client will be waiting for confirmation or error on control FD
        if (fp)
        {
            sendMsg("SENDING", controlFD);
            // go to the end of the file and count how many bytes from the beginning
            fseek(fp, 0, SEEK_END);
            int length = ftell(fp);
            // make a buffer big enough for the file
            char* strbuffer;
            strbuffer = malloc(length * sizeof(char));
            memset(strbuffer, '\0', length * sizeof(char));
            // go to the beginning of the file and read the file into the buffer
            fseek(fp, 0, SEEK_SET);
            fread(strbuffer, 1, length, fp);
            fclose(fp);
            printf("Sending contents of %s on data port..\n", filename);
            sendMsg(strbuffer, dataFD);
            free(strbuffer);
        }
        // File not found; send back error
        else 
        {
            char errorString[255];
            sprintf(errorString, "ERROR: %s not found.", filename);
            printf("%s\n", errorString);
            sendMsg(errorString, controlFD);
        }
    }
}

/********************************************************************* 
** getDataPortStr()
* Description: Given a string with the client's request, extract the data
* port requested for use by the client.
* 
* Pre-conditions: A string with the client's request was given.
* Post-conditions: Returns a string with the port requested.
*********************************************************************/

char* getDataPortStr(char* buffer)
{
    // Extract the port from within the request string
    char* portStart = strstr(buffer, "PORTSTART:");
    portStart += 10;
    char* portEnd = strstr(buffer, "PORTEND");
    char* portStr = malloc(sizeof(char) * 10);
    memset(portStr, '\0', sizeof(char) * 10);
    
    // Using the two pointers, iterate until we have the entire port number
    int i = 0;
    while (portStart != portEnd)
    {
        portStr[i] = *portStart;
        i++;
        portStart++;
    }
    
    return portStr;
}

/********************************************************************* 
** getClientHostInfo()
* Description: Uses the control connection in order to extract the client's
* host for use with the data connection. Uses getpeername().
* 
* Pre-conditions: A connection/socket with the client is active.
* Post-conditions: Copies the client's hostname into a string created
* in main().
* 
* Code source: https://beej.us/guide/bgnet/html/multi/getpeernameman.html
*********************************************************************/


void getClientHostInfo(int activeSocket, char hostname[INET6_ADDRSTRLEN])
{
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    
    len = sizeof addr;
    getpeername(activeSocket, (struct sockaddr*)&addr, &len);
    
    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) 
    {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } 
    else 
    { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }
    strcpy(hostname, ipstr);
}


/********************************************************************* 
** startup()
* Description: Uses the control connection in order to extract the client's
* host for use with the data connection. Uses getpeername().
* 
* Pre-conditions: A connection/socket with the client is active.
* Post-conditions: Copies the client's hostname into a string created
* in main().
* 
* Code source: https://beej.us/guide/bgnet/html/multi/getpeernameman.html
*********************************************************************/

int startup(int argc, char *argv[])
{
    // Syntax check
    if (argc != 2) 
    { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); 
    }
    
    int listenSocketFD = createSocket(atoi(argv[1]));
    listen(listenSocketFD, 5);
    printf("Server is ready for incoming connections on port %s.\n", argv[1]);
    return listenSocketFD;
}
