#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <uv.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Author: Liigo <com.liigo@gmail.com>.

const char* uvx_get_ip_port(const struct sockaddr* addr, char* ipbuf, int buflen, int* port) {
    switch (addr->sa_family) {
    case AF_INET: {
            const struct sockaddr_in* addrin = (const struct sockaddr_in*) addr;
            if(ipbuf) inet_ntop(AF_INET, &(addrin->sin_addr), ipbuf, buflen); // or use portable uv_inet_ntop()
            if(port)  *port = (int) ntohs(addrin->sin_port);
            break;
        }
    case AF_INET6: {
            const struct sockaddr_in6* addrin = (const struct sockaddr_in6*) addr;
            if(ipbuf) inet_ntop(AF_INET6, &(addrin->sin6_addr), ipbuf, buflen); // or use portable uv_inet_ntop()
            if(port)  *port = (int) ntohs(addrin->sin6_port);
            break;
        }
    default:
        if(port) *port = 0;
        return NULL;
    }
}

int uvx_get_raw_ip_port(const struct sockaddr* addr, unsigned char* ipbuf, int* port) {
    switch (addr->sa_family) {
    case AF_INET: {
            const struct sockaddr_in* addrin = (const struct sockaddr_in*) addr;
            if(ipbuf) memcpy(ipbuf, &addrin->sin_addr, sizeof(addrin->sin_addr));
            if(port)  *port = (int) ntohs(addrin->sin_port);
            return sizeof(addrin->sin_addr); // 4
            break;
        }
    case AF_INET6: {
            const struct sockaddr_in6* addrin = (const struct sockaddr_in6*) addr;
            if(ipbuf) memcpy(ipbuf, &addrin->sin6_addr, sizeof(addrin->sin6_addr));
            if(port)  *port = (int) ntohs(addrin->sin6_port);
            return sizeof(addrin->sin6_addr); // 16
            break;
        }
    default:
        if(port) *port = 0;
        return 0;
    }
}

const char* uvx_get_tcp_ip_port(uv_tcp_t* uvclient, char* ipbuf, int buflen, int* port) {
	struct sockaddr addr;
	int len = sizeof(addr);
	int r = uv_tcp_getpeername(uvclient, &addr, &len);
	if(r == 0) {
		return uvx_get_ip_port(&addr, ipbuf, buflen, port);
	} else {
        printf("\n!!! [uvx] get client ip fails: %s\n", uv_strerror(r));
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// internal functions

void uvx__on_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = (char*) malloc(suggested_size);
	buf->len  = buf->base ? suggested_size : 0;
}