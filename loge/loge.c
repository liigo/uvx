#include "loge.h"
#include <time.h>
#include <assert.h>

// by Liigo, 201412.

#ifdef __unix__
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    #define gettid() syscall(SYS_gettid)
#endif

#define LOGE_MIN(a,b) ((a)<(b)?(a):(b))

void loge_init(loge_t* loge, const char* name) {
    assert(LOGE_MAXBUF > sizeof(loge_item_t) && "LOGE_MAXBUF is too small");
    loge_name(loge, name ? name : "loge");
    loge->enabled = 1;
    loge->pid = (int) getpid();
}

void loge_enable(loge_t* loge, int enabled) {
    loge->enabled = enabled;
}

const char* loge_name(loge_t* loge, const char* name) {
    if(name) {
        int n = strlen(name);
        n = LOGE_MIN(n, sizeof(loge->name) - 1);
        loge->name_len = n;
        assert(loge->name_len >= 0 && loge->name_len < sizeof(loge->name));
        memcpy(loge->name, name, n);
        loge->name[n] = '\0';
    }
    return loge->name;
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
        n = LOGE_MIN(n, buf_end - buf);
        memcpy(buf, str, n);
        n = rfind_utf8_leading_byte_index(buf, n);
        buf[n] = '\0';
        return buf + n + 1;
    }
}

unsigned int loge_item(loge_t* loge, void* buf, unsigned int bufsize,
                       int level, const char* tags, const char* msg, const char* file, int line) {
    loge_item_t* item = (loge_item_t*) buf;
    char* extra = (char*)buf + sizeof(loge_item_t);
    const char* extra_end = (char*)buf + LOGE_MIN(bufsize, LOGE_MAXBUF);
    char* p = extra;
    assert(bufsize > sizeof(loge_item_t) && "bufsize is too small");

    if(!loge->enabled) return 0;

    item->version = 1;    // the loge protocol version number, increase if `loge_item_t` changed.
    item->magic1  = 0x4c; // 0x4c/0xaf = 76/175 = 0.4343 = log(e)
    item->magic2  = 0xaf; // which means this is `loge` :)
    item->level   = (int8_t) level;

    item->time = (int32_t) time(NULL);
    item->pid = loge->pid;
    item->tid = (int) gettid();
    item->line = line;

    // fill in strings and it's offsets.
    // the length of first three strings (name/tags/file) can't exceed 255,
    // to make sure the last offset (msg_offset) is valid uint8_t value.

    // name and name_offset
    item->name_offset = 0;
    assert(sizeof(loge->name) < LOGE_MIN(255, extra_end - extra));
    memcpy(extra, loge->name, loge->name_len + 1);
    p += loge->name_len + 1;
    // tags and tags_offset
    item->tags_offset = p - extra;
    p = write_str(p, extra + 255, tags);
    // file and file_offset
    item->file_offset = p - extra;
    p = write_str(p, extra + 255, shorten_path(file));
    // msg and msg_offset
    item->msg_offset = p - extra;
    p = write_str(p, extra_end, msg);

    return (p - (char*)buf);
}
