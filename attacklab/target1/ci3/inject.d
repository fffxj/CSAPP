
inject.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 b8 35 39 62 39 39 	movabs $0x6166373939623935,%rax
   7:	37 66 61 
   a:	48 89 04 24          	mov    %rax,(%rsp)
   e:	c6 44 24 08 00       	movb   $0x0,0x8(%rsp)
  13:	48 89 e7             	mov    %rsp,%rdi
  16:	68 fa 18 40 00       	pushq  $0x4018fa
  1b:	c3                   	retq   
