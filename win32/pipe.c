#include <netinet/in.h>
#include <arpa/inet.h>
#include <event2/util.h>

int pipe(int fds[2])
{
	struct sockaddr_in localhost, listen_addr;
	int cfd, lfd, afd;
	socklen_t len;

	if (inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr) != 1)
		return -1;

	localhost.sin_family = AF_INET;
	localhost.sin_port = 0;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (cfd == -1 || lfd == -1)
	{
		evutil_closesocket(lfd);
		return -1;
	}

	if (bind(lfd, (struct sockaddr *) &localhost, sizeof(localhost)) == -1)
	{
		evutil_closesocket(lfd);
		return -1;
	}

	if (listen(lfd, 1) == -1)
	{
		evutil_closesocket(lfd);
		return -1;
	}

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if (cfd == -1)
	{
		evutil_closesocket(lfd);
		return -1;
	}

	len = sizeof(listen_addr);
	if (getsockname(lfd, (struct sockaddr *) &listen_addr, &len) == -1)
	{
		evutil_closesocket(lfd);
		evutil_closesocket(cfd);
		return -1;
	}

	if (connect(cfd, (struct sockaddr *) &listen_addr, sizeof(listen_addr)))
	{
		evutil_closesocket(lfd);
		evutil_closesocket(cfd);
		return -1;
	}

	afd = accept(lfd, NULL, NULL);

	evutil_closesocket(lfd);

	if (afd == -1)
	{
		evutil_closesocket(cfd);
		return -1;
	}

	fds[0] = cfd;
	fds[1] = afd;

	return 0;
}

