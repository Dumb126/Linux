#include "net.h"
#define MAX_EVENTS 10000

/* 设置阻塞方式 */
int setblock(int sock)
{
      int ret = fcntl(sock, F_SETFL, 0);
      if (0 > ret)
      {
            hand_error("setblock error");
      }
      return 0;
      
}

/* 设置非阻塞方式 */
int setnoblock(int sock)
{
      int ret = fcntl(sock, F_SETFL, O_NONBLOCK);
      if (0 > ret)
      {
            hand_error("setnoblock error");
      }
      return 0;
}

int main(int argc, char const *argv[])
{
      signal(SIGPIPE, SIG_IGN);
      int listenfd;

      listenfd = socket(AF_INET, SOCK_STREAM, 0);
      if (0 > listenfd)
      {
            hand_error("socket_create error");
      }

      setnoblock(listenfd);

      int on = 1;
      if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) < 0))
            hand_error("setsockopt error");

      struct sockaddr_in my_addr;
      memset(&my_addr, 0, sizeof(my_addr));
      my_addr.sin_family = AF_INET;
      my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

      if (bind(listenfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
            hand_error("bind error");
      
      int lisId = listen(listenfd, SOMAXCONN);
      if(0 > lisId)
            hand_error("listen error");
      
      struct sockaddr_in peer_addr;
      socklen_t peerlen;

      vector<int> clients;
      int count = 0;
      int cli_sock = 0;
      int epfd = 0;
      int ret_events;   //epoll_wait的返回值

      struct epoll_event ev_remove, ev, events[MAX_EVENTS];
      ev.events = EPOLLET | EPOLLIN;
      ev.data.fd = listenfd;

      epfd = epoll_create(MAX_EVENTS);
      if(-1 > epfd)
            hand_error("epoll_create error");
      
      int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
      if(0 > ret)
            hand_error("epoll_ctl error");
      
      while (1)
      {
            ret_events = epoll_wait(epfd, events, MAX_EVENTS, -1);
            if (-1 == ret_events)
            {
                  cout<<"ret_events = "<<ret_events<<endl;
                  hand_error("epoll_wait error");
            }

            if (0 == ret_events)
            {
                  cout<<"ret_events = "<<ret_events<<endl;
                  continue;
            }

            for (int i = 0; i < ret_events; i++)
            {
                  cout<<"num = "<<i<<endl;
                  cout<<"events[num].data.fd = "<<events[i].data.fd<<endl;
                  if (listenfd == events[i].data.fd)
                  {
                        cout<<"listen success and listenfd = "<<listenfd<<endl;
                        cli_sock = accept(listenfd, (struct sockaddr *)&peer_addr, &peerlen);
                        if(0 > cli_sock)
                              hand_error("accept error");
                        cout<<"count = "<<count++;
                        printf("ip=%s, port=%d\n", inet_ntoa(peer_addr.sin_addr), peer_addr.sin_port);
                        clients.push_back(cli_sock);
                        setnoblock(cli_sock);
                        ev.events = EPOLLET | EPOLLIN;
                        ev.data.fd = cli_sock;
                        if(epoll_ctl(epfd, EPOLL_CTL_ADD, cli_sock, &ev) < 0)
                              hand_error("epoll_ctl error");
                  }
                  else if(EPOLLIN & events[i].data.fd)
                  {
                        cli_sock = events[i].data.fd;
                        if(0 > cli_sock)
                              hand_error("cli_sock error");
                        
                        char recvbuf[1024];
                        memset(recvbuf, 0, sizeof(recvbuf));
                        int num = read(cli_sock, recvbuf, sizeof(recvbuf));
                        if(-1 == num)
                              hand_error("read error");
                        if (0 == num)
                        {
                              cout<<"client have exit!"<<endl;
                              close(cli_sock);
                              ev_remove = events[num];
                              epoll_ctl(epfd, EPOLL_CTL_DEL, cli_sock,&ev_remove);
                              clients.erase(remove(clients.begin(), clients.end(), cli_sock), clients.end());
                        }
                        fputs(recvbuf, stdout);
                        write(cli_sock, recvbuf, strlen(recvbuf));        
                  }
                  
            }
            
      }
      

      return 0;
}
 








