#include "uvx.h"
#include <math.h>
#include <assert.h>

// Author: Liigo <com.liigo@gmail.com>, 201407.

#ifdef __unix__
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/syscall.h>  
    #define gettid() syscall(SYS_gettid)
#endif

#define UVX_MIN(a,b) ((a)<(b)?(a):(b))

int uvx_log_init(uvx_log_t* xlog, uv_loop_t* loop, const char* target_ip, int target_port, const char* name) {
    xlog->uvloop = loop;
    assert(target_ip);
    if(strchr(target_ip, ':')) {
        uv_ip6_addr(target_ip, target_port, &xlog->target_addr.addr6);
    } else {
        uv_ip4_addr(target_ip, target_port, &xlog->target_addr.addr4);
    }

    // copy name, truncates if need
    if(name == NULL) name = "xlog";
    int n = strlen(name);
    n = UVX_MIN(n, sizeof(xlog->name) - 1);
    xlog->name_len = n;
    assert(xlog->name_len >= 0 && xlog->name_len < sizeof(xlog->name));
    memcpy(xlog->name, name, n);
    xlog->name[n] = '\0';

    xlog->enabled = 1;
    xlog->pid = (int) getpid();

    uvx_udp_start(&xlog->xudp, loop, NULL, 0, uvx_udp_default_config(&xlog->xudp));
    return 1;
}

void uvx_log_enable(uvx_log_t* xlog, int enabled) {
    xlog->enabled = enabled;
}

// reverse find a character from specified position
static const char* strrchr_from(const char* str, int from, char c) {
    assert(str);
    while(from > 0 && str[from] != c) {
        from--;
    }
    if(from > 0) {
        return str + from;
    } else {
        return str[from] == c ? str : NULL;
    }
}

// we only keep the last at most two directories of the file path.
// i.e. "/home/liigo/project/src/file.c" => "project/src/file.c"
static const char* shorten_path(const char* file) {
    if(file == NULL) return NULL;
    char slash = '/';
    const char* last_slash = strrchr(file, slash);
    if(last_slash == NULL) {
        slash = '\\';
        last_slash = strrchr(file, slash);
    }

    int count = 2;
    while(last_slash && last_slash != file && count-- > 0) {
        last_slash = strrchr_from(file, last_slash - file - 1, slash);
    }
    return (last_slash && last_slash != file) ? (last_slash + 1) : file;
}

// from specified position, find the last leading-byte of a utf-8 encoded character,
// returns its index in buf, or returns 0 if not find.
static int rfind_utf8_leading_byte_index(char* buf, int from) {
    while(from >= 0) {
        unsigned char c = buf[0];
        // utf-8 leading bytes: 0-, 110-, 1110-, 11110-
        if(c>>7==0 || c>>5==6 || c>>4==14 || c>>3==30) {
            return from;
        }
        from--;
    }
    return 0;
}

// write str to buf, maybe truncates str if there is no enough space between buf and buf_end.
// buf and buf_end must be non-NULL, str can be NULL.
// returns the next available buf to write another str.
// we ensure that str wrote to buf end with '\0' even it was truncated.
// we ensure that utf-8 encoded str will not be truncated inside a character.
static char* write_str(char* buf, const char* buf_end, const char* str) {
    assert(buf <= buf_end);
    if(buf == buf_end)
        return (char*) buf_end;
    if(str == NULL || *str == '\0') {
        buf[0] = '\0';
        return buf + 1;
    } else {
        int n = strlen(str) + 1;
        n = UVX_MIN(n, buf_end - buf);
        memcpy(buf, str, n);
        n = rfind_utf8_leading_byte_index(buf, n);
        buf[n] = '\0';
        return buf + n + 1;
    }
}

int uvx_log_send(uvx_log_t* xlog, int level, const char* tags, const char* msg, const char* file, int line) {
    char buf[UVX_LOGNODE_MAXBUF];
    uvx_log_node_t* node = (uvx_log_node_t*) buf;
    char* extra = buf + sizeof(uvx_log_node_t);
    const char* extra_end = buf + UVX_LOGNODE_MAXBUF;
    char* p = extra;

    assert(UVX_LOGNODE_MAXBUF > sizeof(uvx_log_node_t) && "check UVX_LOGNODE_MAXBUF size");
    if(!xlog->enabled) return 0;

    node->version = 1;    // the uvx log protocol version number, increase if `uvx_log_node_t` changed
    node->magic1  = 0xa1; // a109 ~= alog
    node->magic2  = 0x09; // which means this is a log :)
    node->level   = (int8_t) level;

    node->time = (int32_t) time(NULL);
    node->pid = xlog->pid;
    node->tid = (int) gettid();
    node->line = line;

    // fill strings and it's offsets.
    // the length of first three strings (name/tags/file) can't exceed 255,
    // to make sure the last offset (msg_offset) is valid uint8_t value.

    // name and name_offset
    node->name_offset = 0;
    assert(sizeof(xlog->name) < UVX_MIN(255, extra_end - extra));
    memcpy(extra, xlog->name, xlog->name_len + 1);
    p += xlog->name_len + 1;
    // tags and tags_offset
    node->tags_offset = p - extra;
    p = write_str(p, extra + 255, tags);
    // file and file_offset
    node->file_offset = p - extra;
    p = write_str(p, extra + 255, shorten_path(file));
    // msg and msg_offset
    node->msg_offset = p - extra;
    p = write_str(p, extra_end, msg);

    // send out through udp
    return uvx_udp_send_to_addr(&xlog->xudp, &xlog->target_addr.addr, buf, p-buf);
}
