Server and client data transfer program. The server (ftserver.c) is started by giving it a port in the command line. The client is started by giving it the hostname of the server, the server's listening port, a command and a data port. The client will listen for connections on the data port. The server connects to the client on the data port and transfers data. Available commands are requesting a directory listing or getting a file.
Features cross-language connection usage and basic socket/network programming.

Directions:
1. ftserver.c will need to be compiled into an executable
2. ftserver is started by providing it a port; Example: "./ftserver [port]"
3. ftclient.py is started with "python3 ftclient.py [server host] [server port] [command + args] [data port]"
4. Both programs will display status text of what is going on and if there are any errors.
