
#include "Pointer.h"
#include <stdint.h>
#include "Hash_murmur.h"
#include "Hash_superfast.h"

int Pointer_equals_(void *p1, void *p2)
{
	return (uintptr_t)p1 == (uintptr_t)p2;
	/*
	uintptr_t i1 = (uintptr_t)p1;
	uintptr_t i2 = (uintptr_t)p2;
	if(i1 == i2) return 0;
	if(i1 < i2) return 1;
	return -1;
	*/
}

unsigned int Pointer_hash1(void *p)
{
	return MurmurHash2((const void *)&p, (int)sizeof(void *), 0) | 0x1; // odd
}

unsigned int Pointer_hash2(void *p)
{
	return SuperFastHash((const char *)&p, (int)sizeof(void *)) << 1; // even
}

