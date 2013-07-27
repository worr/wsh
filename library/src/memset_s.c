#include "memset_s.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

int memset_s(void* v, size_t smax, int c, size_t n) {
	if (v == NULL) return EINVAL;
	if (smax > SIZE_MAX) return EINVAL;
	if (n > smax) return EINVAL;

	volatile unsigned char *p = v;
	while (smax-- && n--) *p++ = c;

	return 0;
}

