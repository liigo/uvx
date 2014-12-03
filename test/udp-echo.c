#include "../uvx.h"
#include <string.h>
#include <unistd.h>

// 运行本程序的两个进程：
// 第一个：./udpecho
// 第二个: ./udpecho 127.0.0.1
// 然后两者会互发信息（一方收到消息会立刻答复给对方），如此无限循环下去
// 作者: Liigo <com.liigo@gmail.com>

static void on_recv(uvx_udp_t* xudp, void* data, ssize_t datalen, const struct sockaddr* addr, unsigned int flag) {
    char ip[16]; int port; uvx_get_ip_port(addr, ip, sizeof(ip), &port);
    printf("recv: %s  size: %d  from %s:%d \n", data, datalen, ip, port);
    int x = atoi(data);
    char newdata[16]; sprintf(newdata, "%d", x + 1);
    uvx_udp_send_to_addr(xudp, addr, newdata, strlen(newdata) + 1);
    sleep(1);
}

void main(int argc, char** argv) {
    char* target_ip = NULL;
    uv_loop_t* loop = uv_default_loop();
    uvx_udp_t xudp;
    uvx_udp_config_t config = uvx_udp_default_config(&xudp);
    config.on_recv = on_recv;

    if(argc > 1) target_ip = argv[1];
    if(target_ip == NULL) {
        uvx_udp_start(&xudp, loop, "0.0.0.0", 8008, config); // 绑定固定端口
    } else {
        uvx_udp_start(&xudp, loop, NULL, 0, config); // 绑定随机端口
        uvx_udp_send_to_ip(&xudp, target_ip, 8008, "1", 2); // 启动消息互发动作，碰撞式应答
    }

    uv_run(loop, UV_RUN_DEFAULT);
}
