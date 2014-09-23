#include "../uvx.h"

// Author: Liigo <com.liigo@gmail.com>

void main() {
    uv_loop_t* loop = uv_default_loop();
    uvx_server_t server;
    uvx_server_config_t config = uvx_server_default_config(&server);
    uvx_server_start(&server, loop, "127.0.0.1", 8001, config);
    uv_run(loop, UV_RUN_DEFAULT);
}
