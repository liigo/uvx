#ifndef LIIGO_LOGE_HEADER
#define LIIGO_LOGE_HEADER

// Loge: a logging protocol focus on serializing log items.
// Author: Liigo <com.liigo@gmail.com>, 201412.
// https://github.com/liigo/loge

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

// predefined logging levels
#define LOGE_LOG_ALL    ((int8_t) -128)
#define LOGE_LOG_TRACE  ((int8_t) -60)
#define LOGE_LOG_DEBUG  ((int8_t) -30)
#define LOGE_LOG_INFO   ((int8_t) 0)
#define LOGE_LOG_WARN   ((int8_t) 30)
#define LOGE_LOG_ERROR  ((int8_t) 60)
#define LOGE_LOG_FATAL  ((int8_t) 90)
#define LOGE_LOG_NONE   ((int8_t) 127)

typedef struct loge_t {
    char name[16]; // the log's name
    int enabled;   // enabled or not, 1: enabled, 0: disabled
    int pid;       // the process id
    int name_len;  // the length of name
} loge_t;

// initialize a `loge_t`
void loge_init(loge_t* loge, const char* name);

// Create a logging item and serialize it into bytes stream, ready to be sent.
// Parameters:
//   `buf`: a pre-allocated output buffer where the new logging item is serialized into;
//   `bufsize`: the size of `buf` in bytes, recommend `LOGE_MAXBUF`, can be smaller or larger;
//   `level`: the logging level, may be one of `LOGE_LOG_*`;
//   `tags`: comma separated text;
//   `msg`:  logging content text;
//   `file` and `line`: the source file path+name and line number;
//   `tags`/`msg`/`file`: can be NULL, and may be truncated if too long;
//   All text parameters should be utf-8 encoded, at least utf-8 compatible.
// Returns the serialized data size in bytes, which is ensure not exceed `bufsize` and `LOGE_MAXBUF`.
// Maybe returns 0, which means nothing was serialized (e.g. when the logging is disabled).
// Example:
//   char buf[1024];
//   unsigned int len = loge_item(loge, buf, sizeof(buf), ...);
unsigned int
loge_item(loge_t* loge, void* buf, unsigned int bufsize,
          int level, const char* tags, const char* msg, const char* file, int line);

// the binary version of `loge_item`, `msg` is a binary data instead of a text.
// returns 0 if the msg's size is too long.
// see `loge_item` for more details.
unsigned int
loge_item_bin(loge_t* loge, void* buf, unsigned int bufsize, int level, const char* tags,
              const void* msg, unsigned int msglen, const char* file, int line);

// To enable (if enabled==1) or disable (if enabled==0) the logging.
void loge_enable(loge_t* loge, int enabled);

// To get (if name==NULL) or set (if name!=NULL) the logging's name.
const char* loge_name(loge_t* loge, const char* name);

// `loge_item_t` defines memory layout of a logging item's serialized data.
// The size of this struct and its extra data block is guaranted
// not exceed 1024 bytes (LOGE_MAXBUF) by default.
// TODO: endian?(prefer little-endian) and align?
typedef struct loge_item_t {
    uint8_t  version;        // the loge protocol version, is `1` currently.
    uint8_t  magic1, magic2; // must always be `0x4c` and `0xaf`
    int8_t   level;          // LOGE_LOG_*
    int32_t  time, pid, tid, line; // all in little-endian
    // offsets inside extra data block
    uint8_t  name_offset, tags_offset, file_offset, msg_offset;
    uint16_t msg_len;      // length of msg in bytes, not including ending '\0'.
    uint8_t  extra_offset; // offset of extra data block inside loge_item_t
    uint8_t  flags;        // resvered for future use cases, must be 0 now.
    // the "extra data block" stores texts, its address can be calculated as:
    //   loge_item_t* log = ...;
    //   const char* extra = (const char*)log + log->extra_offset;
    // and name/tags/file/msg can be read from extra:
    //   const char* name = extra + log->name_offset;
    //   const char* tags = extra + log->tags_offset;
    //   const char* file = extra + log->file_offset;
    //   const char* msg  = extra + log->msg_offset;
} loge_item_t;

// Create and serialize a logging item, but not send it.
// This is a wrapper macro to `loge_item()` and `snprintf()`, provides more conveniency.
// `__FILE__` and `__LINE__` are automaticly serialized into this logging item.
// `bufsize` will be rewrited to the serialized size in bytes, so it must be a variable.
// Example:
//   char buf[1024]; unsigned int len = sizeof(buf);
//   LOGE_ITEM(&loge, buf, len, LOGE_MAXBUF, "author", "name: %s, sex: %d", "Liigo", 1);
#define LOGE_ITEM(loge,buf,bufsize,level,tags,msgfmt,...) {\
        char loge_tmp_msg_[LOGE_MAXBUF]; /* avoid name conflict with outer-scope names */ \
        snprintf(loge_tmp_msg_, sizeof(_uvx_tmp_msg_), msgfmt, __VA_ARGS__);\
        bufsize = loge_item(loge, buf, bufsize, level, tags, loge_tmp_msg_, __FILE__, __LINE__);\
    }

// The max size of single logging item's serialized data, see `loge_item()`.
// You can change it to any N manually where sizeof(loge_item_t) < N <= 1452,
// the most prudent N is not exceed 528, for safe transmission through
// internet (ipv4/ipv6 + UDP), without warry about packet fragmentation.
#define LOGE_MAXBUF 1024

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LIIGO_LOGE_HEADER
