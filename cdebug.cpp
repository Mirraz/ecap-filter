#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ostream>
#include <libecap/common/autoconf.h>
#include <libecap/common/registry.h>
#include <libecap/common/log.h>
#include <libecap/host/host.h>
#include "cdebug.h"
#include "Debug.h"

int cdebug_printf(cdebug_lvmask_type lvmask, const char *format, ...) {
	va_list args;

	va_start(args, format);
	int str_length = vsnprintf(NULL, 0, format, args);
	va_end(args);

	if (str_length < 0) return str_length;
	char *str = (char *)malloc(str_length+1);
	if (str == NULL) return -1;

	va_start(args, format);
	int vsnprintf_res = vsnprintf(str, str_length+1, format, args);
	va_end(args);

	if (vsnprintf_res != str_length) {
		free(str);
		return (vsnprintf_res < 0 ? vsnprintf_res : -1);
	}
	Debug(lvmask) << str;
	free(str);
	return vsnprintf_res;
}
