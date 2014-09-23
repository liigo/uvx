#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include "uvx.h"

// Author: Liigo <com.liigo@gmail.com>.

// defines in uvx.h
void uvx__on_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

static void uvx__on_udp_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags) {
    uvx_udp_t* xudp = (uvx_udp_t*) handle->data;
    // printf("on udp recv: size=%d \n", nread);
    if(nread > 0 && xudp->config.on_recv)
        xudp->config.on_recv(xudp, buf->base, nread, addr, flags);
    free(buf->base);
}

uvx_udp_config_t uvx_udp_default_config(uvx_udp_t* xudp) {
    uvx_udp_config_t config = { 0 };
    snprintf(config.name, sizeof(config.name), "xudp-%p", xudp);
    config.log_out = stdout;
    config.log_err = stderr;
    return config;
}

int uvx_udp_start(uvx_udp_t* xudp, uv_loop_t* loop, const char* ip, int port, uvx_udp_config_t config) {
    assert(xudp && loop);
	xudp->uvloop = loop;
    memcpy(&xudp->config, &config, sizeof(uvx_udp_config_t));

	// init udp
    uv_udp_init(loop, &xudp->uvudp);
    xudp->uvudp.data = xudp;

    if(ip) {
        int r;
        if(strchr(ip, ':')) {
            struct sockaddr_in6 addr;
            uv_ip6_addr(ip, port, &addr);
            r = uv_udp_bind(&xudp->uvudp, (const struct sockaddr*) &addr, 0);
        } else {
            struct sockaddr_in addr;
            uv_ip4_addr(ip, port, &addr);
            r = uv_udp_bind(&xudp->uvudp, (const struct sockaddr*) &addr, 0);
        }

        if(r >= 0 && config.log_out) {
            char timestr[32]; time_t t; time(&t);
            strftime(timestr, sizeof(timestr), "[%F %X]", localtime(&t));
            fprintf(config.log_out, "[uvx-udp] %s %s bind on %s:%d ...\n", timestr, xudp->config.name, ip, port);
        }
        if(r < 0) {
            if(config.log_err)
                fprintf(config.log_err, "\n!!! [uvx-udp] %s bind on %s:%d failed: %s\n", xudp->config.name, ip, port, uv_strerror(r));
            uv_close((uv_handle_t*) &xudp->uvudp, NULL);
            return 0;
        }
    } else {
        // if ip == NULL, default bind to local random port number (for UDP client)
        if(config.log_out)
            fprintf(config.log_out, "[uvx-udp] %s bind on local random port ...\n", xudp->config.name);
    }

    uv_udp_recv_start(&xudp->uvudp, uvx__on_alloc_buf, uvx__on_udp_recv);
    return 1;
}

static void uv_after_udp_send(uv_udp_send_t* req, int status) {
    free(req); // see uvx_udp_send_to_addr()
}

int uvx_udp_send_to_addr(uvx_udp_t* xudp, const struct sockaddr* addr, const void* data, unsigned int datalen) {
    uv_udp_send_t* req = (uv_udp_send_t*) malloc(sizeof(uv_udp_send_t) + datalen);
    uv_buf_t buf = uv_buf_init((char*)req + sizeof(uv_udp_send_t), datalen);
    memcpy(buf.base, data, datalen); // copy data to the end of req
    req->data = xudp;
    return (uv_udp_send(req, &xudp->uvudp, &buf, 1, addr, uv_after_udp_send) == 0 ? 1 : 0); 
}

int uvx_udp_send_to_ip(uvx_udp_t* xudp, const char* ip, int port, const void* data, unsigned int datalen) {
    assert(ip);
    if(strchr(ip, ':')) {
        struct sockaddr_in6 addr;
        uv_ip6_addr(ip, port, &addr);
        return uvx_udp_send_to_addr(xudp, (const struct sockaddr*) &addr, data, datalen);
    } else {
        struct sockaddr_in addr;
        uv_ip4_addr(ip, port, &addr);
        return uvx_udp_send_to_addr(xudp, (const struct sockaddr*) &addr, data, datalen);
    }
}

int uvx_udp_set_broadcast(uvx_udp_t* xudp, int on) {
    return (uv_udp_set_broadcast(&xudp->uvudp, on) == 0 ? 1 : 0);
}

int uvx_udp_shutdown(uvx_udp_t* xudp) {
    uv_udp_recv_stop(&xudp->uvudp);
    uv_close((uv_handle_t*) &xudp->uvudp, NULL);
    return 1;
}
