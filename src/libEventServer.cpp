#include "libEventServer.h"





namespace scoketRedisSentinel {





    void LibEventServer::readCb(struct bufferevent *bev, void *ctx) {
        char buf[4096] = {0x00};
        size_t readSize = bufferevent_read(bev, buf, sizeof(buf) - 1);
        if (readSize < 0) {
            LOG(Error, "bufferevent_read failed", buf);
            exit(1);
        }

        buf[readSize] = '\0';
        LOG(Info, readSize, buf);
    }

    void LibEventServer::writeCb(struct bufferevent *bev, void *ctx) {
        char buf[4096] = {0x00};
        size_t readSize = bufferevent_read(bev, buf, sizeof(buf));
        if (readSize < 0) {
            LOG(Error, "bufferevent_read failed", buf);
            exit(1);
        }

        LOG(Info, readSize, buf);
    }

    void LibEventServer::eventCb(struct bufferevent *bev, short events, void *ctx) {
        if (events & BEV_EVENT_EOF) {
            LOG(Error, "connection closed");
        } else if (events & BEV_EVENT_ERROR) {
            LOG(Error, "some other error");
        } else {

        }

        bufferevent_free(bev);
    }



    void LibEventServer::listenerCb(struct evconnlistener *listener, evutil_socket_t fd,
                struct sockaddr *address, int socklen, void *ctx)
    {
        LOG(Debug, "accept client", fd);

        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        if (NULL == bev) {
            LOG(Error, "bufferevent_socket_new failed", fd);
            return;
        }


        bufferevent_data_cb rCb = LibEventServer::readCb;
        bufferevent_data_cb wCb = NULL;
        bufferevent_event_cb eventCb = LibEventServer::eventCb;

        bufferevent_setcb(bev, rCb, wCb, eventCb, &fd);
        bufferevent_enable(bev, EV_READ | EV_PERSIST);
    }

    int LibEventServer::init() {
        struct event_base *base = event_base_new();
        if (NULL == base) {
            LOG(Error, "event_base_new failed");
            exit(1);
        }

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(8080);
        sin.sin_addr.s_addr = htonl(INADDR_ANY);


        struct evconnlistener *listener = evconnlistener_new_bind(base, listenerCb, NULL,
                                                                LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                                                (struct sockaddr *)&sin, sizeof(sin));
        if (NULL == listener) {
            LOG(Error, "evconnlistener_new_bind failed");
            exit(1);
        }

        event_base_dispatch(base);


        evconnlistener_free(listener);
        event_base_free(base);
        return 0;
    }




}


