        movabsq $0x6166373939623935,%rax
        movq %rax,(%rsp)
        movb $0x00,8(%rsp)
        movq %rsp,%rdi
        pushq $0x4018fa
        ret
