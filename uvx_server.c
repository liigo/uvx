#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <time.h>
#include <assert.h>

#include "uvx.h"
#include "utils/automem.h"
#include "utils/linkhash.h"

// Author: Liigo <com.liigo@gmail.com>.

//! Note: modify this struct along with uvx_server_t.privates!
typedef struct uvx_server_private_s {
    uv_timer_t heartbeat_timer; // sizeof(uv_timer_t) == 120
    unsigned int heartbeat_index;
    struct lh_table* conns; // connections of clients, hash-table of uvx_server_conn_t*
} uvx_server_private_t;

#define _UVX_S_PRIVATE(x)  ((uvx_server_private_t*)(&(x)->privates))

static void uvx__on_connection(uv_stream_t* uvserver, int status);
static void _uv_disconnect_client(uv_stream_t* uvclient);
static void _uv_after_close_connection(uv_handle_t* handle);

uvx_server_config_t uvx_server_default_config(uvx_server_t* xserver) {
    uvx_server_config_t config = { 0 };
    snprintf(config.name, sizeof(config.name), "xserver-%p", xserver);
    config.conn_count = 1024;
    config.conn_backlog = 128;
    config.conn_extra_size = 0;
    config.conn_timeout_seconds = 180.0;
    config.heartbeat_interval_seconds = 60.0;
    config.log_out = stdout;
    config.log_err = stderr;
    return config;
}

static void _uvx_check_timeout_clients(uvx_server_t* xserver);

static void _uv_on_heartbeat_timer(uv_timer_t* handle) {
    uvx_server_t* xserver = (uvx_server_t*) handle->data;
    assert(xserver);
    unsigned int index = _UVX_S_PRIVATE(xserver)->heartbeat_index++;
    if(xserver->config.log_out)
        fprintf(xserver->config.log_out, "[uvx-server] %s on heartbeat (index %u)\n", xserver->config.name, index);
    if(xserver->config.on_heartbeat)
        xserver->config.on_heartbeat(xserver, index);
    // check and close timeout-ed connections
    if(xserver->config.conn_timeout_seconds > 0)
   	    _uvx_check_timeout_clients(xserver);
}

int uvx_server_start(uvx_server_t* xserver, uv_loop_t* loop, const char* ip, int port, uvx_server_config_t config) {
    assert(xserver && loop && ip);
	xserver->uvloop = loop;
    memcpy(&xserver->config, &config, sizeof(uvx_server_config_t));

	_UVX_S_PRIVATE(xserver)->conns = lh_kptr_table_new(config.conn_count, "clients connection table", NULL);

	// init and start timer
	int timeout = (int)(config.heartbeat_interval_seconds * 1000); // in milliseconds
	uv_timer_init(loop, &_UVX_S_PRIVATE(xserver)->heartbeat_timer);
    _UVX_S_PRIVATE(xserver)->heartbeat_timer.data = xserver;
    _UVX_S_PRIVATE(xserver)->heartbeat_index = 0;
	if(timeout > 0)
		uv_timer_start(&_UVX_S_PRIVATE(xserver)->heartbeat_timer, _uv_on_heartbeat_timer, timeout, timeout);

    // init tcp, bind and listen
    uv_tcp_init(loop, &xserver->uvserver);
    xserver->uvserver.data = xserver;
    if(strchr(ip, ':')) {
        struct sockaddr_in6 addr;
        uv_ip6_addr(ip, port, &addr);
        uv_tcp_bind(&xserver->uvserver, (const struct sockaddr*) &addr, 0);
    } else {
        struct sockaddr_in addr;
        uv_ip4_addr(ip, port, &addr);
        uv_tcp_bind(&xserver->uvserver, (const struct sockaddr*) &addr, 0);
    }

	int ret = uv_listen((uv_stream_t*)&xserver->uvserver, config.conn_backlog, uvx__on_connection);
    if(ret >= 0 && config.log_out) {
        char timestr[32]; time_t t; time(&t);
        strftime(timestr, sizeof(timestr), "[%F %X]", localtime(&t));
        fprintf(config.log_out, "[uvx-server] %s %s listening on %s:%d ...\n", timestr, xserver->config.name, ip, port);
    }
    if(ret < 0 && config.log_err)
        fprintf(config.log_err, "\n!!! [uvx-server] %s listen on %s:%d failed: %s\n", xserver->config.name, ip, port, uv_strerror(ret));

    return (ret >= 0);
}

int uvx_server_shutdown(uvx_server_t* xserver) {
	uv_timer_stop(&_UVX_S_PRIVATE(xserver)->heartbeat_timer);
	uv_close((uv_handle_t*)&_UVX_S_PRIVATE(xserver)->heartbeat_timer, NULL);
	lh_table_free(_UVX_S_PRIVATE(xserver)->conns);
	uv_close((uv_handle_t*)&xserver->uvserver, NULL);
    return 0;
}

void uvx_server_conn_ref(uvx_server_conn_t* conn, int ref) {
    assert(ref == 1 || ref == -1);
    uv_mutex_lock(&conn->refmutex);
    conn->refcount += ref; // +1 or -1
    if(conn->refcount == 0) {
        uv_mutex_unlock(&conn->refmutex);
        uv_mutex_destroy(&conn->refmutex);
        free(conn);
        return;
    }
    uv_mutex_unlock(&conn->refmutex);
}

static void uvx__on_read(uv_stream_t* uvclient, ssize_t nread, const uv_buf_t* buf) {
    uvx_server_conn_t* conn = (uvx_server_conn_t*) uvclient->data;
    assert(conn);
    uvx_server_t* xserver = conn->xserver;
    assert(xserver);
	if(nread > 0) {
        // 更新最后通讯时间，先删除再插入的处理依赖lh_table内部实现，保证最近通讯的连接在表尾。
        // 关联代码 _uvx_check_timeout_clients()
        conn->last_comm_time = uv_now(xserver->uvloop);
        int n = lh_table_delete(_UVX_S_PRIVATE(xserver)->conns, (const void*)conn);
        assert(n == 0); //delete success
        lh_table_insert(_UVX_S_PRIVATE(xserver)->conns, conn, (const void*)conn);

        if(xserver->config.on_read)
            xserver->config.on_read(xserver, conn, buf->base, nread);
	} else if(nread < 0) {
		if(xserver->config.log_err)
        fprintf(xserver->config.log_err, "\n!!! [uvx-server] %s on read error: %s\n", xserver->config.name, uv_strerror(nread));
		_uv_disconnect_client(uvclient);
	}
    free(buf->base);
}

static void _uv_after_close_connection(uv_handle_t* handle) {
	uvx_server_conn_t* conn = (uvx_server_conn_t*) handle->data;
    assert(conn && conn->xserver);
    uvx_server_t* xserver = conn->xserver;
    if(xserver->config.on_conn_close)
        xserver->config.on_conn_close(xserver, conn);
	int n = lh_table_delete(_UVX_S_PRIVATE(xserver)->conns, (const void*)conn);
	assert(n == 0); //delete success
	uvx_server_conn_ref(conn, -1); // call on_conn_close() inside here? in non-main-thread?
}

// defines in uvx.c
void uvx__on_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

static void uvx__on_connection(uv_stream_t* uvserver, int status) {
    uvx_server_t* xserver = (uvx_server_t*) uvserver->data;
    assert(xserver);
	if(status == 0) {
        if(xserver->config.log_out)
		    fprintf(xserver->config.log_out, "[uvx-server] %s on connection\n", xserver->config.name);
		assert(uvserver == (uv_stream_t*) &xserver->uvserver);
        assert(xserver->config.conn_extra_size >= 0);

        // Create new connection
		uvx_server_conn_t* conn = (uvx_server_conn_t*) calloc(1, sizeof(uvx_server_conn_t) + xserver->config.conn_extra_size);
        if(xserver->config.conn_extra_size > 0)
            conn->extra = (void*)(conn + 1);
        conn->xserver = xserver;
		conn->uvclient.data = conn;
        conn->last_comm_time = 0;
        conn->refcount = 1;
        uv_mutex_init(&conn->refmutex);

		// Save to connection list
		assert(lh_table_lookup_entry(_UVX_S_PRIVATE(xserver)->conns, conn) == NULL);
		lh_table_insert(_UVX_S_PRIVATE(xserver)->conns, conn, (const void*)conn);

		uv_tcp_init(xserver->uvloop, &conn->uvclient);
		if(uv_accept(uvserver, (uv_stream_t*) &conn->uvclient) == 0) {
			conn->last_comm_time = uv_now(xserver->uvloop);
            if(xserver->config.on_conn_ok)
                xserver->config.on_conn_ok(xserver, conn);
			uv_read_start((uv_stream_t*) &conn->uvclient, uvx__on_alloc_buf, uvx__on_read);
		} else {
			uv_close((uv_handle_t*) &conn->uvclient, _uv_after_close_connection);
		}
	} else {
		if(xserver->config.log_err)
            fprintf(xserver->config.log_err, "\n!!! [uvx-server] %s on connection error: %s\n", xserver->config.name, uv_strerror(status));
	}
}

static void _uv_disconnect_client(uv_stream_t* uvclient) {
	uvx_server_conn_t* conn = (uvx_server_conn_t*) uvclient->data;
	assert(uvclient == (uv_stream_t*) &conn->uvclient);
	uv_read_stop(uvclient);
	uv_close((uv_handle_t*)uvclient, _uv_after_close_connection);
}

//检查长时间未通讯的客户端，主动断开连接
static void _uvx_check_timeout_clients(uvx_server_t* xserver) {
	int conn_timeout = xserver->config.conn_timeout_seconds * 1000; // in milliseconds
    if(conn_timeout <= 0)
        return;
	struct lh_entry *e, *tmp;
	lh_foreach_safe(_UVX_S_PRIVATE(xserver)->conns, e, tmp) {
		uvx_server_conn_t* conn = (uvx_server_conn_t*) e->k;
		if(uv_now(xserver->uvloop) - conn->last_comm_time > conn_timeout) {
            if(xserver->config.log_out)
                fprintf(xserver->config.log_out, "[uvx-server] %s close connection %p for its long time silence\n",
                        xserver->config.name, &conn->uvclient);
			_uv_disconnect_client((uv_stream_t*) &conn->uvclient); // will delete connection
		} else {
			break; //后面都是最近通讯过的
		}
	}
}

int uvx_server_iter_conns(uvx_server_t* xserver, UVX_S_ON_ITER_CONN on_iter_conn, void* userdata) {
	if(on_iter_conn) {
		struct lh_entry *e, *tmp;
		lh_foreach_safe(_UVX_S_PRIVATE(xserver)->conns, e, tmp) {
			on_iter_conn(xserver, (uvx_server_conn_t*) e->k, userdata);
		}
	}
	return _UVX_S_PRIVATE(xserver)->conns->count;
}
