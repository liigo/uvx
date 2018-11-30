#include "../uvx.h"

// Author: Liigo <liigo@qq.com>
// Usage:
//   ./client         Send 1 data  per 10 seconds
//   ./client x       Send x datas per 10 seconds
//   ./client x y     Send x datas per y seconds

int bench = 1;
int interval = 10; // Seconds

static void on_heartbeat(uvx_client_t* xclient, unsigned int index) {
    static char data[] = {'u','v','x','-','s','e','r','v','e','r',',',
                          'b','y',' ','l','i','i','g','o','\n'};
    printf("send %d datas\n", bench);
    for(int i = 0; i < bench; i++) {
        automem_t mem; automem_init(&mem, 32);
        automem_append_voidp(&mem, data, sizeof(data));
        uvx_send_mem(&mem, (uv_stream_t*)&xclient->uvclient);
    }
}

void main(int argc, char** argv) {
    if(argc > 1) {
        bench = atoi(argv[1]);
    }
    if(argc > 2) {
        interval = atoi(argv[2]); // Seconds
    }

    uv_loop_t* uvloop = uv_default_loop();
    uvx_client_t client;
    uvx_client_config_t config = uvx_client_default_config(&client);
    config.on_heartbeat = on_heartbeat;
    config.heartbeat_interval_seconds = (float)interval;
    uvx_client_connect(&client, uvloop, "127.0.0.1", 8001, config);

    uv_run(uvloop, UV_RUN_DEFAULT);
}
