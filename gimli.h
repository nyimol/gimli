#ifndef _GIMLI_H
#define _GIMLI_H 1

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <ruby.h>
#include "smallelfparser.h"

#define VERBOSE_LOG 0

#define PROC_SELF_EXE "/proc/self/exe"
#define MMAPSIZE 1024*1024*10
#define LOG(msg) fprintf(stderr, "[%s => %d]: %s\n", __func__, __LINE__, msg);

#define VERBOSE(msg) \
    if (VERBOSE_LOG) \
        LOG(msg)

#ifdef ElfW
    #undef ElfW
#endif

#ifdef LINUX
    #ifdef x86_64
        #define ElfW(type) Elf64_##type
        #define ELF_R_SYM ELF64_R_SYM
    #else
        #error "BY THE TIME NOW, GIMLI ONLY SUPPORT X86_64"
        #define ElfW(type) Elf32_##type
        #define ELF_R_SYM ELF32_R_SYM
    #endif
#else
    #error "BY THE TIME NOW, GIMLI ONLY SUPPORT LINUX"
#endif /* LINUX */

#define HOOKARRAY "@@hookArray"
#define ORGINALADDR "OrginalAddr"
#define DELEGATECLASS "DelegateClass"
#define FUNCNAME "FunctionName"
#define MAXHOOKNUM 65535
#define HOOKMETHOD "hook"

#define GETMETHOD(reg)\
VALUE getR##reg (VALUE self) {\
    puts ("noooooooo fuuuuuuuuuuuuuuuuucking waaay");\
    return Qnil;\
    struct GPRegs *gpr;\
    gpr = (struct GPRegs*)NUM2LONG (rb_iv_get(self, "@gpraddr"));\
    return LONG2NUM ( gpr->r##reg );\
}

typedef VALUE (method) (...);

struct GPRegs {
    size_t
        rax,
        rbx,
        rcx,
        rdx,
        rsi,
        rdi,
        rbp,
        rsp,
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15;
};

off_t changeGOTEntry (char*, off_t);
off_t calcPLTAddr (size_t);



#endif /* _GIMLI_H */
