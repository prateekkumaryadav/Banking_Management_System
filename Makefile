# Makefile for the Banking System Project
#
# CC: C Compiler
# CFLAGS: Compiler flags
#   -g: Add debugging symbols
#   -Wall: Turn on all warnings
# LDFLAGS: Linker flags
#   -pthread: Link the POSIX threads library (required for server)

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -pthread

# Define all target executables
all: server client setup clear_sessions

# Rule to build the server
server: server.c common.h
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

# Rule to build the client
client: client.c common.h
	$(CC) $(CFLAGS) -o client client.c

# Rule to build the setup utility
setup: setup.c common.h
	$(CC) $(CFLAGS) -o setup setup.c

# Rule to build the clear_sessions utility
clear_sessions: clear_sessions.c common.h
	$(CC) $(CFLAGS) -o clear_sessions clear_sessions.c

# Rule to clean up build files and data
clean:
	rm -f server client setup clear_sessions
	rm -f data/*.dat
	rmdir data

.PHONY: all clean