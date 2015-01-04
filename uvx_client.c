#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>

#include "uvx.h"
#include "utils/automem.h"
#include "utils/linkhash.h"

// Author: Liigo <com.liigo@gmail.com>.

typedef union uvx_sockaddr_4_6_s{
    struct sockaddr_in  in4;
    struct sockaddr_in6 in6;
} uvx_sockaddr_4_6_t;

//! 修改此结构体时注意同步修改uvx_client_t.privates!
typedef struct uvx_client_private_s {
    uv_connect_t conn;              // sizeof(uv_connect_t) == 64
    uvx_sockaddr_4_6_t server_addr; // sizeof(uvx_sockaddr_4_6_t) == 28
    uv_timer_t heartbeat_timer;     // sizeof(uv_timer_t) == 120
    unsigned int heartbeat_index;
    int connection_closed;
} uvx_client_private_t;

#define UVX__C_PRIVATE(x)  ((uvx_client_private_t*)(&(x)->privates))

uvx_client_config_t uvx_client_default_config(uvx_client_t* xclient) {
    uvx_client_config_t config = { 0 };
    snprintf(config.name, sizeof(config.name), "xclient-%p", xclient);
    config.auto_connect = 1;
    config.heartbeat_interval_seconds = 60.0;
    config.log_out = stdout;
    config.log_err = stderr;
    return config;
}

static int uvx__client_reconnect(uvx_client_t* xclient);

static void uvx__on_heartbeat_timer(uv_timer_t* handle) {
    uvx_client_t* xclient = (uvx_client_t*) handle->data;
    assert(xclient);

    if(xclient->uvserver) {
        unsigned int index = UVX__C_PRIVATE(xclient)->heartbeat_index++;
        if(xclient->config.log_out)
            fprintf(xclient->config.log_out, "[uvx-client] %s on heartbeat (index %u)\n", xclient->config.name, index);
        if(xclient->config.on_heartbeat)
            xclient->config.on_heartbeat(xclient, index);
    } else {
        if(UVX__C_PRIVATE(xclient)->connection_closed)
            uvx__client_reconnect(xclient); // auto reconnect
    }
}

int uvx_client_connect(uvx_client_t* xclient, uv_loop_t* loop, const char* ip, int port, uvx_client_config_t config) {
	assert(xclient && loop && ip);
	xclient->uvloop = loop;
    xclient->uvserver = NULL;
    UVX__C_PRIVATE(xclient)->connection_closed = 0;
    memcpy(&xclient->config, &config, sizeof(uvx_client_config_t));
    if(strchr(ip, ':'))
        uv_ip6_addr(ip, port, (struct sockaddr_in6*) &UVX__C_PRIVATE(xclient)->server_addr);
    else
        uv_ip4_addr(ip, port, (struct sockaddr_in*) &UVX__C_PRIVATE(xclient)->server_addr);

	int ret = uvx__client_reconnect(xclient);

    int timeout = config.heartbeat_interval_seconds * 1000; // in milliseconds
    UVX__C_PRIVATE(xclient)->heartbeat_index = 0;
    UVX__C_PRIVATE(xclient)->heartbeat_timer.data = xclient;
	uv_timer_init(loop, &UVX__C_PRIVATE(xclient)->heartbeat_timer);
	uv_timer_start(&UVX__C_PRIVATE(xclient)->heartbeat_timer, uvx__on_heartbeat_timer, timeout, timeout);

	return ret;
}

static void _uv_on_connect(uv_connect_t* conn, int status);

static int uvx__client_reconnect(uvx_client_t* xclient) {
    xclient->uvserver = NULL;
	UVX__C_PRIVATE(xclient)->connection_closed = 0;
    UVX__C_PRIVATE(xclient)->conn.data = xclient;
	uv_tcp_init(xclient->uvloop, &xclient->uvclient);
    int ret = uv_tcp_connect(&UVX__C_PRIVATE(xclient)->conn, &xclient->uvclient,
                             (const struct sockaddr*) &UVX__C_PRIVATE(xclient)->server_addr, _uv_on_connect);
    if(ret >= 0 && xclient->config.log_out)
        fprintf(xclient->config.log_out, "[uvx-client] %s connect to server ...\n", xclient->config.name);
    if(ret < 0 && xclient->config.log_err)
        fprintf(xclient->config.log_err, "\n!!! [uvx-client] %s connect failed: %s\n", xclient->config.name, uv_strerror(ret));
    return (ret >= 0 ? 1 : 0);
}

static void uvx__after_close_client(uv_handle_t* handle) {
    uvx_client_t* xclient = (uvx_client_t*) handle->data;
    assert(handle->data);
    if(xclient->config.on_conn_close)
        xclient->config.on_conn_close(xclient);
    xclient->uvserver = NULL;
    UVX__C_PRIVATE(xclient)->connection_closed = 1;
}

static void _uvx_client_close(uvx_client_t* xclient) {
    if(xclient->config.log_out)
        fprintf(xclient->config.log_out, "[uvx-client] %s on close\n", xclient->config.name);
    if(xclient->config.on_conn_closing)
        xclient->config.on_conn_closing(xclient);
    uv_close((uv_handle_t*) &xclient->uvclient, uvx__after_close_client);

	// heartbeat_timer is reused to re-connect on next uvx__on_heartbeat_timer(), do not stop it.
	// uv_timer_stop(&_context.heartbeat_timer);
	// uv_close((uv_handle_t*)&_context.heartbeat_timer, NULL);
}

static void uvx__on_client_read(uv_stream_t* uvserver, ssize_t nread, const uv_buf_t* buf) {
    uvx_client_t* xclient = (uvx_client_t*) uvserver->data;
    assert(xclient);

	if(nread > 0) {
        assert(xclient->uvserver == (uv_tcp_t*)uvserver);
        if(xclient->config.on_recv)
            xclient->config.on_recv(xclient, buf->base, nread);
	} else if(nread < 0) {
		if(xclient->config.log_err)
            fprintf(xclient->config.log_err, "\n!!! [uvx-client] %s on read error: %s\n", xclient->config.name, uv_strerror(nread));
		_uvx_client_close(xclient); // will try reconnect on next uvx__on_heartbeat_timer()
	}
    free(buf->base);
}

// defines in uvx.c
void uvx__on_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

static void _uv_on_connect(uv_connect_t* conn, int status) {
    uvx_client_t* xclient = (uvx_client_t*) conn->data;
    xclient->uvclient.data = xclient;

	if(status == 0) {
		if(xclient->config.log_out)
            fprintf(xclient->config.log_out, "[uvx-client] %s connect to server ok\n", xclient->config.name);
		assert(conn->handle == (uv_stream_t*) &xclient->uvclient);
		xclient->uvserver = (uv_tcp_t*) conn->handle;
        if(xclient->config.on_conn_ok)
            xclient->config.on_conn_ok(xclient);
		uv_read_start(conn->handle, uvx__on_alloc_buf, uvx__on_client_read);
	} else {
		xclient->uvserver = NULL;
		if(xclient->config.log_err)
            fprintf(xclient->config.log_err, "\n!!! [uvx-client] %s connect to server failed: %s\n", xclient->config.name, uv_strerror(status));
        if(xclient->config.on_conn_fail)
            xclient->config.on_conn_fail(xclient);
		_uvx_client_close(xclient); // will try reconnect on next on_heartbeat
	}
}
