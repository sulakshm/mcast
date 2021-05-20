SERVER:=mcastserver.cc
CLIENT:=mcastclient.cc
COMMON:=mcastcommon.cc
COMMON_H:=mcastcommon.h

SERVER_H:=${COMMON_H} mcastserver.h
CLIENT_H:=${COMMON_H} mcastclient.h

LIBS:=-lpthread
OPTS:=-DENABLE_MAIN

default:all

server: ${SERVER} ${COMMON} ${SERVER_H}
	g++ -g -std=c++17 ${OPTS} -o server ${SERVER} ${COMMON} ${LIBS}

client: ${CLIENT} ${COMMON} ${CLIENT_H}
	g++ -std=c++17 -g ${OPTS} -o client ${CLIENT} ${COMMON} ${LIBS}

all: server client
	@echo "built all binaries"

clean:
	rm -f server client *.o
