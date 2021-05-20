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

struct mcast_client_context {
        int sd;
};

struct sockaddr_in mcast_destination() {
        struct sockaddr_in groupSock;

        memset((char *)&groupSock, 0, sizeof(groupSock));
        groupSock.sin_family = AF_INET;
        groupSock.sin_addr.s_addr = inet_addr(MCASTGROUP);
        groupSock.sin_port = htons(LOCALPORT);

        return groupSock;
}

int mcast_client_init(const char *localaddr, bool disable_loopback_msg) {
        int sd;
        int rc;
        const int loopch = 0;
        struct in_addr localInterface;

        sd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sd < 0) {
                perror("Opening datagram socket error");
                return sd;
        }
        printf("Opening the datagram socket..OK.\n");

        if (disable_loopback_msg) {
                rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP,
                                (char *)&loopch, sizeof(loopch));
                if (rc < 0) {
                        perror("Setting IP_MULTICAST_LOOP error");
                        close(sd);
                        return rc;
                }

                printf("Disabling the loopback..OK.\n");
        }

        /* Set local interface for outbound mcast dgram */
        memset((char *)&localInterface, 0, sizeof(localInterface));
        localInterface.s_addr = inet_addr(localaddr);
        rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF,
                        (char *)&localInterface, sizeof(localInterface));
        if (rc < 0) {
                perror("Setting local interface error");
                close(sd);
                return rc;
        }

        printf("Setting the local interface...OK\n");
        return sd;
}

int mcast_send(mcast_client_context *ctx, const char *msg, size_t len) {
        auto groupSock = mcast_destination();
        /* Send a message to the multicast group specified by the*/
        /* groupSock sockaddr structure. */
        int rc = sendto(ctx->sd, msg, len, 0, (struct sockaddr *)&groupSock,
                        sizeof(groupSock));
        if (rc < 0) {
                perror("Sending datagram message error");
        } else
                printf("Sending datagram message...OK\n");

        return rc;
}

void help(const char *pgm) {
        std::cout << "help: " << pgm
                  << " <localaddr> <disableloopbackmsg={0|1}>" << std::endl;
        exit(1);
}

int main(int argc, char *argv[]) {
        if (argc != 3)
                help(argv[0]);

        const char *localaddr = argv[1];
        const bool disableloopmsg = argv[2][0] == '1' ? true : false;

        std::cout << "argv[2][0]: " << argv[2][0] << std::endl;
        std::cout << "parsed disableloopmsg: " << disableloopmsg << std::endl;

        int sd = mcast_client_init(localaddr, disableloopmsg);
        if (sd < 0)
                exit(1);

        mcast_client_context ctx = {.sd = sd};

        const std::string msg = "test message!!!";

        mcast_send(&ctx, msg.c_str(), msg.length());
        return 0;
}
