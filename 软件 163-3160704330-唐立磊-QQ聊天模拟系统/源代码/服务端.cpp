#include <stdio.h>
#include <netinet/in.h>   //for souockaddr_in
#include <sys/types.h>      
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>

#include <arpa/inet.h>

//for select
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <strings.h>   //for bzero
#include <string.h>

#define BUFF_SIZE 1024
#define backlog 7
#define ser_port 11277
#define CLI_NUM 3


int client_fds[CLI_NUM];

int main(int agrc,char **argv)
{
    int ser_souck_fd;
    int i;   
    char input_message[BUFF_SIZE];
    char resv_message[BUFF_SIZE];


    struct sockaddr_in ser_addr;
    ser_addr.sin_family= AF_INET;    //IPV4
    ser_addr.sin_port = htons(ser_port); 
    ser_addr.sin_addr.s_addr = INADDR_ANY;  //指定的是所有地址

    //creat socket
    if( (ser_souck_fd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
    {
        perror("creat failure");
        return -1;
    } 

    //bind soucket
    if(bind(ser_souck_fd, (const struct sockaddr *)&ser_addr,sizeof(ser_addr)) < 0)
    {
        perror("bind failure");
        return -1;
    }

    //listen
    if(listen(ser_souck_fd, backlog) < 0) 
    {
        perror("listen failure"); 
        return -1;
    }


    //fd_set
    fd_set ser_fdset;
    int max_fd=1;
    struct timeval mytime;
    printf("wait for client connnect!\n");

    while(1)
    {
        mytime.tv_sec=27;
        mytime.tv_usec=0;

        FD_ZERO(&ser_fdset);

        //add standard input
        FD_SET(0,&ser_fdset);
        if(max_fd < 0)
        {
            max_fd=0; 
        }

        //add serverce
        FD_SET(ser_souck_fd,&ser_fdset);
        if(max_fd < ser_souck_fd)
        {
            max_fd = ser_souck_fd;
        }

        //add client 
        for(i=0;i<CLI_NUM;i++)  //用数组定义多个客户端fd
        {
            if(client_fds[i]!=0) 
            {
                FD_SET(client_fds[i],&ser_fdset);
                if(max_fd < client_fds[i])
                {
                    max_fd = client_fds[i]; 
                }
            }
        }
		 //select多路复用
        int ret = select(max_fd + 1, &ser_fdset, NULL, NULL, &mytime);

        if(ret < 0)    
        {    
            perror("select failure\n");    
            continue;    
        }    

        else if(ret == 0)
        {
            printf("time out!");
            continue;
        }

        else
        {
            if(FD_ISSET(0,&ser_fdset)) //标准输入是否存在于ser_fdset集合中（也就是说，检测到输入时，做如下事情）
            {
                printf("send message to");
                bzero(input_message,BUFF_SIZE);
                fgets(input_message,BUFF_SIZE,stdin);

                for(i=0;i<CLI_NUM;i++)
                {
                    if(client_fds[i] != 0)
                    {
                        printf("client_fds[%d]=%d\n", i, client_fds[i]);
                        send(client_fds[i], input_message, BUFF_SIZE, 0);
                    }
                }

            }

            if(FD_ISSET(ser_souck_fd, &ser_fdset)) 
            {
                struct sockaddr_in client_address;
                socklen_t address_len;
                int client_sock_fd = accept(ser_souck_fd,(struct sockaddr *)&client_address, &address_len);
                if(client_sock_fd > 0)
                {
                    int flags=-1;
                    //一个客户端到来分配一个fd，CLI_NUM=3，则最多只能有三个客户端，超过4以后跳出for循环，flags重新被赋值为-1
                    for(i=0;i<CLI_NUM;i++)
                    {
                        if(client_fds[i] == 0)
                        {
                            flags=i; 
                            client_fds[i] = client_sock_fd;
                            break;
                        }
                    }


                    if (flags >= 0)
                    {
                        printf("new user client[%d] add sucessfully!\n",flags);

                    }

                    else //flags=-1
                    {   
                        char full_message[]="the client is full!can't join!\n";
                        bzero(input_message,BUFF_SIZE);
                        strncpy(input_message, full_message,100);
                        send(client_sock_fd, input_message, BUFF_SIZE, 0);

                    }
                }    
            }

        }

        //deal with the message

        for(i=0; i<CLI_NUM; i++)
        {
            if(client_fds[i] != 0)
            {
                if(FD_ISSET(client_fds[i],&ser_fdset))
                {
                    bzero(resv_message,BUFF_SIZE);
                    int byte_num=read(client_fds[i],resv_message,BUFF_SIZE);
                    if(byte_num > 0)
                    {
                        printf("message form client[%d]:%s\n", i, resv_message);
                    }
                    else if(byte_num < 0)
                    {
                        printf("rescessed error!");
                    }

                    //某个客户端退出
                    else  //cancel fdset and set fd=0
                    {
                        printf("clien[%d] exit!\n",i);
                        FD_CLR(client_fds[i], &ser_fdset);
                        client_fds[i] = 0;
                       // printf("clien[%d] exit!\n",i);
                        continue;  //这里如果用break的话一个客户端退出会造成服务器也退出。  
                    }
                }
            }
        }    
    }
    return 0;
}

