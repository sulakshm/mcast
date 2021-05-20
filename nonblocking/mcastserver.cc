#include <iostream>
#include <thread>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mcastcommon.h"

struct mcast_server_context {
        int sd;
};

int mcast_server_init(const char *localaddr) {
        const int reuse = 1;
        int sd;
        int rc;
        struct sockaddr_in localSock;
        struct ip_mreq group;

        sd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sd < 0) {
                perror("Opening datagram socket error");
                return sd;
        }
        printf("Opening datagram socket....OK.\n");

        rc = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                        sizeof(reuse));
        if (rc < 0) {
                perror("Setting SO_REUSEADDR error");
                close(sd);
                return rc;
        }

        printf("Setting SO_REUSEADDR...OK.\n");

        /* Bind to the proper port number with the IP address */
        /* specified as INADDR_ANY. */
        memset((char *)&localSock, 0, sizeof(localSock));

        localSock.sin_family = AF_INET;
        localSock.sin_port = htons(LOCALPORT);
        // localSock.sin_addr.s_addr = inet_addr(localaddr);
        localSock.sin_addr.s_addr = INADDR_ANY;

        rc = bind(sd, (struct sockaddr *)&localSock, sizeof(localSock));
        if (rc != 0) {
                perror("Binding datagram socket error");
                close(sd);
                return rc;
        }
        printf("Binding datagram socket on port %d...OK.\n", LOCALPORT);

        /* Join the multicast group on the local interface */
        /* Note that this IP_ADD_MEMBERSHIP option must be */
        /* called for each local interface over which the multicast */
        /* datagrams are to be received. */
        rc = mcast_membership_add(sd, MCASTGROUP, localaddr);
        if (rc < 0) {
                close(sd);
                return rc;
        }
        printf("mcast membership %s added... OK.\n", MCASTGROUP);

        rc = fcntl(sd, F_GETFL, 0);
        if (rc < 0) {
                perror("fcntl get on socket error");
                close(sd);
                return rc;
        }

        rc = fcntl(sd, F_SETFL, rc | O_NONBLOCK);
        if (rc < 0) {
                perror("fcntl set on socket error");
                close(sd);
                return rc;
        }

        return sd;
}

void cleanup(mcast_server_context *ctx) { close(ctx->sd); }

int handler(mcast_server_context *ctx) {
        struct pollfd fds[1];
        char buff[1024];
        int timeout = (3 * 60 * 1000); // in msecs

        memset(fds, 0, sizeof(fds));

        fds[0].fd = ctx->sd;
        fds[0].events = POLLIN;

        while (true) {
                std::cout << "waiting on poll" << std::endl;
                int rc = poll(fds, 1, timeout);
                if (rc < 0) {
                        perror("poll failed");
                        cleanup(ctx);
                        return rc;
                }

                if (rc == 0) { // timeout reached, exit program
                        std::cout << "timedout" << std::endl;
                        cleanup(ctx);
                        return rc;
                }

                int curr = 1;
                for (int i = 0; i < curr; i++) {
                        if (fds[i].revents != POLLIN) {
                                std::cout
                                    << "Error! revents = " << fds[i].revents
                                    << std::endl;
                                cleanup(ctx);
                                return -1;
                        }

                        if (fds[i].fd == ctx->sd) {
                                // readable socket
                                rc = recv(fds[i].fd, buff, sizeof(buff), 0);
                                if (rc < 0) {
                                        if (errno != EWOULDBLOCK) {
                                                perror(" recv failed ");
                                                cleanup(ctx);
                                                return rc;
                                        }
                                        continue;
                                }

                                int len = rc;
                                std::cout << "received " << len << " bytes"
                                          << std::endl;
                                std::cout << buff << std::endl;
                        }
                }
        }

        return 0;
}

void help(const char *pgm) {
        std::cout << "help: " << pgm << " <localaddr>" << std::endl;
        exit(1);
}

int main(int argc, char *argv[]) {
        // std::cout << "argc: " << argc << std::endl;

        if (argc != 2)
                help(argv[0]);

        const char *localaddr = argv[1];
        int sd = mcast_server_init(localaddr);
		if (sd < 0) exit(1);

        struct mcast_server_context ctx = {.sd = sd};
        std::thread mythread(handler, &ctx);

        mythread.join();
        return 0;
}
