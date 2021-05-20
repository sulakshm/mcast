#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mcastclient.h"
#include "mcastcommon.h"

sockaddr_in *mcast_client::mcast_dest = nullptr;
sockaddr_in mcast_client::groupSock;

struct sockaddr_in *mcast_client::mcast_destination() {

        if (mcast_dest != nullptr) {
                return mcast_dest;
        }

        memset((char *)&groupSock, 0, sizeof(groupSock));
        groupSock.sin_family = AF_INET;
        groupSock.sin_addr.s_addr = inet_addr(MCASTGROUP);
        groupSock.sin_port = htons(MCASTPORT);

        mcast_dest = &groupSock;
        return mcast_dest;
}

mcast_client::~mcast_client() { destroy(); }

void mcast_client::destroy() {
        if (sd != 0) {
                close(sd);
                sd = 0;
        }
}

std::unique_ptr<mcast_client> mcast_client::make(std::string &localaddr,
                                                 bool disable_loopback_msg) {
        auto c = std::make_unique<mcast_client>();
        auto rc = c->init(localaddr, disable_loopback_msg);
        if (rc < 0) {
                c.release();
                return c; // returns null context
        }
        c->sd = rc; // initialize successful socket descriptor
        return c;
}

int mcast_client::init(const std::string &localaddr,
                       bool disable_loopback_msg) {
        int sock;
        int rc;
        const int loopch = 0;
        struct in_addr localInterface;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
                perror("Opening datagram socket error");
                return sock;
        }
        printf("Opening the datagram socket..OK.\n");

        if (disable_loopback_msg) {
                rc = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
                                (char *)&loopch, sizeof(loopch));
                if (rc < 0) {
                        perror("Setting IP_MULTICAST_LOOP error");
                        close(sock);
                        return rc;
                }

                printf("Disabling the loopback..OK.\n");
        }

        /* Set local interface for outbound mcast dgram */
        memset((char *)&localInterface, 0, sizeof(localInterface));
        localInterface.s_addr = inet_addr(localaddr.c_str());
        rc = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
                        (char *)&localInterface, sizeof(localInterface));
        if (rc < 0) {
                perror("Setting local interface error");
                close(sock);
                return rc;
        }

        printf("Setting the local interface...OK\n");

        return sock; // returns successful socket descriptor
}

int mcast_client::send(const char *msg, size_t len) {
        auto groupSock = mcast_destination();
        /* Send a message to the multicast group specified by the*/
        /* groupSock sockaddr structure. */
        int rc = sendto(sd, msg, len, 0, (struct sockaddr *)groupSock,
                        sizeof(mcast_client::groupSock));
        if (rc < 0) {
                perror("Sending datagram message error");
        } else
                printf("Sending datagram message...OK\n");

        return rc;
}

#ifdef ENABLE_MAIN
void help(const char *pgm) {
        std::cout << "help: " << pgm
                  << " <localaddr> <disableloopbackmsg={0|1}>" << std::endl;
        exit(1);
}

int main(int argc, char *argv[]) {
        if (argc != 3)
                help(argv[0]);

        std::string localaddr = std::string(argv[1]);
        const bool disableloopmsg = argv[2][0] == '1' ? true : false;

        std::cout << "argv[2][0]: " << argv[2][0] << std::endl;
        std::cout << "parsed disableloopmsg: " << disableloopmsg << std::endl;

        auto ctx = mcast_client::make(localaddr, disableloopmsg);
        if (ctx == nullptr)
                exit(1);

        const std::string msg = "test message!!!";

        mcast_client_send(ctx, msg, msg.length());
        return 0;
}
#endif
