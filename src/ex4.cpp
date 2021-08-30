#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/uio.h>  

#define NUM_PROCESS     10
#define SO_REUSEPORT    15
#define SERV_PORT       12345
#define MAXSIZE         1024

struct worker_arg {
        int sock_fd;
        int process_num;
};

void DEBUG(char *argv) {
      //TODO  
}
void client_echo(int sock, int process_num)
{
        int n;
        char buff[MAXSIZE];
        struct sockaddr clientfrom;
        socklen_t addrlen = sizeof(clientfrom);
        for(;;) {
                memset(buff, 0, MAXSIZE);
                n = recvfrom(sock, buff, MAXSIZE, 0, &clientfrom, &addrlen);
                printf("%d\n", process_num);
                n = sendto(sock, buff, n, 0, &clientfrom, addrlen);
        }
}

int create_udp_sock(const char *str_addr)
{
        int sockfd;
        int optval = 1;
        int fdval = 0;
        struct sockaddr_in servaddr, cliaddr;
        sockfd = socket(AF_INET, SOCK_DGRAM|SOCK_CLOEXEC, 0); /* create a socket */
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
       //这是关键，关键中的关键。如果没有这个，此代码在之前内核照样完美运行！完美？完美！
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(str_addr);
        servaddr.sin_port = htons(SERV_PORT);
        if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
                perror("bind error");
                exit(1);
        }
                perror("bind ");
        
        return sockfd;
}

void woker_process(int sock, int num)
{        
        int woker_sock = sock;//recv_fd(sock);
        if(woker_sock < 0){
        }
        client_echo(woker_sock, num);
    
}

pid_t create_work(void *arg)
{
        struct worker_arg *warg = (struct worker_arg*)arg;
        pid_t pid = fork();
        if (pid > 0) {
                return pid;
        } else if (pid == 0){
                woker_process(warg->sock_fd, warg->process_num);
                // 不可能运行到此
        } else {
                exit(1);
        }
        // 不可能运行到此
        return pid;
}

void schedule_process(int socks[2], char *str_addr)
{
        pid_t pids[NUM_PROCESS] = {0};
        int udps[NUM_PROCESS] = {0};
        int i = 0;
        for (i = 0; i < NUM_PROCESS; i++) {
                udps[i] = create_udp_sock(str_addr);
        }
        for (i = 0; i < NUM_PROCESS; i++) {
                struct worker_arg warg;
                warg.sock_fd = udps[i];
                warg.process_num = i;
                pid_t pid = create_work(&warg);
                pids[i] = pid;
        }
        while (1) {
                int stat;
                int i;
                pid_t pid = waitpid(-1, &stat, 0);
                for (i = 0; i < NUM_PROCESS; i++) {
                        if (pid == pids[i]) {
                                // 最最关键的，那就是”把特定的套接字传递到特定的进程中“
                                struct worker_arg warg;
                                warg.sock_fd = udps[i];
                                warg.process_num = i;
                                pid_t pid = create_work(&warg);
                                pids[i] = pid;
                                break;
                        }
                }
        }
}

int main(int argc, char** argv)
{
        int unix_socks[2];
        char *str_addr = argv[1];
        if(socketpair(AF_UNIX, SOCK_STREAM, 0, unix_socks) == -1){
                exit(1);
        }
        schedule_process(unix_socks, str_addr);
        return 0;
}
