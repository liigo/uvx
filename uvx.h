#ifndef __LIIGO_UVX_H__
#define __LIIGO_UVX_H__

#ifdef __cplusplus
extern "C"	{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <uv.h>
#include "loge/loge.h"
#include "utils/automem.h"

//-----------------------------------------------
// uvx: a lightweight wrapper of libuv, defines `uvx_server_t`(TCP server),
// `uvx_client_t`(TCP client) and `uvx_udp_t`(UDP), along with an `uvx_log_t`(UDP logger).
//
// - To define a TCP server (which I call it xserver), you're just required to provide
//   a `uvx_server_config_t` with some params and callbacks, then call `uvx_server_start`.
// - To define a TCP client (which I call it xclient), you're just required to provide
//   a `uvx_client_config_t` with some params and callbacks, then call `uvx_client_connect`.
// - To define an UDP service (which I call it xudp), you're just required to provide
//   a `uvx_udp_config_t` with some params and callbacks, then call `uvx_udp_start`.
// - To define a logging service (which I call it xlog), just call `uvx_log_init`.
//
// There are a few predefined callbacks, such as on_conn_ok, on_conn_close, on_heartbeat, etc.
// All callbacks are optional. Maybe `on_recv` is the most useful one you care about.
//
// by Liigo 2014-11.
// http://github.com/liigo/uvx


//-----------------------------------------------
// uvx tcp server: `uvx_server_t`

typedef struct uvx_server_s uvx_server_t;
typedef struct uvx_server_conn_s uvx_server_conn_t;

typedef void (*UVX_S_ON_CONN_OK)        (uvx_server_t* xserver, uvx_server_conn_t* conn);
typedef void (*UVX_S_ON_CONN_FAIL)      (uvx_server_t* xserver, uvx_server_conn_t* conn);
typedef void (*UVX_S_ON_CONN_CLOSING)   (uvx_server_t* xserver, uvx_server_conn_t* conn);
typedef void (*UVX_S_ON_CONN_CLOSE)     (uvx_server_t* xserver, uvx_server_conn_t* conn);
typedef void (*UVX_S_ON_ITER_CONN)      (uvx_server_t* xserver, uvx_server_conn_t* conn, void* userdata);
typedef void (*UVX_S_ON_RECV)           (uvx_server_t* xserver, uvx_server_conn_t* conn, void* data, ssize_t datalen);
typedef void (*UVX_S_ON_HEARTBEAT)      (uvx_server_t* xserver, unsigned int index);

typedef struct uvx_server_config_s {
    char name[32];    // the xserver's name (with-ending-'\0')
    int conn_count;   // estimated connections count
    int conn_backlog; // used by uv_listen()
    int conn_extra_size; // the bytes of extra data, see `uvx_server_conn_t.extra`
    float conn_timeout_seconds; // if > 0, timeout-ed connections will be closed
    float heartbeat_interval_seconds; // used by heartbeat timer
    // callbacks
    UVX_S_ON_CONN_OK        on_conn_ok;
    UVX_S_ON_CONN_FAIL      on_conn_fail;
    UVX_S_ON_CONN_CLOSING   on_conn_closing;
    UVX_S_ON_CONN_CLOSE     on_conn_close;
    UVX_S_ON_HEARTBEAT      on_heartbeat;
    UVX_S_ON_RECV           on_recv;
    // logs
    FILE* log_out;
    FILE* log_err;
} uvx_server_config_t;

struct uvx_server_s {
    uv_loop_t* uvloop;
    uv_tcp_t   uvserver;
    uvx_server_config_t config;
    unsigned char privates[136]; // to store uvx_server_private_t
    void* data; // for public use
};
typedef struct uvx_server_s uvx_server_t;

typedef struct uvx_server_conn_s {
    uvx_server_t* xserver;
    uv_tcp_t uvclient;
    uint64_t last_comm_time; // time of last communication (uv_now(loop))
    int refcount;
    uv_mutex_t refmutex;
    void* extra; // pointer to extra data, if config.conn_extra_size > 0, or else is NULL
    // extra data resides here
} uvx_server_conn_t;

// returns the default config for xserver, used by uvx_server_start().
uvx_server_config_t uvx_server_default_config(uvx_server_t* xserver);

// start an xserver listening on ip:port, support IPv4 and IPv6.
// please pass in uninitialized xserver and initialized config.
// returns 1 on success, or 0 if fails.
int uvx_server_start(uvx_server_t* xserver, uv_loop_t* loop, const char* ip, int port, uvx_server_config_t config);

// shutdown an xserver normally.
// returns 1 on success, or 0 if fails.
int uvx_server_shutdown(uvx_server_t* xserver);

// iterate all connections if on_iter_conn != NULL.
// returns the number of connections.
int uvx_server_iter_conns(uvx_server_t* xserver, UVX_S_ON_ITER_CONN on_iter_conn, void* userdata);

// manager conn refcount manually, +1 or -1, free conn when refcount == 0. threadsafe.
void uvx_server_conn_ref(uvx_server_conn_t* conn, int ref);

//-----------------------------------------------
// uvx tcp client: `uvx_client_t`

typedef struct uvx_client_s uvx_client_t;

//TODO: combine UVX_C_ON_CONNECT_* to UVX_C_ON_CONNECT
typedef void (*UVX_C_ON_CONN_OK)        (uvx_client_t* xclient);
typedef void (*UVX_C_ON_CONN_FAIL)      (uvx_client_t* xclient);
typedef void (*UVX_C_ON_CONN_CLOSING)   (uvx_client_t* xclient);
typedef void (*UVX_C_ON_CONN_CLOSE)     (uvx_client_t* xclient);
typedef void (*UVX_C_ON_RECV)           (uvx_client_t* xclient, void* data, ssize_t datalen);
typedef void (*UVX_C_ON_HEARTBEAT)      (uvx_client_t* xclient, unsigned int index);

typedef struct uvx_client_config_s {
    char name[32];    // the xclient's name (with-ending-'\0')
    int auto_connect; // 1: on, 0: off
    float heartbeat_interval_seconds;
    // callbacks
    UVX_C_ON_CONN_OK       on_conn_ok;
    UVX_C_ON_CONN_FAIL     on_conn_fail;
    UVX_C_ON_CONN_CLOSING  on_conn_closing;
    UVX_C_ON_CONN_CLOSE    on_conn_close;
    UVX_C_ON_RECV          on_recv;
    UVX_C_ON_HEARTBEAT     on_heartbeat;
    // logs
    FILE* log_out;
    FILE* log_err;
} uvx_client_config_t;

struct uvx_client_s {
    uv_loop_t* uvloop;
    uv_tcp_t   uvclient;
    uv_tcp_t*  uvserver; // &uvclient or NULL
    uvx_client_config_t config;
    unsigned char privates[224]; // stores value of uvx_client_private_t
    void* data;
};
typedef struct uvx_client_s uvx_client_t;

// returns the default config for xclient, used by uvx_client_connect().
uvx_client_config_t uvx_client_default_config(uvx_client_t* xclient);

// connect to an tcp server (not only xserver) that listening ip:port. support IPv4 and IPv6.
// please pass in uninitialized xclient and initialized config.
// returns 1 on success, or 0 if fails.
int uvx_client_connect(uvx_client_t* xclient, uv_loop_t* loop, const char* ip, int port, uvx_client_config_t config);


//-----------------------------------------------
// uvx udp: `uvx_udp_t`

typedef struct uvx_udp_s uvx_udp_t;

typedef void (*UVX_UDP_ON_RECV) (uvx_udp_t* xudp, void* data, ssize_t datalen, const struct sockaddr* addr, unsigned int flags);

typedef struct uvx_udp_config_s {
    char name[32];    // the xudp's name (with-ending-'\0')
    // callbacks
    UVX_UDP_ON_RECV on_recv;
    // logs
    FILE* log_out;
    FILE* log_err;
} uvx_udp_config_t;

struct uvx_udp_s {
    uv_loop_t* uvloop;
    uv_udp_t   uvudp;
    uvx_udp_config_t config;
    void* data;
};

// returns the default config for xudp, used by uvx_udp_start().
uvx_udp_config_t uvx_udp_default_config(uvx_udp_t* xudp);

// start the udp service bind on ip:port. support IPv4 and IPv6.
// if ip == NULL, bind to local random port automatically.
// please pass in uninitialized xudp and initialized config.
// returns 1 on success, or 0 if fails.
int uvx_udp_start(uvx_udp_t* xudp, uv_loop_t* loop, const char* ip, int port, uvx_udp_config_t config);

// send data through udp. support IPv4 and IPv6.
// will copy data internally, so caller can take it easy to free it after this call.
int uvx_udp_send_to_ip(uvx_udp_t* xudp, const char* ip, int port, const void* data, unsigned int datalen);
int uvx_udp_send_to_addr(uvx_udp_t* xudp, const struct sockaddr* addr, const void* data, unsigned int datalen);

// set broadcast on (1) or off (0)
// returns 1 on success, or 0 if fails.
int uvx_udp_set_broadcast(uvx_udp_t* xudp, int on);

// shutdown the xudp normally.
// returns 1 on success, or 0 if fails.
int uvx_udp_shutdown(uvx_udp_t* xudp);


//-----------------------------------------------
// uvx_log: `uvx_log_t`

typedef struct uvx_log_t {
    uv_loop_t* uvloop;
    uvx_udp_t xudp;
    union {
        struct sockaddr     addr;
        struct sockaddr_in  addr4;
        struct sockaddr_in6 addr6;
    } target_addr;

    loge_t loge;
} uvx_log_t;

// predefined log levels
#define UVX_LOG_ALL    LOGE_LOG_ALL
#define UVX_LOG_TRACE  LOGE_LOG_TRACE
#define UVX_LOG_DEBUG  LOGE_LOG_DEBUG
#define UVX_LOG_INFO   LOGE_LOG_INFO
#define UVX_LOG_WARN   LOGE_LOG_WARN
#define UVX_LOG_ERROR  LOGE_LOG_ERROR
#define UVX_LOG_FATAL  LOGE_LOG_FATAL
#define UVX_LOG_NONE   LOGE_LOG_NONE

// init an uvx log with uv loop, ip, port and name.
// please pass in uninitialized xlog. name can be NULL, default to "xlog".
// logs are sent to its target (target_ip:target_port) through IPv4/IPv6 + UDP.
// returns 1 on success, or 0 if fails.
int uvx_log_init(uvx_log_t* xlog, uv_loop_t* loop, const char* target_ip, int target_port, const char* name);

// send a log to target through UDP.
// level: see UVX_LOG_*; tags: comma separated text; msg: log content text.
// file and line: the source file path+name and line number.
// parameter tags/msg/file can be NULL, and may be truncated if too long.
// all text parameters should be utf-8 encoded, or utf-8 compatible.
// returns 1 on success, or 0 if fails.
// note: be limited by libuv, we should call `uvx_log_send` only in its main loop thread.
int uvx_log_send(uvx_log_t* xlog, int level, const char* tags, const char* msg, const char* file, int line);

// serialize a log into bytes stream, ready to be sent by `uvx_log_send_serialized` later.
// buf and bufsize: an output buffer and its size, recommend 1024, can be smaller or larger.
// the other parameters are as same as `uvx_log_send`.
// returns the serialized data size in bytes, which is ensure not exceed bufsize and 1024.
// maybe returns 0, which means nothing was serialized (e.g. when the log was disabled).
// example:
//   char buf[1024];
//   unsigned int len = uvx_log_serialize(xlog, buf, sizeof(buf), ...);
//   uvx_log_send_serialized(xlog, buf, len);
unsigned int
uvx_log_serialize(uvx_log_t* xlog, void* buf, unsigned int bufsize,
                  int level, const char* tags, const char* msg, const char* file, int line);

// the binary version of `uvx_log_serialize`, `msg` is a binary data instead of a text.
// returns 0 if the msg's size is too long.
// see `uvx_log_serialize` for more details.
unsigned int
uvx_log_serialize_bin(uvx_log_t* xlog, void* buf, unsigned int bufsize, int level, const char* tags,
                      const void* msg, unsigned int msglen, const char* file, int line);

// send a serialized log to target through UDP.
// the parameter `data`/`datalen` must be serialized by `uvx_log_serialize` before.
// returns 1 on success, or 0 if fails.
// note: be limited by libuv, we should call `uvx_log_send_serialized` only in its main loop thread.
int uvx_log_send_serialized(uvx_log_t* xlog, const void* data, unsigned int datalen);

// to enable (if enabled==1) or disable (if enabled==0) the log
void uvx_log_enable(uvx_log_t* xlog, int enabled);


// a printf-like UVX_LOG utility macro, to format and send a log.
// parameters:
//   log: an uvx_log_t* which already uvx_log_start()-ed
//   level: one of UVX_LOG_* consts
//   tags: comma separated text
//   msgfmt: the format text for msg, e.g. "something %s %d or else"
//   ...: the values that match %* in msgfmt
// examples:
//   UVX_LOG(&log, UVX_LOG_INFO, "uvx,liigo", "%d %s", 123, "liigo");
//   UVX_LOG(&log, UVX_LOG_INFO, "uvx,liigo", "pure text without format", NULL);
#define UVX_LOG(xlog,level,tags,msgfmt,...) {\
        char uvx_tmp_msg_[LOGE_MAXBUF]; /* avoid name conflict with outer-scope names */ \
        snprintf(uvx_tmp_msg_, sizeof(uvx_tmp_msg_), msgfmt, __VA_ARGS__);\
        uvx_log_send(xlog, level, tags, uvx_tmp_msg_, __FILE__, __LINE__);\
    }

// only serialize a log, but not send it.
// `bufsize` will be rewrite to fill in the serialized size.
// example:
//   char buf[1024]; unsigned int len = sizeof(buf);
//   UVX_LOG_SERIALIZE(&xlog, buf, len, UVX_LOG_INFO, "author", "name: %s, sex: %d", "Liigo", 1);
//   uvx_log_send_serialized(&xlog, buf, len);
#define UVX_LOG_SERIALIZE(xlog,buf,bufsize,level,tags,msgfmt,...) {\
        char uvx_tmp_msg_[LOGE_MAXBUF]; /* avoid name conflict with outer-scope names */ \
        snprintf(uvx_tmp_msg_, sizeof(uvx_tmp_msg_), msgfmt, __VA_ARGS__);\
        bufsize = uvx_log_serialize(xlog, buf, bufsize, level, tags, uvx_tmp_msg_, __FILE__, __LINE__);\
    }


//-----------------------------------------------
// other

// get text presentation ip address, writing to ipbuf and returns ipbuf. writing port if not NULL.
// support IPv4 (buflen = 16) and IPv6 (buflen = 40).
const char* uvx_get_ip_port(const struct sockaddr* addr, char* ipbuf, int buflen, int* port);

// get binary presentation ip address, writing to ipbuf and returns lengh in bytes of it. writing port if not NULL.
// support IPv4 (buflen = 4) and IPv6 (buflen = 16).
int uvx_get_raw_ip_port(const struct sockaddr* addr, unsigned char* ipbuf, int* port);

// get the tcp-client's text presentation ip address, writing to ipbuf and returns ipbuf. writing port if not NULL.
// support IPv4 (buflen = 16) and IPv6 (buflen = 40).
const char* uvx_get_tcp_ip_port(uv_tcp_t* uvclient, char* ipbuf, int buflen, int* port);

// send mem to a libuv tcp stream. don't use mem any more after this call.
int uvx_send_mem(automem_t* mem, uv_stream_t* stream);


#if defined(_WIN32) && !defined(__GNUC__)
#include <stdarg.h>
// Emulate snprintf() on Windows, _snprintf() doesn't zero-terminate the buffer on overflow...
int snprintf(char* buf, size_t len, const char* fmt, ...);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__LIIGO_UVX_H__
