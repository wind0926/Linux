#include<stdio.h>    
#include<stdlib.h>    
#include<netinet/in.h>    
#include<sys/socket.h>    
#include<arpa/inet.h>    
#include<string.h>    
#include<unistd.h>    

#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024    

int main(int argc, const char * argv[])    
{   
    int i,n;
    int connfd,sockfd;
    struct epoll_event ev,events[20]; //ev用于注册事件,数组用于回传要处理的事件
    int epfd=epoll_create(256);//创建一个epoll的句柄，其中256为你epoll所支持的最大句柄数

    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;    

    server_addr.sin_family = AF_INET;    
    server_addr.sin_port = htons(11277);    
    server_addr.sin_addr.s_addr =INADDR_ANY;    
    bzero(&(server_addr.sin_zero), 8);    

    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);  

    ev.data.fd=server_sock_fd;//设置与要处理的事件相关的文件描述符
    ev.events=EPOLLIN|EPOLLET;//设置要处理的事件类型
    epoll_ctl(epfd,EPOLL_CTL_ADD,server_sock_fd,&ev);//注册epoll事件

    if(server_sock_fd == -1)    
    {    
        perror("socket error");    
        return 1;    
    }    

    char recv_msg[BUFFER_SIZE];    
    char input_msg[BUFFER_SIZE];    

    if(connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)    
    {    
        for(;;)
        {
            int nfds=epoll_wait(epfd,events,20,500);//等待epoll事件的发生
            for(i=0;i<nfds;++i)
            {    
                if(events[i].events&EPOLLOUT) //有数据发送，写socket
                {
                    bzero(input_msg, BUFFER_SIZE);    
                    fgets(input_msg, BUFFER_SIZE, stdin);    

                    sockfd = events[i].data.fd;
                    write(sockfd, recv_msg, n);

                    ev.data.fd=sockfd;
                    ev.events=EPOLLIN|EPOLLET;
                    epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev);
                }   

                else if(events[i].events&EPOLLIN)//有数据到来，读socket
                {
                    bzero(recv_msg, BUFFER_SIZE);
                    if((n = read(server_sock_fd, recv_msg, BUFFER_SIZE)) <0 )
                    {
                        printf("read error!");
                    }

                    ev.data.fd=server_sock_fd;
                    ev.events=EPOLLOUT|EPOLLET;
                    printf("%s\n",recv_msg);
                }

            }        
        }
    }    
    return 0;    
}   
