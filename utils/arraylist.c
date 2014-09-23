/*
 * $Id: arraylist.c,v 1.4 2006/01/26 02:16:28 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "bits.h"
#include "arraylist.h"

struct array_list*
array_list_new(array_list_free_fn *free_fn)
{
  struct array_list *arr;

  arr = (struct array_list*)calloc(1, sizeof(struct array_list));
  if(!arr) return NULL;
  arr->size = ARRAY_LIST_DEFAULT_SIZE;
  arr->length = 0;
  arr->free_fn = free_fn;
  if(!(arr->array = (void**)calloc(sizeof(void*), arr->size))) {
    free(arr);
    return NULL;
  }
  return arr;
}

extern void
array_list_free(struct array_list *arr)
{
  int i;
  for(i = 0; i < arr->length; i++)
    if(arr->array[i] && NULL != arr->free_fn) arr->free_fn(arr->array[i]);
  free(arr->array);
  free(arr);
}

void*
array_list_get_idx(struct array_list *arr, int i)
{
  if(i >= arr->length) return NULL;
  return arr->array[i];
}
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
static int array_list_expand_internal(struct array_list *arr, int mx)
{
  void *t;
  int new_size;

  if(mx < arr->size) return 0;
  new_size = max(arr->size << 1, mx);
  if(!(t = realloc(arr->array, new_size*sizeof(void*)))) return -1;
  arr->array = (void**)t;
  (void)memset(arr->array + arr->size, 0, (new_size-arr->size)*sizeof(void*));
  arr->size = new_size;
  return 0;
}

int
array_list_put_idx(struct array_list *arr, int idx, void *data)
{
  if(array_list_expand_internal(arr, idx)) return -1;
  if(arr->array[idx]) arr->free_fn(arr->array[idx]);
  arr->array[idx] = data;
  if(arr->length <= idx) arr->length = idx + 1;
  return 0;
}

int
array_list_add(struct array_list *arr, void *data)
{
  return array_list_put_idx(arr, arr->length, data);
}

int
array_list_length(struct array_list *arr)
{
  return arr->length;
}


static void check_and_free_array_memary(struct array_list * arr){
	void *t;
	int new_size;
	if(arr->size <=ARRAY_LIST_DEFAULT_SIZE || arr->size == arr->length) return;	//为空 或者刚刚好。直接返回
	
	new_size = arr->size >> 1;
	if(arr->length >= new_size) return;
	
	if(!(t = realloc(arr->array, new_size*sizeof(void*)))) return;
	arr->array = (void**)t;
	(void)memset(arr->array + arr->length, 0, (new_size-arr->length)*sizeof(void*));
	arr->size = new_size;
}

int array_list_del_idx( struct array_list * arr, int idx)
{
	int i;
	
	if( idx >= arr->length || idx < 0)
		return 0;
	
	
	arr->free_fn(arr->array[idx]);
	arr->length--;

	for( i=idx; i<arr->length; i++ )
		arr->array[i]  = arr->array[i+1];
	arr->array[arr->length]=NULL;

	check_and_free_array_memary(arr);
	return 1;
};
