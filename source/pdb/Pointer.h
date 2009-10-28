
#ifndef POINTER_DEFINED
#define POINTER_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

int Pointer_equals_(void *p1, void *p2);
unsigned int Pointer_hash1(void *p);
unsigned int Pointer_hash2(void *p);

#ifdef __cplusplus
}
#endif
#endif
