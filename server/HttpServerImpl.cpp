/**
 * Project Untitled
 */

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <algorithm>
#include "HttpServerImpl.h"
#include "rmtool.h"
#include "glog/logging.h"
#include "type.h"

/**
 * HttpServerImpl implementation
 */

using namespace std;

const string ALLOWIP = "allowip";
const char * HTTP_HEAD = "HTTP/1.0 200 OK\r\n";
const char * SERVER_STRING = "Server: omniWallethttpd/0.1.0\r\n";
const char * CONTENT_TYPE = "Content-Type: text/plain\r\n";

const int QUEUE_MAX_COUNT = 5;
const int RECEIVE_BUFF_SIZE = 526;
//const int SEND_BUFF_SIZE = 526;

HttpServerImpl::HttpServerImpl()
{

}

HttpServerImpl::~HttpServerImpl() 
{

}

int HttpServerImpl::init() 
{
    //    LOG(INFO) << __func__ << endl;
    struct sockaddr_in server_addr;

    string ip_file = PROOT_PATH;
    ip_file += IP_PATH;

    if(-1 == RmTool::readConfigFile(ip_file, ALLOWIP, ip_list))
    {
        LOG(ERROR) << "read configure file error!";
        exit(-1);
    }

    //创建socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        LOG(ERROR) << "socket error!";
        exit(-1);
    }
    memset(&server_addr, 0, sizeof(server_addr));

    //设置端口，IP，和TCP/IP协议族
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(HTTP_PORT) ;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;

    //套接字关闭后, 套接字状态TIME_WAIT约保留2到4分钟, 为了不bind失败，设置允许重用
    int on = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    //绑定
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))  < 0)
    {
        LOG(ERROR) << "bind error!";

        close(server_fd);
        server_fd = -1;
        exit(-1) ;
    }

    //监听
    if (listen(server_fd, QUEUE_MAX_COUNT) < 0)
    {
        LOG(ERROR) << "listen error!";
        close(server_fd);
        server_fd = -1;
        exit(-1) ;
    }

    LOG(INFO) << "http server running on port :" << HTTP_PORT;

    return 0;
}

int HttpServerImpl::start() 
{
    int client_fd = -1;
    struct sockaddr_in client_addr;

    socklen_t client_addr_len = sizeof(client_addr);

    char recv_buf[RECEIVE_BUFF_SIZE];
    memset(recv_buf, 0, RECEIVE_BUFF_SIZE);

    const char *send_buf = "ok ,we get it";

    while (1)
    {
        //调用accept函数阻塞程序，直到接收到客户端的请求
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len) ;
        if (client_fd < 0)
        {
            LOG(ERROR) << "accept error!";
            continue;
        }

        string client_address = inet_ntoa(client_addr.sin_addr);
        LOG(INFO) << "accept a client ! ip:" << client_address.c_str();

//        std::vector<std::string>::iterator iter = find(ip_list.begin(), ip_list.end(), client_address);
        auto iter = find(ip_list.begin(), ip_list.end(), client_address);
        if(iter == ip_list.end())
        {
            LOG(ERROR) << "!!!!Unknow client:" << client_address.c_str() << ", close the connector!";
            close(client_fd);
            continue;
        }

        // 接收客户端的请求 
        if(0 >= recv(client_fd, recv_buf, RECEIVE_BUFF_SIZE, 0))
        {
            LOG(ERROR) << "receive error! Maybe the connect is off!";
            close(client_fd);
            continue;
        }
        LOG(INFO) << "####recv from client :" << recv_buf;
#if 0
        //调用钱包RPC方法，获取信息
        if(-1 == handleRpcRequest(recv_buf, send_buf))
        {
            LOG(ERROR) << "get Wallet RPC ERROR!";
            close(client_fd);
            continue;
        }

        if(nullptr == send_buf)
        {
            LOG(ERROR) << "receive Nothing from walletRPC!";
            close(client_fd);
            continue;
        }

        //LOG(INFO) << "####### receive from wallet:[" << send_buf<< "]" << endl;;
#endif
        // 发送响应给客户端
        sendToClient(client_fd, send_buf, strlen(send_buf) + 1);
//        LOG(INFO) << __LINE__ << send_buf;
        memset(recv_buf, 0, RECEIVE_BUFF_SIZE) ;
//        free(send_buf);
//        send_buf = nullptr;

        // 关闭客户端套接字
        close(client_fd) ;
    }

    return 0;
}


int HttpServerImpl::sendToClient(int& client_fd, const char *send_buf, int buf_size)
{
    char sendBuff[buf_size];
    memset(sendBuff, 0, buf_size);

    send(client_fd, HTTP_HEAD, strlen(HTTP_HEAD), 0);
    send(client_fd, SERVER_STRING, strlen(SERVER_STRING),0);
    send(client_fd, CONTENT_TYPE, strlen(CONTENT_TYPE), 0);
    send(client_fd, "\r\n", strlen("\r\n"), 0);

    memcpy(sendBuff, send_buf, strlen(send_buf));
    LOG(INFO) << "####### send to client:[" << sendBuff<< "]" << endl;
    send(client_fd, sendBuff, buf_size - 1, 0);

    return 0;
}

int HttpServerImpl::handle() 
{
    //todo
    return 0;
}
