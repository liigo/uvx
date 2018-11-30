#include "../uvx.h"
#include "../loge/loge.h"
#include <time.h>

// logs, to receive and print loge loggings.
// Author: Liigo <liigo@qq.com>

static unsigned int log_count = 1;
static char timestr_buf[32];

static const char* localtimestr(int time) {
	time_t t = (time_t)time;
	strftime(timestr_buf, sizeof(timestr_buf), "[%Y-%m-%d %X]", localtime(&t));
	return (const char*)timestr_buf;
}

static void on_recv(uvx_udp_t* xudp, void* data, ssize_t datalen, const struct sockaddr* addr, unsigned int flag) {
    char ip[16]; int port; uvx_get_ip_port(addr, ip, sizeof(ip), &port);
    printf("recv: %d bytes from %s:%d \n", datalen, ip, port);

    char buf[2048];
    loge_item_t* item = (loge_item_t*) data;
    const char* extra = (const char*)item + item->extra_offset;
    snprintf(buf, sizeof(buf), "ver: %d, magic1: 0x%02x, magic2: 0x%02x\n"
                               "name: %s, tags: %s, ip: %s\n"
							   "level: %d, pid: %d, tid: %d, time: %d %s\n"
                               "msg: %s\n"
                               "file: %s, line: %d\n"
                               "msg_len: %d, extra_offset: %d, flags: %d\n"
                               "-------- received logs count: %d --------\n",
                               item->version, item->magic1, item->magic2,
                               extra + item->name_offset, extra + item->tags_offset, ip,
							   item->level, item->pid, item->tid, item->time, localtimestr(item->time),
                               extra + item->msg_offset, extra + item->file_offset, item->line,
                               item->msg_len, item->extra_offset, item->flags,
                               log_count++);
    puts(buf);
}

void main(int argc, char** argv) {
    uv_loop_t* loop = uv_default_loop();
    uvx_udp_t xudp;
    uvx_udp_config_t config = uvx_udp_default_config(&xudp);
    config.on_recv = on_recv;

    uvx_udp_start(&xudp, loop, "127.0.0.1", 8004, config);

    uv_run(loop, UV_RUN_DEFAULT);
}
