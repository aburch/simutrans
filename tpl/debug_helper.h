#ifndef tpl_debug_helper_h
#define tpl_debug_helper_h

#ifdef simdebug_h

#define MESSAGE dbg->message

#else

#undef ERROR

#define ERROR out_error
#define WARNING out_warning
#define MESSAGE out_message

void out_error(  const char *who, const char *format, ...);
void out_warning(const char *who, const char *format, ...);
void out_message(const char *who, const char *format, ...);
void trap(void);

#endif

#endif
