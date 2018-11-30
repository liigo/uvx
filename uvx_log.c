#include "uvx.h"
#include <math.h>
#include <assert.h>

// Author: Liigo <liigo@qq.com>, 201407.

int uvx_log_init(uvx_log_t* xlog, uv_loop_t* loop, const char* target_ip, int target_port, const char* name) {
    xlog->uvloop = loop;
    assert(target_ip);
    if(strchr(target_ip, ':')) {
        uv_ip6_addr(target_ip, target_port, &xlog->target_addr.addr6);
    } else {
        uv_ip4_addr(target_ip, target_port, &xlog->target_addr.addr4);
    }

    loge_init(&xlog->loge, name);
    return uvx_udp_start(&xlog->xudp, loop, NULL, 0, uvx_udp_default_config(&xlog->xudp));
}

#if defined(_MSC_VER)
	#define UVXLOG_INLINE /*__inline*/
#else
	#define UVXLOG_INLINE inline
#endif

UVXLOG_INLINE
void uvx_log_enable(uvx_log_t* xlog, int enabled) {
    loge_enable(&xlog->loge, enabled);
}

UVXLOG_INLINE unsigned int
uvx_log_serialize(uvx_log_t* xlog, void* buf, unsigned int bufsize,
                  int level, const char* tags, const char* msg, const char* file, int line) {
    return loge_item(&xlog->loge, buf, bufsize, level, tags, msg, file, line);
}

UVXLOG_INLINE unsigned int
uvx_log_serialize_bin(uvx_log_t* xlog, void* buf, unsigned int bufsize, int level, const char* tags,
                      const void* msg, unsigned int msglen, const char* file, int line) {
	return loge_item_bin(&xlog->loge, buf, bufsize, level, tags, msg, msglen, file, line);
}

UVXLOG_INLINE
int uvx_log_send(uvx_log_t* xlog, int level, const char* tags, const char* msg, const char* file, int line) {
    char buf[LOGE_MAXBUF];
    unsigned int size = loge_item(&xlog->loge, buf, sizeof(buf), level, tags, msg, file, line);
    // send out through udp
    return uvx_udp_send_to_addr(&xlog->xudp, &xlog->target_addr.addr, buf, size);
}

UVXLOG_INLINE
int uvx_log_send_bin(uvx_log_t* xlog, int level, const char* tags,
                     const void* msg, unsigned int msglen, const char* file, int line) {
	char buf[LOGE_MAXBUF];
	unsigned int size = loge_item_bin(&xlog->loge, buf, sizeof(buf), level, tags, msg, msglen, file, line);
	// send out through udp
	return uvx_udp_send_to_addr(&xlog->xudp, &xlog->target_addr.addr, buf, size);
}

UVXLOG_INLINE
int uvx_log_send_serialized(uvx_log_t* xlog, const void* data, unsigned int datalen) {
    return uvx_udp_send_to_addr(&xlog->xudp, &xlog->target_addr.addr, data, datalen);
}
