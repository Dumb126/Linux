#include <stdio.h>
#ifndef _NET_H
#define _NET_H

#include <iostream>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

using namespace std;

#define hand_error(msg) do{perror(msg); exit(EXIT_FAILURE);}while(0)
#endif