#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctype.h>

#define MAXLINE 8192
#define SERVER_PORT 8000
#define OPEN_MAX 5000

void perr_exit(const char *s)
{
      perror(s);
      exit(-1);
}

int main(int argc, char const *argv[])
{
      int i, listenfd, connfd, sockfd;
      int n, num = 0;
      ssize_t nready, efd, res;
      char buf[MAXLINE], str[INET_ADDRSTRLEN];
      socklen_t clilen;

      struct sockaddr_in cliaddr, servaddr;
      struct epoll_event tep, ep[OPEN_MAX];
      
      /* 1.创建socket */
      listenfd = socket(AF_INET, SOCK_STREAM, 0);
      if (0 > listenfd)
      {
            perror("Faild to create socket fd!");
            return -1;
      }

      /* 2.设置端口复用 */
      int opt = 1;
      setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //端口复用

      /* 结构体赋值 */
      bzero(&servaddr, sizeof(servaddr));
      servaddr.sin_family = AF_INET;
      servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
      servaddr.sin_port = htons(SERVER_PORT);

      /* 3.绑定端口和地址 */
      int ret = 0;
      ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
      if (0 != ret)
      {
            perror("Failed to bind the socket fd!");
            return ret;
      }

      /* 4.设置监听 */
      (void)listen(listenfd, 20);

      /* 创建epoll */
      efd = epoll_create(OPEN_MAX);
      if (0 > efd)
      {
            perr_exit("epoll_create error");
      }

      tep.events = EPOLLIN;
      tep.data.fd = listenfd;  //指定lfd的监听事件为‘读’
      res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);  //将lfd及对应的结构体设置到树上，efd可找到该树
      if (-1 == res)
      {
            perr_exit("epoll_ctl error.");
      }
      

      for(;;)
      {
            nready = epoll_wait(efd, ep, OPEN_MAX, -1);
            if (nready == -1)
            {
                  perr_exit("epoll_wait error.");
            }

            for(i = 0; i < nready; i++)
            {
                  if (!(ep[i].events & EPOLLIN))
                  {
                        /* 如果不是‘读’事件，继续循环 */
                        continue;
                  }

                  if (ep[i].data.fd == listenfd)   //判断满足事件的fd是不是lfd
                  {
                        clilen = sizeof(cliaddr);
                        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen); //接受链接

                        printf("received from %s at PORT %d\n",
                        inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                        ntohs(cliaddr.sin_port));
                        printf("cfd %d ----client %d\n", connfd, ++num);

                        tep.events = EPOLLIN;
                        tep.data.fd = connfd;
                        res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
                        if(res == -1)
                        {
                              perr_exit("epoll_ctl error.");
                        }
                  }
                  else    //不是lfd
                  {
                        sockfd = ep[i].data.fd;
                        n = read(sockfd, buf, MAXLINE);

                        /* 读到0， 说明客户端关闭链接 */
                        if (0 == n)
                        {
                              /* 将该文件描述符从epoll中删除，也就是从红黑树删除 */
                              res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                              if (-1 == res)
                              {
                                    perr_exit("epoll_ctl error.");
                              }
                              close(sockfd);
                              printf("Client [%d] closed connection\n", sockfd);
                        }
                        else if(0 > n)
                        {
                              /* error */
                              perror("read n < 0 error:");
                              res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                              close(sockfd);
                        }
                        else
                        {
                              /* 走到这里，就是实际读到了字节数 */
                              write(STDOUT_FILENO, buf, n);  //打印到标准输出

                              for(i = 0; i < n; i++)
                              {
                                    buf[i] = toupper(buf[i]);
                              }
                              write(sockfd, buf, n);
                        }
                  }
            }
      }
      
      close(listenfd);
      close(efd);
      return 0;
}
