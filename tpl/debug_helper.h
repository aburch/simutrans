#ifndef tpl_debug_helper_h
#define tpl_debug_helper_h

#ifdef simdebug_h

#define ERROR dbg->error
#define WARNING dbg->warning
#define MESSAGE dbg->message

#else

#ifdef __cplusplus
extern "C" {
#else
	#undef ERROR
#endif

	#define ERROR out_error
	#define WARNING out_warning
	#define MESSAGE out_message

	extern void out_error(const char *who, const char *format, ...);
	extern void out_warning(const char *who, const char *format, ...);
	extern void out_message(const char *who, const char *format, ...);
  extern void trap(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
