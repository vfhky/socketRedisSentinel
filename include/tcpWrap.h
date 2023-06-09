#ifndef __SCOKET_REDIS_SENTINEL_TCP_WRAP_H__
#define __SCOKET_REDIS_SENTINEL_TCP_WRAP_H__




#include "common.h"
#include "utils.h"


using namespace std;

namespace socketRedisSentinel {


    class TcpWrap
    {
    public:
        TcpWrap();


        bool connect(const string &ip, const uint16_t &port);

        void setSocketOpt(const int64_t &rcvSec, const int64_t &sendSec);

        bool send(const string &message);


        bool recv(string &message);

        void close();

        int getClientFd();


    private:


        int m_client_fd;
    };




}







#endif

