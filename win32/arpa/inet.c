#include <windows.h>
#include <ws2tcpip.h>

int inet_pton(int af, const char *src, void *dst)
{
	int address_length;
	union
	{
		struct sockaddr_storage sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} addrs;

	switch (af)
	{
		case AF_INET:
			address_length = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			address_length = sizeof(struct sockaddr_in6);
			break;
		default:
			return -1;
	}

	if (!WSAStringToAddress((LPSTR) src, af, NULL, &addrs.sa, &address_length))
	{
		switch (af)
		{
			case AF_INET:
				memcpy(dst, &addrs.sin.sin_addr, sizeof(struct in_addr));
				break;
			case AF_INET6:
				memcpy(dst, &addrs.sin6.sin6_addr, sizeof(struct in6_addr));
				break;
		}

		return 1;
	}

	return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
	int address_length;
	DWORD string_length = size;
	union
	{
		struct sockaddr_storage sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} addrs;

	memset(&addrs, 0, sizeof(addrs));

	switch (af)
	{
		case AF_INET:
			address_length = sizeof(struct sockaddr_in);
			addrs.sin.sin_family = af;
			memcpy(&addrs.sin.sin_addr, src, sizeof(struct in_addr));
			break;
		case AF_INET6:
			address_length = sizeof(struct sockaddr_in6);
			addrs.sin6.sin6_family = af;
			memcpy(&addrs.sin6.sin6_addr, src, sizeof(struct in6_addr));
			break;
		default:
			return NULL;
	}

	if (!WSAAddressToString(&addrs.sa, address_length, NULL, dst, &string_length))
		return dst;

	return NULL;
}

