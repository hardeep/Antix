#ifndef CLIENT_H
#define CLIENT_H

#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include "TcpConnection.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <errno.h>

#ifndef DEBUG
#define DEBUG true
#endif

namespace Network
{

typedef std::map<int, TcpConnection*> TcpMap;

class Client
{
	
public:
	Client();
	int init();
	int initConnection(const char * host, const char * port);
	//virtual ~Client();
	virtual int handler(int fd) = 0;
	int start();
	
private:
	int handle_epoll(int epfd, int op, int fd, epoll_event* event);
	int addHandler(int fd, unsigned int events, TcpConnection * connection);
	int setnonblock(int fd); //magic!!
	int m_epfd; //more epic magic!! :)
	
protected:
	TcpMap m_serverList;
};
}

#endif
