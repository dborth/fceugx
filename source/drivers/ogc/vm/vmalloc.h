/****************************************************************************
 * FCE Ultra GX
 *
 * emu_kidid 2015
 *
 * vmalloc.h
 *
 * GC VM memory allocator
 ***************************************************************************/

#ifdef HW_DOL

#ifndef _VMMANAGER_H_
#define _VMMANAGER_H_

void* vm_malloc(u32 size);
bool vm_free(void *ptr);
int vm_size_free();
#endif

#endif
