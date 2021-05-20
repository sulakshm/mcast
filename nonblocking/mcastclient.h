#ifndef _MCAST_CLIENT_H_
#define _MCAST_CLIENT_H_

#include <memory>

#include <sys/socket.h>

class mcast_client {
      public:
        mcast_client() = default;
        ~mcast_client();

        static struct sockaddr_in *mcast_destination();
        static std::unique_ptr<mcast_client>
        make(std::string &localaddr, bool disable_loopback_msg = true);

        int send(const char *msg, size_t len);
        int init(const std::string &localaddr,
                 bool disable_loopback_msg = true);

      private:
        void destroy();

        int sd;

        static struct sockaddr_in *mcast_dest;
        static struct sockaddr_in groupSock;
};

static inline int mcast_client_init(std::unique_ptr<mcast_client> &ctx,
                                    std::string &localaddr) {
        if (ctx != nullptr) {
                return ctx->init(localaddr);
        }

        return -EINVAL;
}

static inline void mcast_client_destroy(std::unique_ptr<mcast_client> &ctx) {
        if (ctx != nullptr) {
                ctx.release();
        }
}

static inline int mcast_client_send(std::unique_ptr<mcast_client> &ctx,
                                    const std::string &msg, size_t len) {
        if (ctx != nullptr) {
                return ctx->send(msg.c_str(), len);
        }

        return -EINVAL;
}

#endif /* _MCAST_CLIENT_H_ */
