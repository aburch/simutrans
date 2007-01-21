#ifndef tpl_debug_helper_h
#define tpl_debug_helper_h

#ifdef simdebug_h

#ifdef __cplusplus
#define ERROR dbg->error
#define WARNING dbg->warning
#define MESSAGE dbg->message

#else
#define ERROR c_out_error
#define WARNING c_out_warning
#define MESSAGE c_out_message
#endif

#ifdef __cplusplus
extern "C" {
#endif
  extern void trap();
#ifdef __cplusplus
}
#endif
#else

#ifdef __cplusplus
extern "C" {
#endif

  extern void c_out_error(const char *who, const char *format, ...);
  extern void c_out_warning(const char *who, const char *format, ...);
  extern void c_out_message(const char *who, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

#endif
