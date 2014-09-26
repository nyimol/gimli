#include "gimli.h"

static void* fakeplt;
VALUE hookArray = Qnil;

VALUE Gimli_addHook (VALUE self, VALUE functionName, VALUE delegateClass) {
    size_t index = RARRAY_LEN(hookArray);
    if (index < (MAXHOOKNUM - 1)) {
        Check_Type (functionName, T_STRING);
        Check_Type (delegateClass, T_OBJECT);
        VALUE hookDetailes = rb_hash_new ();
        off_t fakePLTaddress = calcPLTAddr (index);
        off_t orginalAddress = changeGOTEntry (RSTRING_PTR (functionName), fakePLTaddress);
        rb_hash_aset (hookDetailes, rb_str_new2 (ORGINALADDR), LONG2NUM (orginalAddress));
        rb_hash_aset (hookDetailes, rb_str_new2 (FUNCNAME), functionName);
        rb_hash_aset (hookDetailes, rb_str_new2 (DELEGATECLASS), delegateClass);
        rb_ary_store (hookArray, index, hookDetailes);
    }
    return Qnil;
}

VALUE Gimli_init (VALUE self) {
    if (hookArray == Qnil) {
        hookArray = rb_ary_new ();
    }
    rb_iv_set (self, HOOKARRAY, hookArray);
    return self;
}

size_t callRubyHook (struct GPRegs *regs, size_t index) {
    VALUE hash = rb_ary_entry (hookArray, index);
    VALUE cklass = rb_hash_aref (hash, rb_str_new2 (DELEGATECLASS));

    rb_iv_set (cklass, "@rax", LONG2NUM(regs->rax));
    rb_iv_set (cklass, "@rbx", LONG2NUM(regs->rbx));
    rb_iv_set (cklass, "@rcx", LONG2NUM(regs->rcx));
    rb_iv_set (cklass, "@rdx", LONG2NUM(regs->rdx));
    rb_iv_set (cklass, "@rsi", LONG2NUM(regs->rsi));
    rb_iv_set (cklass, "@rdi", LONG2NUM(regs->rdi));
    rb_iv_set (cklass, "@rsp", LONG2NUM(regs->rsp));
    rb_iv_set (cklass, "@rbp", LONG2NUM(regs->rbp));
    rb_iv_set (cklass, "@r8" , LONG2NUM(regs->r8 ));
    rb_iv_set (cklass, "@r9" , LONG2NUM(regs->r9 ));
    rb_iv_set (cklass, "@r10", LONG2NUM(regs->r10));
    rb_iv_set (cklass, "@r11", LONG2NUM(regs->r11));
    rb_iv_set (cklass, "@r12", LONG2NUM(regs->r12));
    rb_iv_set (cklass, "@r13", LONG2NUM(regs->r13));
    rb_iv_set (cklass, "@r14", LONG2NUM(regs->r14));
    rb_iv_set (cklass, "@r15", LONG2NUM(regs->r15));

    VALUE result = rb_funcall (cklass, rb_intern (HOOKMETHOD), 0);

    // ALL the breaks are fucking useless :-)
    switch (TYPE(result)) {
        case T_STRING:
            return (size_t)RSTRING_PTR (result);
            break;
        case T_FIXNUM:
            return FIX2LONG (result);
            break;
        case T_BIGNUM:
            return NUM2LONG (result);
            break;
        case T_TRUE:
            return 1;
            break;
        default :
            return 0;
            break;
    }
}

off_t changeGOTEntry (char* symbol, off_t address) {
    if (symbol == NULL) return NULL;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    ElfW(Rela)* rela;
    SmallELFParser *elfParser = new SmallELFParser;
    void* ptr;
    link_map* linkMap = elfParser->getFirstLinkMapEntry ();
    off_t orginalAddress = 0;
    int index = elfParser->getSymbolIndex (symbol);
    if (index != -1) {
        rela = elfParser->getRelaForIndex (index);
        ptr = (void*)rela->r_offset;
        mprotect((void *)(((size_t)ptr) & (((size_t)-1) ^ (pagesize - 1))),
            pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);
        memcpy (&orginalAddress, ptr, sizeof(size_t));
        memcpy (ptr, &address, sizeof(size_t));
    }
    delete elfParser;
    while (1) {
        if (strlen (linkMap->l_name) > 0 &&
            strstr (linkMap->l_name, "gimli.so") == NULL) {
            elfParser = new SmallELFParser;
            elfParser->openFile (linkMap->l_name);
            index = elfParser->getSymbolIndex (symbol);
            if (index != -1) {
                rela = elfParser->getRelaForIndex (index);
                if (rela) {
                    ptr = (void*)rela->r_offset;
                    mprotect((void *)(((size_t)ptr) & (((size_t)-1) ^ (pagesize - 1))),
                        pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);
                    ptr = (void*)((size_t)ptr + linkMap->l_addr);
                    if (!orginalAddress) {
                        memcpy (&orginalAddress, ptr, sizeof(size_t));
                    }
                    memcpy (ptr, &address, sizeof(size_t));
                }
            }
            delete elfParser;
        }
        if (!linkMap->l_next) { break; }
        linkMap = linkMap->l_next;
    }
    return orginalAddress;
}

//I missed asm-blocks so fucking much
void* __attribute__ ((naked)) dispatcher () {
    asm("popq %rax;"
        "push %rbp;"
        "movq %rsp, %rbp;"
        "sub $0x80, %rsp;"
        "mov %rax, 0x00(%rsp);"
        "mov %rbx, 0x08(%rsp);"
        "mov %rcx, 0x10(%rsp);"
        "mov %rdx, 0x18(%rsp);"
        "mov %rsi, 0x20(%rsp);"
        "mov %rdi, 0x28(%rsp);"
        "mov %rbp, 0x30(%rsp);"
        "mov %rsp, 0x38(%rsp);"
        "mov %r8 , 0x40(%rsp);"
        "mov %r9 , 0x48(%rsp);"
        "mov %r10, 0x50(%rsp);"
        "mov %r11, 0x58(%rsp);"
        "mov %r12, 0x60(%rsp);"
        "mov %r13, 0x68(%rsp);"
        "mov %r14, 0x70(%rsp);"
        "mov %r15, 0x78(%rsp);"
        "mov %rax, %rsi;"
        "mov %rsp, %rdi;");

        asm ("callq *%0": :"r"(callRubyHook));
        
        asm("add $0x80, %rsp;"
            "pop %rbp;"
            "ret;");
}

off_t calcPLTAddr (size_t index) {
    return (index * 11) + 8 + (size_t)fakeplt;
}

void writeFakePLTEntry (size_t index) {
    off_t offset = calcPLTAddr (index);
    memcpy ((void*)offset, "\x68", 1); //push opcode
    offset++;
    memcpy ((void*)offset, &index, 4);
    offset += 4;
    memcpy ((void*)offset, "\xff\x25", 2); //jmp *displacement(%rip) opcode
    offset += 2;
    size_t displacement = offset + 4;
    displacement = (size_t)fakeplt - displacement;
    memcpy ((void*)offset, &displacement, 4);
}

int __attribute__((constructor)) damnFunc () {
    fakeplt = mmap (NULL, MMAPSIZE,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (fakeplt == MAP_FAILED) {
        exit(1);
    }

    size_t dispatcherFuncAddr = (size_t)&dispatcher;
    memcpy (fakeplt, &dispatcherFuncAddr, sizeof(size_t));
    writeFakePLTEntry (0);
    ruby_init();
    ruby_init_loadpath();
    VALUE rb_cKlass = rb_define_class("Gimli", rb_cObject);
    rb_define_method(rb_cKlass, "addHook", (method*)Gimli_addHook, 2);
    rb_define_method(rb_cKlass, "initialize", (method*)Gimli_init, 0);
    
    rb_require ("~/hook");

    int counter;
    for (counter = 0; counter < MAXHOOKNUM; counter++) {
        writeFakePLTEntry (counter);
    }
    return 0;
}
