#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
#endif

#include "automem.h"

// author: bywayboy

void automem_init(automem_t* pmem, unsigned int initBufferSize)
{
	pmem->size = 0;
	pmem->buffersize = (initBufferSize == 0 ? 128 : initBufferSize);
#if defined(_WIN32) || defined(_WIN64)
	pmem->pdata = (unsigned char*) HeapAlloc(GetProcessHeap(),0,pmem->buffersize);
#else
	pmem->pdata = (unsigned char*) malloc(pmem->buffersize);
#endif
	
}

void automem_uninit(automem_t* pmem)
{
	if(NULL != pmem->pdata)
	{
		pmem->size = pmem->buffersize = 0;
#if defined(_WIN32) || defined(_WIN64)
		HeapFree(GetProcessHeap(),0,pmem->pdata);
#else
		free(pmem->pdata);
#endif
		pmem->pdata = NULL;
	}
}
/*清理并重新初始化*/
void automem_clean(automem_t* pmen)
{
	automem_uninit(pmen);
	automem_init(pmen,128);
}

void automem_reset(automem_t* pmem)
{
	pmem->size = 0;
}

void automem_ensure_newspace(automem_t* pmem, unsigned int len)
{
	unsigned int newbuffersize = pmem->buffersize;
	while(newbuffersize - pmem->size < len)
	{
		newbuffersize *= 2;
	}
	if(newbuffersize > pmem->buffersize)
	{
#if defined(_WIN32) || defined(_WIN64)
		pmem->pdata = (unsigned char*)HeapReAlloc(GetProcessHeap(),0,pmem->pdata,newbuffersize);
#else
		pmem->pdata = (unsigned char*) realloc(pmem->pdata, newbuffersize);
#endif
		pmem->buffersize = newbuffersize;
	}
}

/* 绑定到一块内存空间 */
void automem_attach(automem_t* pmem, void* pdata, unsigned int len)
{
	if(len > pmem->buffersize)
		automem_ensure_newspace(pmem, len - pmem->buffersize);
	pmem->size = len;
	memcpy(pmem->pdata, pdata, len);
}

//after automem_detach(), pmem is not available utill next automem_init()
//returned value must be free(), if not NULL

/* 获取数据指针和长度 */

void* automem_detach(automem_t* pmem, unsigned int* plen)
{
	void* pdata = (pmem->size == 0 ? NULL : pmem->pdata);
	if(plen) *plen = pmem->size;
	return pdata;
}


int automem_append_voidp(automem_t* pmem, const void* p, unsigned int len)
{
	automem_ensure_newspace(pmem, len);
	memcpy(pmem->pdata + pmem->size, p, len);
	pmem->size += len;
	return len;
}

int automem_erase(automem_t * pmem, unsigned int size)
{
	if(size < pmem->size)
	{
		memmove(pmem->pdata, pmem->pdata + size, pmem->size - size);
		pmem->size -= size;
	}else{
		pmem->size = 0;
	}
	return pmem->size;
}

int automem_erase_ex(automem_t* pmem, unsigned int size,unsigned int limit)
{

	if(size < pmem->size)
	{
		unsigned int newsize = pmem->size - size;

		if(pmem->buffersize > (newsize + limit))
		{
#if defined(_WIN32) || defined(_WIN64)
			char * buffer = (char*) HeapAlloc(GetProcessHeap(), 0, newsize + limit);
			memcpy(buffer, pmem->pdata + size, newsize);
			HeapFree(GetProcessHeap(),0,pmem->pdata);
#else
			char * buffer = (unsigned char*) malloc(newsize + limit);
			memcpy(buffer, pmem->pdata + size, newsize);
			free(pmem->pdata);
#endif
			pmem->pdata = (unsigned char*)buffer;
			pmem->size = newsize;
			pmem->buffersize = newsize + limit;
		}	
		else
		{
			memmove(pmem->pdata, pmem->pdata + size, newsize);
			pmem->size = newsize;
		}
	}
	else
	{
		pmem->size = 0;
		if(pmem->buffersize > limit)
		{
			automem_uninit(pmem);
			automem_init(pmem, limit);
		}
	}
	return pmem->size;
	
}

void automem_append_int(automem_t* pmem, int n)
{
	automem_append_voidp(pmem, &n, sizeof(int));
}
void automem_append_pchar(automem_t* pmem, char* n)
{
	automem_append_voidp(pmem, &n, sizeof(char*));
}

void automem_append_char(automem_t* pmem, char c)
{
	automem_append_voidp(pmem, &c, sizeof(char));
}
void automem_append_byte(automem_t* pmem, unsigned char c)
{
	automem_append_voidp(pmem, &c, sizeof(unsigned char));
}

/* 此宏定义中的函数 只在 中心服务器中才被使用. */
#if defined(_NEW_CENTER_SERVEER)



int automem_append_field_int(automem_t* pmem, const char * field, unsigned int f_len, int val)
{
	char intVal[20];int slen;

	automem_append_voidp(pmem, field, f_len);
	_itoa(val, intVal,10);
	slen = strlen(intVal);

	automem_ensure_newspace(pmem, slen + 4);
	pmem->pdata[pmem->size++]='=';
	pmem->pdata[pmem->size++]='"';
	automem_append_voidp(pmem, intVal, slen);
	pmem->pdata[pmem->size++]='"';
	pmem->pdata[pmem->size++]=' ';
	return pmem->size;
}

int automem_append_field_ulong(automem_t* pmem, const char * field, unsigned int f_len, unsigned long val)
{
	char intVal[32];int slen;

	automem_append_voidp(pmem, field, f_len);
	_ultoa(val, intVal,10);
	slen = strlen(intVal);

	automem_ensure_newspace(pmem, slen + 4);
	pmem->pdata[pmem->size++]='=';
	pmem->pdata[pmem->size++]='"';
	automem_append_voidp(pmem, intVal, slen);
	pmem->pdata[pmem->size++]='"';
	pmem->pdata[pmem->size++]=' ';

	return pmem->size;
}

int automem_append_field_fast(automem_t* pmem, const char * field, unsigned int f_len, const char * val,unsigned int v_len)
{
	if(NULL == val) return pmem->size;
	if(0 == v_len) v_len = strlen(val);
	if(0 == v_len) return pmem->size;

	automem_append_voidp(pmem, field, f_len);
	automem_ensure_newspace(pmem, v_len + 5);


	pmem->pdata[pmem->size++]='=';
	pmem->pdata[pmem->size++]='"';
	automem_append_voidp(pmem, val, v_len);
	pmem->pdata[pmem->size++]='"';
	pmem->pdata[pmem->size++]=' ';

	return pmem->size;
}
int automem_append_field(automem_t* pmem, const char * field, unsigned int f_len, const char * val)
{
	const char * p = val;
	char c;
	unsigned int t  = 4, sz = 0;
	if(NULL == val)return pmem->size;
	while(c = *p++)
	{
		switch(c)
		{
		case '&': // 5 &amp;
			t+=5;break;
		case '<': // 4 &lt;
			t+=4;break;
		case '>': // 4 &gt;
			t+=4;break;
		case '"': // 6 &quot;
			t+=6;break;
		case '\'': // 6 &apos;
			t+=6;break;
		default:
			t++;
		}
	}
	automem_append_voidp(pmem, field, f_len);
	automem_ensure_newspace(pmem, t);

	sz = pmem->size;
	pmem->pdata[sz++]='=';
	pmem->pdata[sz++]='"';
	p = val;

	while(c = *p++)
	{
		switch(c)
		{
		case '&': // 5 &amp;
			*(int*)(pmem->pdata+sz) = *(int*)"&amp";
			sz+=sizeof(int);
			pmem->pdata[sz++]=';';
			break;
		case '<': // 4 &lt;
			*(int*)(pmem->pdata+sz) = *(int*)"&lt;";
			sz+=sizeof(int);
			break;
		case '>': // 4 &gt;
			*(int*)(pmem->pdata+sz) = *(int*)"&gt;";
			sz+=sizeof(int);
			break;
		case '"': // 6 &quot;
			*(int*)(pmem->pdata+sz) = *(int*)"&quo";
			sz+=sizeof(int);
			*(short*)(pmem->pdata+sz) = *(short*)"t;";
			sz+=sizeof(short);
			break;
		case '\'': // 6 &apos;
			*(int*)(pmem->pdata+sz) = *(int*)"&apo";
			sz+=sizeof(int);
			*(short*)(pmem->pdata+sz) = *(short*)"s;";
			sz+=sizeof(short);
			break;
		default:
			pmem->pdata[sz++]=c;
		}
	}
	*(short*)(pmem->pdata+sz) = *(short*)"\" ";
	sz+=sizeof(short);
	pmem->size = sz;

	return sz;
}

#endif