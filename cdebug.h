#ifndef CDEBUG_H
#define CDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

enum cdebug_importance_level {
	CDEBUG_IL_DEBUG = 0,
	CDEBUG_IL_NORMAL = 1,
	CDEBUG_IL_CRITICAL = 2
};
enum cdebug_frequency_level {
	CDEBUG_FL_OPERATION = 0,
	CDEBUG_FL_XACTION = 1 << 4,
	CDEBUG_FL_APPLICATION = 2 << 4
};
enum cdebug_message_size_level {
	CDEBUG_MSL_NORMAL = 0,
	CDEBUG_MSL_LARGE = 1 << 8
};

typedef unsigned int cdebug_lvmask_type;

int cdebug_printf(cdebug_lvmask_type lvmask, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif/*CDEBUG_H*/
