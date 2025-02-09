#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>


int serverfd = 0, clientfd, maxfd, id, clients[16 * 4096], port;
char msg[97*4096], buff[96*4096];
fd_set current, rd, wr;
ssize_t recv_size;
struct sockaddr_in cli;
socklen_t len = sizeof(cli);


void err(const char *msg)
{
	if(serverfd > 2) close(serverfd);
	write(2, msg, strlen(msg));
	exit(1);
}

void ok(int s)
{
	if(s < 0)
		err("Fatal error\n");
}

void send_to_all(int exfd)
{
	for(int fd = 0; fd < maxfd; ++fd)
	{
		if(fd != exfd && FD_ISSET(fd, &wr))
			ok(send(fd, msg, strlen(msg), 0));
	}
	bzero(msg, sizeof(msg));
}

int main(int ac, char **av)
{
	if(ac != 2) err("Wrong number of arguments\n");
	if((port = atoi(av[1])) < 0) err("Port");
	struct sockaddr_in serveraddr = {AF_INET, htons(port), {htonl(INADDR_LOOPBACK)}, {0}};
	ok(serverfd = socket(AF_INET, SOCK_STREAM, 0));
	ok(bind(serverfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)));
	ok(listen(serverfd, 10));
	FD_ZERO(&current);
	FD_SET(serverfd, &current);
	maxfd = serverfd;

	while(1)
	{
		rd = wr = current;
		if(select(maxfd + 1, &rd, &wr, 0, 0) < 0) continue;

		if(FD_ISSET(serverfd, &rd))
		{
			ok(clientfd = accept(serverfd, (struct sockaddr *)&cli, &len));
			maxfd = clientfd > maxfd ? clientfd : maxfd;
			sprintf(msg, "server: client %d just arrived\n", clients[clientfd] = id++);
			FD_SET(clientfd, &current);
			send_to_all(clientfd);
			continue;
		}

		for(int fd = 0; fd <= maxfd; ++fd)
		{
			if(FD_ISSET(fd, &rd))
			{
				bzero(buff, sizeof(buff));
				recv_size = 1;
				int b_len = 0;
				while(recv_size == 1 && buff[b_len -1] != '\n') recv_size = recv(fd, buff + b_len++, 1, 0);
				if(recv_size <= 0)
				{
					sprintf(msg, "server: client %d just left\n", clients[fd]);
					FD_CLR(fd, &current);
					close(fd);
				}
				else
					sprintf(msg, "client %d : %s",  clients[fd], buff);
				send_to_all(fd);
			}
		}
	}
}
