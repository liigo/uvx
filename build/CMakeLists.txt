cmake_minimum_required(VERSION 2.6)

PROJECT(uvx)

SET(CMAKE_C_FLAGS "-O3 -DNDEBUG -std=gnu99 -pthread -fpic")

SET(UVX_SOURCES 
	../uvx.c
	../uvx_server.c
	../uvx_client.c
	../uvx_udp.c
	../uvx_log.c
	../loge/loge.c
	../utils/automem.c
	../utils/linkhash.c
)

ADD_LIBRARY(uvx ${UVX_SOURCES})

TARGET_LINK_LIBRARIES(uvx uv pthread rt)
