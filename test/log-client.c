#include "../uvx.h"
#include <time.h>

// logc, to generate and send uvx logs
// Author: Liigo <com.liigo@gmail.com>
// Usage:
//   ./logc         Send a log  per 10 seconds
//   ./logc x       Send x logs per 10 seconds
//   ./logc x y     Send x logs per y seconds


#ifndef WIN32
    #define _UINT64_FMT     "lld"
#else
    #define _UINT64_FMT     "I64x"
#endif

uvx_log_t xlog;
uv_loop_t* loop = NULL;
int bench = 1;
int interval = 10; // Seconds

static void on_timer(uv_timer_t* handle) {
    char msg[1024];
    const char* s = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
    snprintf(msg, sizeof(msg), "Repeated text: (1) %s (2) %s (3) %s (4) %s", s, s, s, s);

    uint64_t elapsed_time = uv_hrtime(); // ns
    for(int i = 0; i < bench; i++) {
        // uvx_log_send(&xlog, UVX_LOG_INFO, "xlog,test,liigo", msg, "/home/liigo/source.c", i + 1);
        UVX_LOG(&xlog, UVX_LOG_INFO, "xlog,test,liigo", "Log content: %s", msg);
    }
    printf("sent %d logs, elapsed time: %"_UINT64_FMT" us (1000us = 1ms).\n", bench, (uv_hrtime() - elapsed_time) / 1000);
}

void main(int argc, char** argv) {
    loop = uv_default_loop();
    if(argc > 1) {
        bench = atoi(argv[1]);
    }
    if(argc > 2) {
        interval = atoi(argv[2]); // Seconds
    }

    // start log client
    uvx_log_start(&xlog, loop, "127.0.0.1", 19730, "xlog-test");

    // start timer
    uv_timer_t timer;
    
    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, on_timer, 0, interval/*seconds*/ * 1000);

    uv_run(loop, UV_RUN_DEFAULT);
}
