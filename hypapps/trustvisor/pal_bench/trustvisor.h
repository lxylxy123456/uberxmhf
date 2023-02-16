#if TV_H == 64
#include "trustvisor64.h"
#elif TV_H == 32
#include "trustvisor32.h"
#else
#error TV_H should be 64 or 32
#endif
