
#include <mach/mach.h>

#include <mach-o/dyld.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// MARK: - CPU

extern void sys_icache_invalidate(void *start, size_t length);

struct dyld_interpose_tuple {
    const void* replacement;
    const void* replacee;
};

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif

void* sign_pc(void* ptr) {
#if __arm64e__
    return ptrauth_sign_unauthenticated(ptr, ptrauth_key_process_independent_code, 0x7481);
#else
    return ptr;
#endif
}

void* sign_data(void* ptr) {
#if __arm64e__
    return ptrauth_sign_unauthenticated(ptr, ptrauth_key_process_independent_data, 0);
#else
    return ptr;
#endif
}

void hexdump_ugh2(void *mem, unsigned int len)
{
        unsigned int i, j;
        
        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("0x%06x: ", i);
                }
 
                /* print hex data */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        printf("   ");
                }
                
                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        putchar(' ');
                                }
                                else if(isprint(((char*)mem)[j])) /* printable char */
                                {
                                        putchar(0xFF & ((char*)mem)[j]);
                                }
                                else /* other char */
                                {
                                        putchar('.');
                                }
                        }
                        putchar('\n');
                }
        }
}
extern const struct mach_header __dso_handle;
extern void dyld_dynamic_interpose(const struct mach_header* mh, const struct dyld_interpose_tuple array[], size_t count);

// MARK: - Libhooker

struct LHFunctionHook {
    void* function;
    void *replacement;
    void *oldptr;
    struct LHFunctionHookOptions *options;
};

enum LHOptions {
    LHOptionsNone,
    LHOptionsSetJumpReg
};

struct LHFunctionHookOptions {
    enum LHOptions options;
    int jmp_reg;
};

enum LIBHOOKER_ERR {
    LIBHOOKER_OK = 0,
    LIBHOOKER_ERR_SELECTOR_NOT_FOUND = 1,
    LIBHOOKER_ERR_SHORT_FUNC = 2,
    LIBHOOKER_ERR_BAD_INSN_AT_START = 3,
    LIBHOOKER_ERR_VM = 4,
    LIBHOOKER_ERR_NO_SYMBOL = 5
};

struct LHMemoryPatch {
    void *destination;
    const void *data;
    size_t size;
    void *options;
};

// MARK: - PAC

extern void* sign_pointer(void* ptr);
extern void* strip_pointer(void* ptr);
extern void* sign_pc(void* ptr);

// MARK: - The rest

extern const void* assembly(const unsigned char code[], size_t);

extern kern_return_t
mach_vm_allocate(mach_port_name_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags);

extern kern_return_t
mach_vm_protect(mach_port_name_t task, mach_vm_address_t address, mach_vm_size_t size, boolean_t set_maximum, vm_prot_t new_protection);

#pragma pack(4)
typedef struct {
  mach_msg_header_t Head;
  mach_msg_body_t msgh_body;
  mach_msg_port_descriptor_t thread;
  mach_msg_port_descriptor_t task;
  NDR_record_t NDR;
} exception_raise_request; // the bits we need at least

typedef struct {
  mach_msg_header_t Head;
  NDR_record_t NDR;
  kern_return_t RetCode;
} exception_raise_reply;
#pragma pack()

struct arm_thread_state64
{
#if __arm64e__
    __uint64_t __x[29];     /* General purpose registers x0-x28 */
    void*      __opaque_fp; /* Frame pointer x29 */
    void*      __opaque_lr; /* Link register x30 */
    void*      __opaque_sp; /* Stack pointer x31 */
    void*      __opaque_pc; /* Program counter */
    __uint32_t __cpsr;      /* Current program status register */
    __uint32_t __opaque_flags; /* Flags describing structure format */
#else
    __uint64_t __x[29]; /* General purpose registers x0-x28 */
    __uint64_t __fp;    /* Frame pointer x29 */
    __uint64_t __lr;    /* Link register x30 */
    __uint64_t __sp;    /* Stack pointer x31 */
    __uint64_t __pc;    /* Program counter */
    __uint32_t __cpsr;  /* Current program status register */
    __uint32_t __pad;   /* Same size for 32-bit or 64-bit clients */
#endif
};
