/**
 * Copyright (c) 2023-07, typecodes.com (vfhky@typecodes.com)
 *
 * All rights reserved.
 *
 * The entrance of creating a server like http/https/tcp.
 */


#include "eventServer.h"
#include "eventHttpServer.h"
#include "eventHttpsServer.h"
#include "eventTcpServer.h"





namespace socketRedisSentinel {

    void EventServer::signalCb(evutil_socket_t sig, short event, void *arg) {
        struct event_base *base = static_cast<event_base *>(arg);

        switch(sig) {
            case SIGHUP:
                event_base_loopbreak(base);
                LOG(Info, "Hangup signal catched.");
                break;
            case SIGTERM:
                event_base_loopbreak(base);
                LOG(Info, "Terminate signal catched.");
                break;
            case SIGUSR1:
                Config::instance().setLogLv(Debug);
                Config::instance().setLogType(LOG_TYPE_STDOUT);
                LOG(Info, "reload config");
                break;
            default:
                event_base_loopbreak(base);
                LOG(Info, "recv sinal", sig);
                break;
        }
    }

    // register signal callback
    void EventServer::registerSignalCb(struct event_base *base) {
        struct event *signalInt = evsignal_new(base, SIGINT, signalCb, base);
        event_add(signalInt, NULL);
        struct event *signalTerm = evsignal_new(base, SIGTERM, signalCb, base);
        event_add(signalTerm, NULL);
        struct event *signalHup = evsignal_new(base, SIGHUP, signalCb, base);
        event_add(signalHup, NULL);
    }


    int EventServer::init() {
        const string libEventVer = event_get_version();
        LOG(Debug, "using libEvent.", libEventVer);

        struct event_base *base = event_base_new();
        if (NULL == base) {
            LOG(Error, "event_base_new failed", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
            return -1;
        }

        // create tcp server
        struct evconnlistener *listener = EventTcpServer::instance().createTcpServer(base);
        if (NULL == listener) {
            return -2;
        }

        // create http server
        struct evhttp *http = EventHttpServer::instance().createHttpServer(base);
        if (NULL == http) {
            return -3;
        }

#ifdef _START_HTTPS_SERVER
        // create https server
        struct evhttp *https = EventHttpsServer::instance().createHttpsServer(base);
        if (NULL == https) {
            return -4;
        }
#endif

        // register signal callback
        EventServer::registerSignalCb(base);

        // run forever
        event_base_dispatch(base);

        // clean up
        if (NULL != listener) {
            evconnlistener_free(listener);
        }
        if (NULL != http) {
            evhttp_free(http);
        }
#ifdef _START_HTTPS_SERVER
        if (NULL != https) {
            evhttp_free(https);
        }
#endif
        if (NULL != base) {
            event_base_free(base);
        }

        LOG(Error, "something err occur", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        return -5;
    }












}


