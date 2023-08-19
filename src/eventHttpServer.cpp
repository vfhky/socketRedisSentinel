#include "eventHttpServer.h"
#include "logicEntrance.h"




/**
 * GET ： $ curl "typecodes.com:10087?type=1&ip=sentinel.typecodes.com&port=20066&redisType=1&poolName=revenueGameTest_001&hashKey=1111"
 * POST post data：
 * $ curl -X POST -d 'type=1&ip=sentinel.typecodes.com&port=20066&redisType=1&poolName=revenueGameTest_001&hashKey=1111' typecodes.com:10087
 * $ curl -s -H "Content-Type: application/json" -X POST -d '{"type":1,"ip":"sentinel.typecodes.com","port":"20066","redisType":1,"poolName":"revenueGameTest_001","hashKey":"1111"}' typecodes.com:10087
 * $ data.json file： $ curl -H "Content-Type: application/json" -X POST -d @data.json  http://typecodes.com:8090

 * request command not support will send 400 status code:
 * root@4d1ef2bcb408:~/testwork/testLibEvent# curl -XDELETE "http://typecodes.com:8090?type=1&ip=typecodes&port=20066"
    <HTML><HEAD>
    <TITLE>400 Bad Request</TITLE>
    </HEAD><BODY>
    <H1>Bad Request</H1>
    </BODY></HTML>
    root@4d1ef2bcb408:~/testwork/testLibEvent#
 */




namespace socketRedisSentinel {



    EventHttpServer &EventHttpServer::instance() {
        static EventHttpServer instance;
        return instance;
    }





    ClientReqInfo EventHttpServer::httpBodyToClientReqInfo(const std::map<std::string, std::string> &httpBody) {
        ClientReqInfo clientReqInfo;

        __foreach(it, httpBody) {
            std::string key = it->first;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            const std::string &value = it->second;
            if (key == "type") {
                clientReqInfo.type = static_cast<CLIENT_REQ_TYPE>(Utils::stringToU32(value));
            } else if (key == "ip") {
                clientReqInfo.ip = value;
            } else if (key == "port") {
                clientReqInfo.port = (uint16_t)Utils::stringToU32(value);
            } else if (key == "redistype") {
                clientReqInfo.redisType = static_cast<CLIENT_REQ_REDIS_TYPE>(Utils::stringToU32(value));
            } else if (key == "poolname") {
                clientReqInfo.poolName = value;
            } else if (key == "hashkey") {
                clientReqInfo.hashKey = value;
            } else if (key == "loglv") {
                clientReqInfo.logLv = Utils::stringToI64(value);
            } else if (key == "logtype") {
                clientReqInfo.logType = Utils::stringToI64(value);
            } else {
                //
            }
        }
        return clientReqInfo;
    }


    const bool EventHttpServer::fillGetParams(HttpReqInfo &httpReqInfo) {
        // /?id=001&name=typecodes&phone=1111
        struct evhttp_uri* uri = evhttp_uri_parse(httpReqInfo.requestUri.c_str());
        if (!uri) {
            LOG(Warn, "Failed to parse URI.");
            return false;
        }

        // do not use evhttp_parse_query_str(requestUri, &params); for it will return key of "/?id"
        struct evkeyvalq params;
        evhttp_parse_query_str(evhttp_uri_get_query(uri), &params);
        struct evkeyval* kv;
        TAILQ_FOREACH(kv, &params, next) {
            httpReqInfo.body[kv->key] = kv->value;
        }

        /**
         * get special param :
         *   const char* id = evhttp_find_header(&params, "id");
        */

        evhttp_clear_headers(&params);
        return true;
    }


    // handle get command
    void EventHttpServer::handleGetReq(struct evhttp_request *req, HttpReqInfo &httpReqInfo, void *arg) {
        if (!EventHttpServer::fillGetParams(httpReqInfo)) {
            evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", NULL);
            LOG(Info, "Bad Request", httpReqInfo.dump());
            return;
        }

        LOG(Info, "fill param end", httpReqInfo.dump());
    }

    const std::map<std::string, std::string> EventHttpServer::parseFormData(const std::string& postData) {
        std::map<std::string, std::string> params;
        size_t pos = 0;
        while (pos < postData.length()) {
            size_t delimiterPos = postData.find('&', pos);
            if (delimiterPos == std::string::npos) {
                delimiterPos = postData.length();
            }

            size_t equalPos = postData.find('=', pos);
            if (equalPos != std::string::npos && equalPos < delimiterPos) {
                std::string key = postData.substr(pos, equalPos - pos);
                std::string value = postData.substr(equalPos + 1, delimiterPos - equalPos - 1);
                params[key] = value;
            }

            pos = delimiterPos + 1;
        }

        return params;
    }

    const bool EventHttpServer::fillPostParams(struct evhttp_request *req, HttpReqInfo &httpReqInfo) {
        struct evbuffer* inbuf = evhttp_request_get_input_buffer(req);
        std::stringstream dataStream;
        while (evbuffer_get_length(inbuf)) {
            int n = 0;
            char cbuf[128] = {0x00};
            n = evbuffer_remove(inbuf, cbuf, sizeof(cbuf));
            if (n > 0) {
                dataStream.write(cbuf, n);
            }
        }
        LOG(Debug, dataStream.str());

        std::map<std::string, std::string> params;
        std::string contentType = "";
        if (httpReqInfo.headers.find("Content-Type") != httpReqInfo.headers.end()) {
            contentType = httpReqInfo.headers["Content-Type"];
        }
        if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
            httpReqInfo.body = EventHttpServer::parseFormData(dataStream.str());
        } else if (contentType.find("application/json") != std::string::npos) {
            httpReqInfo.body = Utils::simpleJsonToMap(dataStream.str());
        } else {
            LOG(Warn, "Unsupported Content-Type", contentType, httpReqInfo.dump());
            return false;
        }

        return true;
    }

    // handle post command
    void EventHttpServer::handlePostReq(struct evhttp_request *req, HttpReqInfo &httpReqInfo, void *arg) {
        // all post params
        if (!EventHttpServer::fillPostParams(req, httpReqInfo)) {
            evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", NULL);
            LOG(Info, "Bad Request", httpReqInfo.dump());
            return;
        }

        LOG(Info, "fill param end", httpReqInfo.dump());
    }

    // make http response to client
    void EventHttpServer::doHttpRsp(struct evhttp_request *req, const std::string &rspData,
                const int &rspCode, const std::string &rspReason) {
        struct evbuffer *returnbuffer = evbuffer_new();
        evbuffer_add_printf(returnbuffer, "%s\n", rspData.c_str());
        evhttp_send_reply(req, rspCode, rspReason.c_str(), returnbuffer);
        evbuffer_free(returnbuffer);
        LOG(Info, "response end", rspData, rspCode, rspReason);
    }

    const void EventHttpServer::fillHeaders(struct evhttp_request *req, HttpReqInfo &httpReqInfo) {
        struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
        struct evkeyval* kv;
        TAILQ_FOREACH(kv, headers, next) {
            httpReqInfo.headers[kv->key] = kv->value;
        }

        evhttp_clear_headers(headers);
    }

    const void EventHttpServer::fillClientIpPort(struct evhttp_request *req, HttpReqInfo &httpReqInfo) {
        struct evhttp_connection* conn = evhttp_request_get_connection(req);
        char * clientIP = NULL;
        ev_uint16_t clientPort = 0;
        evhttp_connection_get_peer(conn, &clientIP, &clientPort);
        httpReqInfo.peerIp = clientIP;
        httpReqInfo.peerPort = clientPort;
    }

    const void EventHttpServer::fillCmdType(struct evhttp_request *req, HttpReqInfo &httpReqInfo) {
        httpReqInfo.cmdType = evhttp_request_get_command(req);
    }

    const void EventHttpServer::fillRequestUri(struct evhttp_request *req, HttpReqInfo &httpReqInfo) {
        httpReqInfo.requestUri = evhttp_request_get_uri(req);

        struct evhttp_uri* httpUri = evhttp_uri_parse(httpReqInfo.requestUri.c_str());
        if (NULL != httpUri) {
            const char *uriPath = evhttp_uri_get_path(httpUri);
            httpReqInfo.uriPath = (NULL != uriPath) ? uriPath : "";

            const char *uriHost = evhttp_uri_get_host(httpUri);
            httpReqInfo.uriHost = (NULL != uriHost) ? uriHost : "";

            httpReqInfo.uriPort = evhttp_uri_get_port(httpUri);

            const char *userInfo = evhttp_uri_get_userinfo(httpUri);
            httpReqInfo.userInfo = (NULL != userInfo) ? userInfo : "";
        }
    }

    const void EventHttpServer::fillHttpReqInfo(struct evhttp_request *req, HttpReqInfo &httpReqInfo) {
        EventHttpServer::fillClientIpPort(req, httpReqInfo);
        EventHttpServer::fillCmdType(req, httpReqInfo);
        EventHttpServer::fillHeaders(req, httpReqInfo);
        EventHttpServer::fillRequestUri(req, httpReqInfo);
    }

    void EventHttpServer::handleSentinelUri(struct evhttp_request *req, const map<string, string> &httpBody) {
        ClientReqInfo clientReqInfo = EventHttpServer::httpBodyToClientReqInfo(httpBody);
        string rsp = LogicEntrance::instance().handleReq(clientReqInfo);
        EventHttpServer::doHttpRsp(req, rsp, HTTP_OK, "OK");
    }

    // http request handle entrance
    void EventHttpServer::httpReqEntrance(struct evhttp_request *req, void *arg) {
        HttpReqInfo httpReqInfo;
        EventHttpServer::fillHttpReqInfo(req, httpReqInfo);

        switch (httpReqInfo.cmdType) {
            case EVHTTP_REQ_GET:
                EventHttpServer::handleGetReq(req, httpReqInfo, arg);
                break;
            case EVHTTP_REQ_POST:
                EventHttpServer::handlePostReq(req, httpReqInfo, arg);
                break;
            default:
                evhttp_send_error(req, HTTP_BADREQUEST, 0);
                LOG(Warn, "not supported req type.", httpReqInfo.dump());
                return;
        }

        // router
        if (HTTP_URI_SENTINEL == httpReqInfo.uriPath) {
            LOG(Info, "find router.", HTTP_URI_SENTINEL);
            EventHttpServer::handleSentinelUri(req, httpReqInfo.body);
        } else {
            LOG(Info, "not find router , redirect to sentinel help");
            EventHttpServer::handleSentinelUri(req, httpReqInfo.body);
        }
    }

    void EventHttpServer::httpRouter(struct evhttp *http) {
        if (NULL != http) {
            // default router
            evhttp_set_gencb(http, EventHttpServer::httpReqEntrance, NULL);
            //evhttp_set_cb(http, "/help", EventHttpServer::helpEntrance, NULL);
            //evhttp_set_cb(http, "/sentinel", EventHttpServer::httpReqEntrance, NULL);

            LOG(Info, "set http router success");
        }
    }

    struct evhttp * EventHttpServer::createHttpServer(struct event_base *base) {
        if (NULL == base) {
            LOG(Error, "createHttpServer failed due to NULL event_base", \
                    evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
            return NULL;
        }


        // Create a new evhttp object to handle requests.
        struct evhttp *http = evhttp_new(base);
        if (!http) {
            LOG(Error, "evhttp_new failed", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
            return NULL;
        }

        EventHttpServer::httpRouter(http);

        struct evhttp_bound_socket *handle = evhttp_bind_socket_with_handle(http, SERVER_LISTEN_IP, HTTP_LISTEN_PORT);
        if (!handle) {
            LOG(Error, "couldn't bind to port ", HTTP_LISTEN_PORT, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
            return NULL;
        }

        LOG(Info, "http server listening on ", SERVER_LISTEN_IP, ":", HTTP_LISTEN_PORT);
        return http;
    }








}


