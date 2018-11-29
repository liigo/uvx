uvx
===

A libuv based network extension library focusing on usability.

Please ref [uvx.h](uvx.h) for detailed document.

## Build

```
cd build
cmake .
make
```

## Test

```
cd test/build
cmake .
make
```

## 与 WX/QT/MFC 等其他消息循环的整合应用

- 初始化UV和UVX（在窗口创建之前或之后均可）

```c
	g_uvloop = uv_default_loop();
	uvx_client_config_t config = uvx_client_default_config(&g_xclient);
	config.on_recv = xclient_on_recv;
	uvx_client_connect(&g_xclient, g_uvloop, "127.0.0.1", 8001, config);
```

- 在 Timer 或 Idle 事件内不断地调用 `uv_run(g_uvloop, UV_RUN_NOWAIT);` 以驱动libuv运转

该调用仅执行少数必要的动作，会很快返回，不会明显影响现有消息循环。

注意不能用 `UV_RUN_ONCE`，它会阻塞UI线程；更不能用 `UV_RUN_DEFAULT`，两个消息循环会打架。

这里不推荐在独立线程中执行 `uv_run(g_uvloop, UV_RUN_DEFAULT);`，它会引入不必要的线程同步机制，使得代码复杂化。

- 终结UVX和UV（在窗口关闭之后）

```c
	uvx_client_shutdown(&g_xclient);
	// uv_stop(g_uvloop);
	uv_run(g_uvloop, UV_RUN_DEFAULT);
	uv_loop_close(g_uvloop);
```

注意这里使用 `UV_RUN_DEFAULT`。 单次调用 `UV_RUN_NOWAIT` 或 `UV_RUN_ONCE` 均不足于确保等到 libuv 消息循环正常终结。

正常情况不需要调用 `uv_stop()`。但如果你程序中还有 libuv 的 handle 或 request 未处理，先调用 `uv_stop()` 可避免 `uv_run()` 内部死循环无法正常返回，确保进程退出。

## License

MIT
