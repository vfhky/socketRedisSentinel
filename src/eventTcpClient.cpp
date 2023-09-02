#include "eventTcpClient.h"





namespace socketRedisSentinel {


    // tcp recieved data.
    std::string EventTcpClient::m_rcvData = "";

    void EventTcpClient::setTcpNoDelay(evutil_socket_t fd) {
        int one = 1;
        int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        LOG(Debug, "setTcpNoDelay done", fd, ret);
    }

    void EventTcpClient::eventCb(struct bufferevent *bev, short event, void *ctx) {
        evutil_socket_t fd = bufferevent_getfd(bev);
        int i_errCode = EVUTIL_SOCKET_ERROR();
        LOG(Debug, fd, i_errCode, event, evutil_socket_error_to_string(i_errCode) );

        bool bLoopExit = true;
        if(event & BEV_EVENT_TIMEOUT) {
            LOG(Debug, "eventCb timeout reached");
        } else if (event & BEV_EVENT_CONNECTED) {
            EventTcpClient::setTcpNoDelay(fd);
            bLoopExit = false;
            LOG(Debug, "eventCb connection success");
        } else if (event & BEV_EVENT_EOF) {
            LOG(Debug, "eventCb connection closed");
        } else if (event & BEV_EVENT_ERROR) {
            LOG(Error, "eventCb some other error");
        } else if(event & BEV_EVENT_READING) {
            bLoopExit = false;
            LOG(Debug, "eventCb read data is ready");
        } else if(event & BEV_EVENT_WRITING ) {
            bLoopExit = false;
            LOG(Debug, "eventCb write data is ready");
        } else {
            LOG(Debug, "eventCb unkown event callback", event);
        }

        // break loop
        if (bLoopExit && NULL != ctx) {
            event_base* base = static_cast<event_base*>(ctx);
            event_base_loopexit(base, NULL);
            LOG(Debug, "eventCb event_base_loopexit");
        }
    }


    void EventTcpClient::sendDataCb(evutil_socket_t fd, short events, void *arg) {
        bufferevent *bev = static_cast<bufferevent *>(arg);

        // send data to server
        std::string reqData = "test";
        if (0 != bufferevent_write(bev, reqData.c_str(), reqData.size())) {
            LOG(Error, "send data to server fail", reqData);
        }
    }


    bool EventTcpClient::sendData(struct bufferevent* bev, const std::string &data) {
        if (data.empty()) {
            LOG(Info, data);
            return true;
        }

        string reqData = data + "\n";
        if (0 == bufferevent_write(bev, reqData.c_str(), reqData.size())) {
            LOG(Debug, "bufferevent_write ok , wait callback", reqData);
            return true;
        }

        LOG(Error, "bufferevent_write fail", reqData, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        return false;
    }

    // callback function when recieved data from server.
    void EventTcpClient::readCb(struct bufferevent* bev, void* ctx) {
        struct evbuffer* input = bufferevent_get_input(bev);
        std::string data = "";

        char *buffer = new char[SOCKET_DATA_BATCH_SIZE];
        size_t readSize = 0;
        while (NULL != input) {
            memset(buffer, 0x00, sizeof(buffer));
            size_t len = evbuffer_remove(input, buffer, SOCKET_DATA_BATCH_SIZE - 1);
            if (len <= 0) {
                break;
            }
            // buffer[len] = '\0';
            data += buffer;
            readSize += len;
        }
        delete []buffer;

        // set the recieved data to memeber.
        m_rcvData = data;

        // break loop
        if (NULL != ctx) {
            event_base* base = static_cast<event_base*>(ctx);
            event_base_loopexit(base, NULL);
        }

        LOG(Debug, "readCb ok", ctx, readSize, m_rcvData);
    }

    // callback function when data was send to server.
    void EventTcpClient::writeCb(struct bufferevent *bev, void *ctx) {
        LOG(Info, "writeCb done");
    }

    void EventTcpClient::reqTimeoutCb(evutil_socket_t fd, short event, void* arg) {
        bool bArg = NULL != arg;
        if (bArg) {
            struct event_base* base = static_cast<struct event_base*>(arg);
            event_base_loopbreak(base);
        }

        m_rcvData.clear();
        LOG(Info, "timeout reached", fd, event, bArg, m_rcvData);
    }


    bool EventTcpClient::doRequest(const std::string &ip, u_short port, const std::string &reqData,
            int16_t timeoutMics, std::string &rcvData) {
        event_base* base = event_base_new();
        if (NULL == base) {
            LOG(Error, "event_base_new fail", ip, port, timeoutMics, \
                evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()), reqData);
            return false;
        }

        bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

        // callback
        bufferevent_data_cb rCb = EventTcpClient::readCb;
        bufferevent_data_cb wCb = EventTcpClient::writeCb;;
        bufferevent_event_cb eventCb = EventTcpClient::eventCb;
        bufferevent_setcb(bev, rCb, wCb, eventCb, base);
        bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);

        // total request timeout callback
        struct event* reqTimeoutEvent = evtimer_new(base, reqTimeoutCb, base);
        struct timeval reqTimeout;
        reqTimeout.tv_sec = timeoutMics / 1000;
        reqTimeout.tv_usec = timeoutMics % 1000;
        evtimer_add(reqTimeoutEvent, &reqTimeout);

        // connect server
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        // Convert IPv4 and IPv6 addresses from text to binary
        inet_pton(AF_INET, ip.c_str(), &(serverAddr.sin_addr));
        if (bufferevent_socket_connect(bev, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            LOG(Error, "bufferevent_socket_connect fail", ip, port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()), reqData);
            bufferevent_free(bev);
            event_free(reqTimeoutEvent);
            event_base_free(base);
            return false;
        }

        // make an event
        #if 0
        evutil_socket_t fd = bufferevent_getfd(bev);
        struct event* ev = event_new(base, fd, EV_READ | EV_PERSIST, sendDataCb, bev);
        event_add(ev, NULL);
        #endif
        // send data directly
        if (!EventTcpClient::sendData(bev, reqData)) {
            LOG(Error, "sendData fail", ip, port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()), reqData);
            bufferevent_free(bev);
            event_free(reqTimeoutEvent);
            event_base_free(base);
            return false;
        }

        // run forever
        event_base_dispatch(base);

        // release source
        if (NULL != bev) {
            bufferevent_free(bev);
        }
        if (NULL != reqTimeoutEvent) {
            event_free(reqTimeoutEvent);
        }
        if (NULL != base) {
            event_base_free(base);
        }

        // return recieved data
        rcvData = m_rcvData;

        LOG(Debug, "done", ip, port, timeoutMics, rcvData, reqData);
        return true;
    }















}

