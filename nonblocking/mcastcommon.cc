#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mcastcommon.h"

int mcast_membership_add(int sd, const char *groupaddr, const char *localaddr) {
        struct ip_mreq group;
        int rc;

        group.imr_multiaddr.s_addr = inet_addr(groupaddr);
        group.imr_interface.s_addr = inet_addr(localaddr);

        rc = setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group,
                        sizeof(group));
        if (rc < 0) {
                perror("Adding multicast group error");
                return rc;
        }
        printf("Adding multicast group...OK.\n");
        return 0;
}

struct sockaddr_in mcast_destination_sock() {
        struct sockaddr_in groupSock;

        memset((char *)&groupSock, 0, sizeof(groupSock));
        groupSock.sin_family = AF_INET;
        groupSock.sin_addr.s_addr = inet_addr(MCASTGROUP);
        groupSock.sin_port = htons(LOCALPORT);

        return groupSock;
}
