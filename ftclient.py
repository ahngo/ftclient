#!/usr/bin/env python3

###############################################################################
## Program name: ftclient.py (Python3)
#
# Description: ftclient, given a hostname and port connects to ftserver. ftclient
# requires a command and data port of which ftserver connects back to ftclient
# to send data specific to the command given.
#
# Commands: -l (directory listing), -g [filename] (get filename)
# Usage: python3 ftclient.py [host] [port] [command] [filename*] [data port]
# * filename only required for -g (get)
#
###############################################################################

import socket
import sys
import time
import os

# Maximum transmission size
MAX_LIMIT = 1024

###############################################################################
# getMsg()
# Description: Receives text from a file descriptor and decodes it from bytes 
# to plaintext.
#
# Pre-conditions: A valid connection exists from which data will be sent.
# Post-conditions: Places text from the connection into a variable named data.
###############################################################################

def getMsg(connection):
    data = connection.recv(MAX_LIMIT)
    data = data.decode("utf-8")
    data = data.strip('\n')
    return data

###############################################################################
# syntaxCheck()
# Description: Given an array of command line arguments, ensures proper length
# of arguments and syntax usage. Calls badSyntaxExit() if errors are found.
#
# Pre-conditions: None
# Post-conditions: Calls badSyntaxExit() on bad length or syntax usage.
###############################################################################

def syntaxCheck(args):
    if (len(args) < 5 or len(args) > 6):
        badSyntaxExit()
    if (args[3] == "-g" and len(args) < 6):
        badSyntaxExit()

###############################################################################
# badSyntaxExit()
# Description: Called when bad length of command line arguments or syntax is found.
#
# Pre-conditions: None
# Post-conditions: Prints proper command line argument syntax and exits program.
###############################################################################

def badSyntaxExit():
    print("USAGE: python3 ftclient.py [server host] [server port] [command] [filename] [data port]")
    print("Available commands: List: -l or Get: -g [filename]")
    sys.exit(1)
    

###############################################################################
# initiateContact()
# Description: Creates a TCP socket and attempts to connect another host.
#
# Pre-conditions: Proper host and ports are provided via command line.
# Post-conditions: Creates socket, connects to the host and returns the socket.
###############################################################################

def initiateContact():
    host = sys.argv[1]
    port = sys.argv[2]
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, int(port)))
    return s

###############################################################################
# makeRequest()
# Description: Creates a string that is sent to ftserver, providing information
# regarding the dat aport, command, and filename.
#
# Pre-conditions: Proper data port is provided via command line arguments.
# Post-conditions: Creates a string and returns it.
###############################################################################

def makeRequest(dataPort):
    # Embed data port in string
    startString = ("PORTSTART:" + str(dataPort))
    startString += "PORTEND"
    startString += "CMD:"
    if sys.argv[3] == "-l":
        # List
        startString += "LIST"
        # Get
    elif sys.argv[3] == "-g":
        startString += "GET"
        startString += "FILENAME:"
        startString += sys.argv[4]
        startString += "FILENAMEEND"
        # Other 
    else: startString += "UNKNOWN"

    # The server expects to receive a 100-byte string, so it is padded.
    while len(startString) < 99:
        startString += "#"
    startString += '\0'
    return startString

###############################################################################
# getDataPort()
# Description: Depending on the command provided, the data port changes position
# within the array of command line arguments. This function extracts it, depending
# on the length of the argument array.
#
# Pre-conditions: Proper data port is provided via command line arguments.
# Post-conditions: Extracts the string used for the data port and returns it
# as a number.
###############################################################################

def getDataPort():
    if ((len(sys.argv)) == 5):
        dataPort = sys.argv[4]
    else:
        dataPort = sys.argv[5]
    return int(dataPort)

###############################################################################
# receiveData()
# Description: Facilitates the command given by the user upon starting the
# program. Gets a directory listing or facilitates file transfer, prompting
# for overwriting a file if a duplicate file exists.
#
# Pre-conditions: A valid file descriptor is provided.
# Post-conditions: Prints a directory listing or facilitates file transfer.
###############################################################################

def receiveData(controlFD, dataFD):
    # Expecting a directory listing
    if sys.argv[3] == "-l":
        dirListing = getMsg(dataFD)
        print("Directory contents:")
        print(dirListing)
        return
    
    # Expecting a file; handle duplicate files
    if sys.argv[3] == "-g":
        status = getMsg(controlFD)
        if "ERROR" in status:
            print("Message from server: " + status)
            controlFD.close()
            dataFD.close()
            return
        
        if os.path.isfile(sys.argv[4]):
            overwrite = input(sys.argv[4] + " already exists. Overwrite? N = no, anything else = yes\n")
            if overwrite.lower() == 'n':
                print("Transfer aborted.")
                return
        
        with open(sys.argv[4], 'wb') as f:
            print("Transferring " + sys.argv[4] + ".")
            while True:
                data = dataFD.recv(1024)
                if not data:
                    break
                f.write(data)
        print("Transfer complete.")        
        f.close()
        return

###############################################################################
# socketStart()
# Description: Creates a TCP socket and binds it .
#
# Pre-conditions: Valid host and port strings are provided.
# Post-conditions: Returns a socket bound to the host and port.
###############################################################################

def socketStart(HOST, PORT):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)		
    # Reuse socket if it is still in use by the OS
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, int(PORT)))
    return s
    
###############################################################################
# Main Program
#
# Basic program flow: Syntax check, listen on data port, connect to server on 
# control port, send the request in the form of a string, server connects on
# data port, client receives data or an error message back. Handles duplicate
# files by asking to overwrite. Handles unknown commands and non-existent file
# names.
###############################################################################

# Check syntax
syntaxCheck(sys.argv)

# Extract server host, control port, data port
host = sys.argv[1]
port = sys.argv[2]
dataPort = getDataPort()

# Create a string to send to server detailing data port and command
startString = makeRequest(dataPort)

# Listen on data port
s2 = socketStart('', dataPort)
s2.listen()
print("Listening on data port " + str(dataPort))

# Connect to the server on a control port and send the info string
s = initiateContact()
time.sleep(1)
s.send(bytes(startString, "utf8"))

# Check if the server has acknowledged a valid command or an error message
confirm = getMsg(s)
if "ERROR:" in confirm:
    print(confirm)
    s.close()
    s2.close()
    sys.exit(0)
else: print("Connected to ftserver on control port " + port)

# Accept connections on the data port
dpConnection, client_address = s2.accept()
print("ftserver connected on data port " + str(dataPort))
time.sleep(1)

# Get data from the server on data port connection
receiveData(s, dpConnection)

# Close sockets and connections and end program
s.close()
s2.close()
dpConnection.close()
print("** Operations complete. Closing connections. **")
sys.exit(0)