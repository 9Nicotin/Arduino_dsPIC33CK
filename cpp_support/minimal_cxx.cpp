/*
 * minimal_cxx.cpp — Minimal C++ runtime support for dsPIC33CK
 *
 * Provides new/delete operators and pure virtual handler.
 * Include this file in the Arduino core build when C++ is enabled.
 *
 * Compile with: xc-dsc-g++ -fno-exceptions -fno-rtti -c minimal_cxx.cpp
 */

#include <stdlib.h>

/* Sized and unsized new/delete */
void* operator new(unsigned int size) { return malloc(size); }
void* operator new[](unsigned int size) { return malloc(size); }
void operator delete(void* ptr) { free(ptr); }
void operator delete[](void* ptr) { free(ptr); }
void operator delete(void* ptr, unsigned int) { free(ptr); }
void operator delete[](void* ptr, unsigned int) { free(ptr); }

extern "C" {

/* Called when a pure virtual function is invoked — trap forever */
void __cxa_pure_virtual(void)
{
    while (1);
}

/* Guard for static local variables (simplified — single-threaded) */
int __cxa_guard_acquire(int *guard)
{
    if (*guard) return 0;   /* Already initialized */
    return 1;               /* Need to initialize */
}

void __cxa_guard_release(int *guard)
{
    *guard = 1;             /* Mark as initialized */
}

void __cxa_guard_abort(int *guard)
{
    (void)guard;
}

} /* extern "C" */
