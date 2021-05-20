SERVER:=mcastserver.cc
CLIENT:=mcastclient.cc
COMMON:=mcastcommon.cc
COMMON_H:=mcastcommon.h

SERVER_H:=${COMMON_H} mcastserver.h
CLIENT_H:=${COMMON_H} mcastclient.h


server: ${SERVER} ${COMMON} ${SERVER_H}
	g++ -g -std=c++17 -o server ${SERVER} ${COMMON} -lpthread

client: ${CLIENT} ${COMMON} ${CLIENT_H}
	g++ -std=c++17 -g -o client ${CLIENT} ${COMMON} -lpthread


clean:
	rm -f server client *.o
