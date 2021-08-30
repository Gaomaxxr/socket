#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
#define BUFF_LEN 1024


/*
    server:
            socket-->bind-->recvfrom-->sendto-->close
*/

int main(int argc, char* argv[])
{
    int listen_fd, ret;
    struct sockaddr_in ser_addr;
    struct sockaddr_in client_addr;
    socklen_t clilen;

    // 1. UDP server创建UDP socket fd,设置socket为REUSEADDR和REUSEPORT、同时bind本地地址local_addr
    listen_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if(listen_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    int bReuseaddr = 1;
    int bReuseport = 1;
    if (0 > setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr, sizeof(bReuseaddr))) {
        printf("setsokopt 111 failed \r\n");
    }
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &bReuseport, sizeof(bReuseport));
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);  //端口号，需要网络序转换

    printf("s_addr:%d\n", ser_addr.sin_addr.s_addr);
    printf("sin_port:%d\n", ser_addr.sin_port);

    ret = bind(listen_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }

    // 2. 创建epoll fd，并将listen_fd放到epoll中 并监听其可读事件
    int epoll_fd;
    struct epoll_event ep_event;
    int in_fds;
    struct epoll_event in_events[10];


    epoll_fd = epoll_create(10);
    ep_event.events = EPOLLIN|EPOLLET;
    ep_event.data.fd = listen_fd;
    epoll_ctl(epoll_fd , EPOLL_CTL_ADD, listen_fd, &ep_event);
    printf("开始epoll_wait\n");

    while (1)
    {
        in_fds = epoll_wait(epoll_fd, in_events, 10, -1);

        for(int i = 0;i < in_fds; ++i)
        {
            if(in_events[i].data.fd == listen_fd) //如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
            {
                printf("新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接\n");
                char buf[BUFF_LEN];  //接收缓冲区
                socklen_t len;
                int new_fd;

                // 3. epoll_wait返回时，如果epoll_wait返回的事件fd是listen_fd，调用recvfrom接收client第一个UDP包并根据recvfrom返回的client地址, 创建一个新的socket(new_fd)与之对应，设置new_fd为REUSEADDR和REUSEPORT、同时bind本地地址local_addr，然后connect上recvfrom返回的client地址
                int count = recvfrom(listen_fd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &len);
                printf("client addr:%d port:%d msg:%s\n",client_addr.sin_addr.s_addr, client_addr.sin_port, buf);  //打印client发过来的信息
                memset(buf, 0, BUFF_LEN);
               //sprintf(buf, "I have recieved %d bytes data!\n", count);  //回复client
                //printf("server:%s\n",buf);  //打印自己发送的信息给
                //sendto(listen_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针

                new_fd = socket(AF_INET, SOCK_DGRAM, 0);
                int reuse = 1;
                if (0 > setsockopt(new_fd , SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse))) {
                    printf("setsockopt failed");
                }
                setsockopt(new_fd , SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
                if (0 > bind(new_fd , (struct sockaddr *) &ser_addr, sizeof(struct sockaddr))) {
                    printf("bind failed \r\n");
                }
                
                if (0 > connect(new_fd , (struct sockaddr *) &client_addr, sizeof(struct sockaddr))) {
                    printf("connect failed");
                }

                // 4. 将新创建的new_fd加入到epoll中并监听其可读等事件
                struct epoll_event client_ev;
                client_ev.events = EPOLLIN;
                client_ev.data.fd = new_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd , &client_ev);
                //send(new_fd, buf, BUFF_LEN, 0);
                printf("listen fd = %d, new fd = %d\r\n", listen_fd, new_fd);
            } else 
            {
                // 5. 当epoll_wait返回时，如果epoll_wait返回的事件fd是new_fd 那么就可以调用recvfrom来接收特定client的UDP包了
                char recvbuf[BUFF_LEN];  //接收缓冲区，1024字节
                int new_fd = in_events[i].data.fd;
                recv(new_fd , recvbuf, sizeof(recvbuf), 0);

                static int count = 0;

                if (10 > count++)
                    printf("new fd = %d\r\n", new_fd);
                #if 0
                socklen_t len;

                int count;
                struct sockaddr_in clent_addr;  //clent_addr用于记录发送方的地址信息

                memset(recvbuf, 0, BUFF_LEN);
                len = sizeof(clent_addr);
                count = recv(new_fd, recvbuf, BUFF_LEN, 0);  //recvfrom是拥塞函数，没有数据就一直拥塞
                if(count == -1)
                {
                    printf("recieve data fail!\n");
                    return 0;
                }
                printf("client:%s\n",recvbuf);  //打印client发过来的信息
                memset(recvbuf, 0, BUFF_LEN);
                sprintf(recvbuf, "I have recieved %d bytes data!\n", count);  //回复client
                printf("server:%s\n",recvbuf);  //打印自己发送的信息给
                send(new_fd, recvbuf, BUFF_LEN, 0);  //发送信息给client，注意使用了clent_addr结构体指针
                #endif
            }
        }
    }
    printf("end\n");
    close(listen_fd);
    return 0;
}