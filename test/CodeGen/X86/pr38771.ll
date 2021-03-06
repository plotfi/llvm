; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown | FileCheck %s

define void @function() nounwind {
; CHECK-LABEL: function:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    movabsq $281474976710656, %rax # imm = 0x1000000000000
; CHECK-NEXT:    notq %rax
; CHECK-NEXT:    movl $2147483647, %ecx # imm = 0x7FFFFFFF
; CHECK-NEXT:    shldq $65, %rax, %rcx
; CHECK-NEXT:    xorl %eax, %eax
; CHECK-NEXT:    movb $64, %dl
; CHECK-NEXT:    testb %dl, %dl
; CHECK-NEXT:    cmoveq %rcx, %rax
; CHECK-NEXT:    movq %rax, (%rax)
; CHECK-NEXT:    movl $0, (%rax)
; CHECK-NEXT:    retq
entry:
  %B68 = sub i96 39614081257132168796771975167, 281474976710656
  %B49 = or i96 39614081257132168796771975167, 39614081257132168796771975167
  %B33 = lshr i96 %B68, %B68
  store i96 %B33, i96* undef
  ret void
}
