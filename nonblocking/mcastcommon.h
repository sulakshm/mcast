#ifndef _MCAST_COMMON_H_
#define _MCAST_COMMON_H_

#define LOCALPORT (9044)
#define MCASTGROUP "226.1.1.1"

int mcast_membership_add(int sd, const char *group, const char *localaddr);
struct sockaddr_in mcast_destination_sock();

#endif /* _MCAST_COMMON_H_ */
