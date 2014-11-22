#include "../uvx.h"

// logs, to receive and print uvx logs.
// Author: Liigo <com.liigo@gmail.com>

static unsigned int log_count = 1; 

static void on_recv(uvx_udp_t* xudp, void* data, ssize_t datalen, const struct sockaddr* addr, unsigned int flag) {
    char ip[16]; int port; uvx_get_ip_port(addr, ip, sizeof(ip), &port);
    printf("recv: %d bytes from %s:%d \n", datalen, ip, port);

    char buf[1024];
    uvx_log_node_t* node = (uvx_log_node_t*) data;
    const char* extra = (const char*) (node + 1);
    snprintf(buf, sizeof(buf), "ver: %d, magic1: 0x%02x, magic2: 0x%02x\n"
                               "name: %s, tags: %s, ip: %s\n"
                               "level: %d, pid: %d, tid: %d, time: %d\n"
                               "msg: %s\n"
                               "file: %s, line: %d\n"
                               "-------- received logs count: %d --------\n",
                               node->version, node->magic1, node->magic2,
                               extra + node->name_offset, extra + node->tags_offset, ip,
                               node->level, node->pid, node->tid, node->time,
                               extra + node->msg_offset, extra + node->file_offset, node->line,
                               log_count++);
    puts(buf);
}

void main(int argc, char** argv) {
    uv_loop_t* loop = uv_default_loop();
    uvx_udp_t xudp;
    uvx_udp_config_t config = uvx_udp_default_config(&xudp);
    config.on_recv = on_recv;

    uvx_udp_start(&xudp, loop, "127.0.0.1", 19730, config); // 用于接收LOG消息

    uv_run(loop, UV_RUN_DEFAULT);
}
