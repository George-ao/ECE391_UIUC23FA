#define ASM

#define HANDLE_LINK(name, func)            \
.GLOBL name               		;\
name:   						\
		ADD 		$-4, %ESP   ;\
		PUSHL		%EAX		;\
								\
		PUSHL		%FS         ;\
		PUSHL		%ES         ;\
		PUSHL		%DS         ;\
		PUSHL		%EAX        ;\
		PUSHL		%EBP        ;\
		PUSHL		%EDI        ;\
		PUSHL		%ESI        ;\
		PUSHL		%EDX        ;\
		PUSHL		%ECX        ;\
								\
	    CALL  		func      	;\
		CALL 		sig_handler ;\
								\
		POPL		%ECX        ;\
		POPL		%EDX        ;\
		POPL		%ESI        ;\
		POPL		%EDI        ;\
		POPL		%EBP        ;\
		ADDL 		$4, %ESP    ;\
		POPL		%DS         ;\
		POPL		%ES         ;\
		POPL		%FS         ;\
								\
		ADDL		$4, %ESP    ;\
		ADDL 		$4, %ESP    ;\
								\
	    IRET


.GLOBL sys_call_linkage
sys_call_linkage:
		CMPL	$0x00, %EAX
		JLE		error_num
		CMPL	$0x0D, %EAX				
		JG		error_num

		ADDL 	$-4, %ESP		# push dummy data for Error code
		PUSHL   %EAX

		PUSHL   %FS				# save ALL
		PUSHL   %ES
		PUSHL   %DS
		PUSHL   %EAX
		PUSHL   %EBP
		PUSHL   %EDI
		PUSHL   %ESI
		PUSHL   %EDX
		PUSHL   %ECX

		PUSHL	%EDX			# parameters
		PUSHL	%ECX
		PUSHL	%EBX
		CALL 	*jump_table(, %EAX, 4)		
		ADDL	$12, %ESP

		POPL	%ECX			# restore ALL
		POPL	%EDX
		POPL	%ESI
		POPL	%EDI
		POPL	%EBP
		ADDL 	$4, %ESP		# do not pop EAX
		POPL	%DS
		POPL	%ES
		POPL	%FS

		ADDL 	$4, %ESP		# IRQ
		ADDL 	$4, %ESP		# pop dummy data for Error code
		JMP 	end_call

error_num:
		MOVL	$-1, %EAX

end_call:
		IRET

jump_table:	
		.long  0 # make sure that the numbers are correct
		.long  halt
		.long  execute
		.long  read
		.long  write
		.long  open
		.long  close
		.long  getargs
		.long  vidmap
		.long  set_handler
		.long  sigreturn
		.long  malloc
		.long  free
		.long  ioctl


HANDLE_LINK(division_error_linkage, division_error_handler);
HANDLE_LINK(debug_linkage, debug_handler);
HANDLE_LINK(nmi_linkage, nmi_handler);
HANDLE_LINK(breakpoint_linkage, breakpoint_handler);
HANDLE_LINK(overflow_linkage, overflow_handler);
HANDLE_LINK(bound_range_exceeded_linkage, bound_range_exceeded_handler);
HANDLE_LINK(invalid_opcode_linkage, invalid_opcode_handler);
HANDLE_LINK(device_not_avail_linkage, device_not_avail_handler);
HANDLE_LINK(double_fault_linkage, double_fault_handler);
HANDLE_LINK(coprocessor_seg_overrun_linkage, coprocessor_seg_overrun_handler);
HANDLE_LINK(invalid_task_state_seg_linkage, invalid_task_state_seg_handler);
HANDLE_LINK(seg_not_present_linkage, seg_not_present_handler);
HANDLE_LINK(stack_seg_linkage, stack_seg_handler);
HANDLE_LINK(general_protection_linkage, general_protection_handler);
HANDLE_LINK(page_fault_linkage, page_fault_handler);
HANDLE_LINK(reserved_linkage, reserved_handler);
HANDLE_LINK(floating_point_linkage, floating_point_handler);
HANDLE_LINK(alignment_check_linkage, alignment_check_handler);
HANDLE_LINK(machine_check_linkage, machine_check_handler);
HANDLE_LINK(simd_floating_point_linkage, simd_floating_point_handler);
HANDLE_LINK(PIT_linkage, PIT_handler);
HANDLE_LINK(keyboard_linkage, keyboard_handler);
HANDLE_LINK(rtc_linkage, rtc_handler);
