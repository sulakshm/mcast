#ifndef _MCAST_CLIENT_H_
#define _MCAST_CLIENT_H_

struct mcast_client_context;

struct sockaddr_in mcast_destination();

int mcast_client_init(const char *localaddr, bool disable_loopback_msg);
int mcast_send(mcast_client_context *ctx, const char *msg, size_t len);


#endif /* _MCAST_CLIENT_H_ */
