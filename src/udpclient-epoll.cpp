#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define SERVER_PORT 8888
#define BUFF_LEN 512


void udp_msg_sender(int fd, struct sockaddr* dst)
{

    socklen_t len;
    struct sockaddr_in src;
    for(int i =0; i < 100; i++)  //while(1)
    {
        char buf[BUFF_LEN] = "TEST UDP MSG!\n";
        len = sizeof(*dst);
        printf("client:%s\n",buf);  //打印自己发送的信息

        send(fd, buf, BUFF_LEN, 0);  // 用connect
        //sendto(fd, buf, BUFF_LEN, 0, dst, len);   // 不用connect


        memset(buf, 0, BUFF_LEN);
        //recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&src, &len);  //接收来自server的信息
        recv(fd, buf, BUFF_LEN, 0);  //接收来自server的信息
        printf("server:%s\n",buf);
        sleep(1);  //一秒发送一次消息
    }
}

/*
    client:
            socket-->sendto-->revcfrom-->close
*/

int main(int argc, char* argv[])
{
    int client_fd;
    struct sockaddr_in ser_addr;
    struct sockaddr_in client_addr;
    struct timeval tv;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    //ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络序转换
    ser_addr.sin_port = htons(SERVER_PORT);  //注意网络序转换

    // client_addr.sin_family = AF_INET;
    // client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络序转换
    // client_addr.sin_port = htons(SERVER_PORT);  //注意网络序转换

    //bind(client_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    connect(client_fd, (struct sockaddr* )&ser_addr, sizeof(struct sockaddr_in));

    // gettimeofday(&tv, NULL);
    // long time1 = tv.tv_sec * 1000000 + tv.tv_usec;
    udp_msg_sender(client_fd, (struct sockaddr*)&ser_addr);

    close(client_fd);

    return 0;
}