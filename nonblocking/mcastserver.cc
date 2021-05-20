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

struct mcast_server_context {
        int sd = 0;
        std::vector<std::string> ifaces;
        bool inited = false;

        mcast_server_context() = default;
        mcast_server_context(std::vector<std::string> &ref)
            : sd(0), ifaces(ref) {}

        static std::unique_ptr<mcast_server_context> make() {
                return std::make_unique<mcast_server_context>();
        }

        static std::unique_ptr<mcast_server_context>
        make(const std::string &iface) {
                auto c = mcast_server_context::make();
                c->ifaces.emplace_back(iface);
                return c;
        }

        static std::unique_ptr<mcast_server_context>
        make(std::vector<std::string> &ifaces) {
                auto c = mcast_server_context::make();

                for (auto &i : ifaces) {
                        c->ifaces.emplace_back(i);
                }
                return c;
        }
};

int mcast_server_add_interface(std::unique_ptr<mcast_server_context> &c,
                               const std::string &iface) {
        if (c->inited) {
                perror("cannot add interface after initializing");
                return -EINVAL;
        }

        for (auto &i : c->ifaces) {
                if (i == iface) {
                        return -EEXIST;
                }
        }

        c->ifaces.emplace_back(iface);
        return 0;
}

int mcast_server_context_init(std::unique_ptr<mcast_server_context> &c) {
        const int reuse = 1;
        int sd;
        int rc;
        struct sockaddr_in localSock;
        struct ip_mreq group;

        if (c->inited) {
                perror("server context already initialized");
                return -EINVAL;
        }

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
        for (auto &iface : c->ifaces) {
                rc = mcast_membership_add(sd, MCASTGROUP, iface.c_str());
                if (rc < 0) {
                        close(sd);
                        return rc;
                }
                printf("mcast membership %s added to interface %s... OK.\n",
                       MCASTGROUP, iface.c_str());
        }

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

        c->sd = sd;
        c->inited = true;
        return 0;
}

std::unique_ptr<mcast_server_context> mcast_server_init(const char *localaddr) {
        auto c = mcast_server_context::make(localaddr);
        auto rc = mcast_server_context_init(c);
        if (rc != 0) {
                perror("mcast_server_context_init failed");
                c.release();
        }

        return c;
}

void cleanup(std::unique_ptr<mcast_server_context> &ctx) {
        if (ctx == nullptr)
                return;

        if (ctx->inited)
                close(ctx->sd);
        ctx.release();
}

int handler(std::unique_ptr<mcast_server_context> &&ctx) {
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
        auto ctx = mcast_server_init(localaddr);
        if (ctx == nullptr) {
                return -1;
        }

        std::thread mythread(handler, std::move(ctx));
        mythread.join();
        return 0;
}
