#include "../uvx.h"

// Author: Liigo <com.liigo@gmail.com>

static void on_recv(uvx_server_t* xserver, uvx_server_conn_t* conn, void* data, ssize_t datalen) {
    char ip[40]; int port;
    uvx_get_tcp_ip_port(&conn->uvclient, ip, sizeof(ip), &port);
    printf("receive %d bytes from %s:%d\n", datalen, ip, port);
}

void main() {
    uv_loop_t* loop = uv_default_loop();
    uvx_server_t server;
    uvx_server_config_t config = uvx_server_default_config(&server);
    config.on_recv = on_recv;
    uvx_server_start(&server, loop, "127.0.0.1", 8001, config);
    uv_run(loop, UV_RUN_DEFAULT);
}
