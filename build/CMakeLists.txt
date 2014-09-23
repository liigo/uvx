cmake_minimum_required(VERSION 2.6)

PROJECT(uvx)

SET(CMAKE_C_FLAGS "-Os -std=gnu99 -pthread -fpic")
SET(CMAKE_CXX_FLAGS "-Os -std=gnu++0x -pthread -fpic")

#O2 -DNDEBUG -Wall -Werror -Werror=parentheses

SET(UVX_SOURCES 
	../uvx.c
	../uvx_server.c
	../uvx_client.c
	../uvx_udp.c
	../uvx_log.c
	../utils/automem.c
	../utils/linkhash.c
)

ADD_LIBRARY(uvx ${UVX_SOURCES})
TARGET_LINK_LIBRARIES(uvx uv pthread rt)
