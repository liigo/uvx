cmake_minimum_required(VERSION 2.6)

PROJECT(uvx_test)

SET(CMAKE_C_FLAGS "-g -std=gnu99 -pthread -fpic")
SET(CMAKE_CXX_FLAGS "-g -std=gnu++0x -pthread -fpic")

#O2 -DNDEBUG -Wall -Werror -Werror=parentheses

SET(SERVER_SOURCES
	../server.c 
	../../uvx.c
	../../uvx_server.c
	../../utils/automem.c
	../../utils/linkhash.c
)

ADD_EXECUTABLE(server ${SERVER_SOURCES})
TARGET_LINK_LIBRARIES(server uv pthread rt)


SET(CLIENT_SOURCES
	../client.c 
	../../uvx.c
	../../uvx_client.c
	../../utils/automem.c
	../../utils/linkhash.c
)

ADD_EXECUTABLE(client ${CLIENT_SOURCES})
TARGET_LINK_LIBRARIES(client uv pthread rt)


SET(UDPECHO_SOURCES
	../udp-echo.c
	../../uvx.c
	../../uvx_udp.c
	../../utils/automem.c
	../../utils/linkhash.c
)

ADD_EXECUTABLE(udpecho ${UDPECHO_SOURCES})
TARGET_LINK_LIBRARIES(udpecho uv pthread rt)

SET(LOGC_SOURCES
	../log-client.c
	../../uvx.c
	../../uvx_log.c
	../../uvx_udp.c
	../../utils/automem.c
	../../utils/linkhash.c
)

ADD_EXECUTABLE(logc ${LOGC_SOURCES})
TARGET_LINK_LIBRARIES(logc uv pthread rt)

SET(LOGS_SOURCES
	../log-server.c
	../../uvx.c
	../../uvx_udp.c
	../../utils/automem.c
	../../utils/linkhash.c
)

ADD_EXECUTABLE(logs ${LOGS_SOURCES})
TARGET_LINK_LIBRARIES(logs uv pthread rt)
