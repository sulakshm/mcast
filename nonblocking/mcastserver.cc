#include <iostream>
#include <thread>
#include <vector>

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
#include "mcastserver.h"

int mcast_server::add_interface(const std::string &iface) {
        for (auto &i : ifaces) {
                if (i == iface) {
                        // return -EEXIST;
                        return 0;
                }
        }

        ifaces.emplace_back(iface);

        if (inited) {
                auto rc = mcast_membership_add(sd, MCASTGROUP, iface.c_str());
                if (rc < 0) {
                        return rc;
                }
                printf("mcast membership %s added to interface %s... OK.\n",
                       MCASTGROUP, iface.c_str());
        }
        return 0;
}

int mcast_server::init() {
        const int reuse = 1;
        struct sockaddr_in localSock;
        int sock;
        int rc;

        if (inited) {
                perror("server context already initialized");
                return -EINVAL;
        }

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
                perror("Opening datagram socket error");
                return sock;
        }
        printf("Opening datagram socket....OK.\n");

        rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                        sizeof(reuse));
        if (rc < 0) {
                perror("Setting SO_REUSEADDR error");
                close(sock);
                return rc;
        }

        printf("Setting SO_REUSEADDR...OK.\n");

        /* Bind to the proper port number with the IP address */
        /* specified as INADDR_ANY. */
        memset((char *)&localSock, 0, sizeof(localSock));

        localSock.sin_family = AF_INET;
        localSock.sin_port = htons(MCASTPORT);
        // localSock.sin_addr.s_addr = inet_addr(localaddr);
        localSock.sin_addr.s_addr = INADDR_ANY;

        rc = bind(sock, (struct sockaddr *)&localSock, sizeof(localSock));
        if (rc != 0) {
                perror("Binding datagram socket error");
                close(sock);
                return rc;
        }
        printf("Binding datagram socket on port %d...OK.\n", MCASTPORT);

        /* Join the multicast group on the local interface */
        /* Note that this IP_ADD_MEMBERSHIP option must be */
        /* called for each local interface over which the multicast */
        /* datagrams are to be received. */
        for (auto &iface : ifaces) {
                rc = mcast_membership_add(sock, MCASTGROUP, iface.c_str());
                if (rc < 0) {
                        close(sock);
                        return rc;
                }
                printf("mcast membership %s added to interface %s... OK.\n",
                       MCASTGROUP, iface.c_str());
        }

        rc = fcntl(sock, F_GETFL, 0);
        if (rc < 0) {
                perror("fcntl get on socket error");
                close(sock);
                return rc;
        }

        rc = fcntl(sock, F_SETFL, rc | O_NONBLOCK);
        if (rc < 0) {
                perror("fcntl set on socket error");
                close(sock);
                return rc;
        }

        sd = sock;
        inited = true;
        return 0;
}

std::unique_ptr<mcast_server> mcast_server_init(const char *localaddr) {
        auto c = mcast_server::make(localaddr);
        auto rc = c->init();
        if (rc != 0) {
                perror("mcast_server_init failed");
                c.release();
        }

        return c;
}

mcast_server::~mcast_server() {
        if (inited)
                close(sd);
}

int mcast_server::handler(std::unique_ptr<mcast_server> &&ctx) {
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
                        ctx.release();
                        return rc;
                }

                if (rc == 0) { // timeout reached, exit program
                        std::cout << "timedout" << std::endl;
                        ctx.release();
                        return rc;
                }

                int curr = 1;
                for (int i = 0; i < curr; i++) {
                        if (fds[i].revents != POLLIN) {
                                std::cout
                                    << "Error! revents = " << fds[i].revents
                                    << std::endl;
                                ctx.release();
                                return -1;
                        }

                        if (fds[i].fd == ctx->sd) {
                                // readable socket
                                rc = recv(fds[i].fd, buff, sizeof(buff), 0);
                                if (rc < 0) {
                                        if (errno != EWOULDBLOCK) {
                                                perror(" recv failed ");
                                                ctx.release();
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

#ifdef ENABLE_MAIN
void help(const char *pgm) {
        std::cout << "help: " << pgm << " <localaddr>" << std::endl;
        exit(1);
}

int main(int argc, char *argv[]) {
        // std::cout << "argc: " << argc << std::endl;

        if (argc != 2)
                help(argv[0]);

        const char *localaddr = argv[1];
        auto ctx = mcast_server::make(localaddr);
        if (ctx == nullptr) {
                return -1;
        }

        std::thread mythread(&mcast_server::handler, std::move(ctx));
        mythread.join();
        return 0;
}
#endif
