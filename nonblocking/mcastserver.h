#ifndef _MCAST_SERVER_H_
#define _MCAST_SERVER_H_

class mcast_server {
      public:
        mcast_server() = default;
        mcast_server(std::vector<std::string> &ref) : sd(0), ifaces(ref) {}

        ~mcast_server();

        static std::unique_ptr<mcast_server> make(const std::string &iface) {
                auto c = mcast_server::make();
                c->ifaces.emplace_back(iface);

                if (c->init()) {
                        // init was not successful
                        c.release();
                }
                return c;
        }

        static std::unique_ptr<mcast_server>
        make(std::vector<std::string> &ifaces) {
                auto c = mcast_server::make();

                for (auto &i : ifaces) {
                        c->ifaces.emplace_back(i);
                }
                if (c->init()) {
                        // init was not successful
                        c.release();
                }
                return c;
        }

        int init(void);
        int add_interface(const std::string &iface);

        static int handler(std::unique_ptr<mcast_server> &&ctx);

      private:
        static std::unique_ptr<mcast_server> make() {
                return std::make_unique<mcast_server>();
        }

        int sd = 0;
        std::vector<std::string> ifaces;
        bool inited = false;
};

#endif /* _MCAST_SERVER_H_ */
