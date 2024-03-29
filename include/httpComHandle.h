/**
 * Copyright (c) 2023-07, typecodes.com (vfhky@typecodes.com)
 *
 * All rights reserved.
 *
 * Tools for handling the request of http/https.
 */


#ifndef __SOCKET_REDIS_SENTINEL_HTTP_COM_HANDLE_H__
#define __SOCKET_REDIS_SENTINEL_HTTP_COM_HANDLE_H__




#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/keyvalq_struct.h>
#include <sys/queue.h>



#include "utils.h"
#include "clientReqInfo.h"





namespace socketRedisSentinel {


    struct HttpReqInfo {
		// enum : evhttp_cmd_type
        int32_t cmdType;

        std::string requestUri;

        std::string uriPath;

        std::string uriHost;

        int32_t uriPort;

        std::string userInfo;

        std::string peerIp;
        ev_uint16_t peerPort;

        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> body;


        HttpReqInfo() {
            cmdType = 0;
            requestUri.clear();
            uriPath.clear();
            uriHost.clear();
            uriPort = -9999;
            userInfo.clear();
            peerIp.clear();
            peerPort = 0;
            headers.clear();
            body.clear();
        }

        void clear() {
            cmdType = 0;
            requestUri.clear();
            uriPath.clear();
            uriHost.clear();
            uriPort = -9999;
            userInfo.clear();
            peerIp.clear();
            peerPort = 0;
            headers.clear();
            body.clear();
        }

        const std::string dump() const {
            std::stringstream ss;
            ss << "HttpReqInfo - {"
                << "[cmdType:" << cmdType << "]"
                << "[requestUri:" << requestUri << "]"
                << "[uriPath:" << uriPath << "]"
                << "[uriHost:" << uriHost << "]"
                << "[uriPort:" << uriPort << "]"
                << "[userInfo:" << userInfo << "]"
                << "[peerIp:" << peerIp << "]"
                << "[peerPort:" << peerPort << "]"
                << "[headers:" << Utils::printMap(headers) << "]"
                << "[body:" << Utils::printMap(body) << "]"
                << "}";

            return ss.str();
        }
    };


    // define http router uri
    static const std::string HTTP_URI_HOME = "/";
    static const std::string HTTP_URI_SENTINEL = "/sentinel";




    class HttpComHandle
    {
    public:

        static HttpComHandle& instance();

        static void httpReqEntrance(struct evhttp_request *req, void *arg);

        static void httpRouter(struct evhttp *http);





    private:

        static void handleGetReq(struct evhttp_request *req, HttpReqInfo &httpReqInfo, void *arg);
        static void handlePostReq(struct evhttp_request *req, HttpReqInfo &httpReqInfo, void *arg);

        static const void fillHttpReqInfo(struct evhttp_request *req, HttpReqInfo &httpReqInfo);
        static const void fillHeaders(struct evhttp_request *req, HttpReqInfo &httpReqInfo);
        static const void fillClientIpPort(struct evhttp_request *req, HttpReqInfo &httpReqInfo);
        static const void fillCmdType(struct evhttp_request *req, HttpReqInfo &httpReqInfo);
        static const void fillRequestUri(struct evhttp_request *req, HttpReqInfo &httpReqInfo);
        static const bool fillGetParams(HttpReqInfo &httpReqInfo);
        static const bool fillPostParams(struct evhttp_request *req, HttpReqInfo &httpReqInfo);
        static const std::map<std::string, std::string> parseFormData(const std::string& postData);
        static ClientReqInfo httpBodyToClientReqInfo(const std::map<std::string, std::string> &httpBody);




        static const map<string, string> dumpGetParams(struct evhttp_request *req);
        static const map<string, string> dumpPostParams(struct evhttp_request *req);

        // http response
        static void doHttpRsp(struct evhttp_request *req, const std::string &rspData,
                const int &rspCode, const std::string &rspReason);

        static void handleSentinelUri(struct evhttp_request *req, const map<string, string> &httpBody);










    private:











    };




}







#endif

