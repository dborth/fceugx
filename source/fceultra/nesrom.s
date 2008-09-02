.rodata
.globl nesrom
nesromsize: .long 1048592 
.balign 32
nesrom:
.incbin "../source/rom/ROM.NES"

