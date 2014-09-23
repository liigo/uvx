#include "../uvx.h"

// Author: Liigo <com.liigo@gmail.com>

void main() {
    uv_loop_t* loop = uv_default_loop();
    uvx_client_t client;
    uvx_client_config_t config = uvx_client_default_config(&client);
    uvx_client_connect(&client, loop, "127.0.0.1", 8001, config);
    uv_run(loop, UV_RUN_DEFAULT);
}
