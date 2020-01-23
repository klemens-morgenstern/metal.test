#include <metal/unit.def>

int __metal_status   = 1;
int __metal_critical = 0;
int __metal_errored = 0;

#if defined(__GNUC__)
#define METAL_NO_INLINE __attribute__ ((noinline))
#else
#define METAL_NO_INLINE
#endif

void METAL_NO_INLINE __metal_unit_break(__metal_level lvl,
               __metal_oper oper,
               int condition,
               int bitwise,
               const char* str1,
               const char* str2,
               const char* str3,
               const char* file,
               int line)
{
    if (__metal_level_assert == lvl)
        __metal_errored |= !condition;
        
    __metal_status = condition;
}

inline void __metal_call(void (*func)(), const char * msg,
			   const char * file, int line)
{
	__metal_unit_break(__metal_level_expect,
			  __metal_oper_enter_case,
			  1,
			  0,
			  msg,
			  0,
			  0,
			  file,
			  line);
	func();
	__metal_unit_break(__metal_level_expect,
			  __metal_oper_exit_case,
			  1,
			  0,
			  msg,
			  0,
			  0,
			  file,
			  line);
}

int METAL_NO_INLINE __metal_report()
{
	__metal_unit_break(__metal_level_expect,
		  __metal_oper_report,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0);
	return __metal_errored != 0;
}
