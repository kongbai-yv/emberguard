/* CMSIS GCC compiler header (minimal replacement) */
#ifndef __CMSIS_GCC_H
#define __CMSIS_GCC_H

#include <stdint.h>

#ifndef   __ASM
#define __ASM                                  __asm
#endif
#ifndef   __INLINE
#define __INLINE                               inline
#endif
#ifndef   __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE                   static inline __attribute__((always_inline))
#endif
#ifndef   __NO_RETURN
#define __NO_RETURN                            __attribute__((noreturn))
#endif
#ifndef   __USED
#define __USED                                 __attribute__((used))
#endif
#ifndef   __WEAK
#define __WEAK                                 __attribute__((weak))
#endif
#ifndef   __PACKED
#define __PACKED                               __attribute__((packed))
#endif
#ifndef   __PACKED_STRUCT
#define __PACKED_STRUCT                        struct __attribute__((packed))
#endif
#ifndef   __PACKED_UNION
#define __PACKED_UNION                         union __attribute__((packed))
#endif
#ifndef   __ALIGNED
#define __ALIGNED(x)                           __attribute__((aligned(x)))
#endif
#ifndef   __RESTRICT
#define __RESTRICT                             __restrict
#endif
#ifndef   __COMPILER_BARRIER
#define __COMPILER_BARRIER()                   __asm volatile ("" ::: "memory")
#endif

#ifndef   __UNALIGNED_UINT32
struct __attribute__((packed)) T_UINT32 { uint32_t v; };
#define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
#endif

#ifndef   __UNALIGNED_UINT16_WRITE
__PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
#define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void*)(addr))->v) = (val))
#endif

#ifndef   __UNALIGNED_UINT16_READ
__PACKED_STRUCT T_UINT16_READ { uint16_t v; };
#define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
#endif

#ifndef   __UNALIGNED_UINT32_WRITE
__PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
#define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
#endif

#ifndef   __UNALIGNED_UINT32_READ
__PACKED_STRUCT T_UINT32_READ { uint32_t v; };
#define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
#endif

#endif /* __CMSIS_GCC_H */
