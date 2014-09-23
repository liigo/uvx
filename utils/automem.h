#ifndef __AUTOMEM_H
#define __AUTOMEM_H

// author: bywayboy

#ifdef __cplusplus
extern "C" {
#endif

struct automem_s
{
	unsigned int size;
	unsigned int buffersize;
	unsigned char* pdata;
};
typedef struct automem_s automem_t;


void automem_init(automem_t* pmem, unsigned int initBufferSize);
void automem_uninit(automem_t* pmem);
void automem_clean(automem_t* pmen);
void automem_ensure_newspace(automem_t* pmem, unsigned int len);
void automem_attach(automem_t* pmem, void* pdata, unsigned int len);
void* automem_detach(automem_t* pmem, unsigned int* plen);
int automem_append_voidp(automem_t* pmem, const void* p, unsigned int len);
void automem_append_int(automem_t* pmem, int n);

void automem_append_char(automem_t* pmem, char c);
void automem_append_pchar(automem_t* pmem, char* n);
void automem_append_byte(automem_t* pmem, unsigned char c);
//快速重置
void automem_reset(automem_t* pmem);

//移除前面部分数据
int automem_erase(automem_t* pmem, unsigned int size);
//移除前面的数据 并且检查剩余空间是否超过 limit 超过则清理
int automem_erase_ex(automem_t* pmem, unsigned int size,unsigned int limit);

#if defined(_NEW_CENTER_SERVEER)
int automem_append_field_int(automem_t* pmem, const char * field, unsigned int f_len, int val);
int automem_append_field_ulong(automem_t* pmem, const char * field, unsigned int f_len, unsigned long val);
int automem_append_field_fast(automem_t* pmem, const char * field, unsigned int f_len, const char * val,unsigned int v_len);
int automem_append_field(automem_t* pmem, const char * field, unsigned int f_len, const char * val);

#endif

#ifdef __cplusplus
}
#endif

#endif