
# mp1.S - missile-command version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct missile, 
    # struct missile is defined in rtc.h and mp1.h

	NEXT 	  = 0
	X	  = 4
	Y	  = 8
	VX	  = 12
	VY	  = 16
	DEST_X	  = 20
	DEST_Y	  = 24
	EXPLODED  = 28
	C	  = 32

    # Character to draw for an explosion - '@'
	EXPLOSION = 64

	# my data 
	ROW_MUL_OFFSET = 160		# 2*80
	COL_MUL_OFFSET = 2

# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl mp1_missile_list, base_alive, mp1_score

mp1_missile_list:	.long 0x0	# Head of list pointer
base_alive:		.long 0x0	# Array of 3 bytes, plus a padding byte
mp1_score:		.long 0x0	# Player's current score


# Data private to this  file

base_pic:     	    .string "/^^^\\"	# Picture of a live base
dead_base_pic: 	    .string "xxxxx"	# Picture of a dead base
crosshairs_x:		.long 0x0	# X-position of the crosshairs
crosshairs_y:		.long 0x0	# Y-position of the crosshairs


from_ptr_of_getstatus:	.long 0x0 # a pointer for getstatus function, store the value we want to copy to user space
notify_user:		.long 0x0	# 0: not notify; else: notify
.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long garbage);
# Performs three tasks:
#	(1) updates the list of missiles (implement this in update_missiles,
#           below, and call it from here).
#	(2) Redraw the bases - they may have been overwritten with missiles
#	(3) Redraw the crosshairs - it may have been overwritten with missiles
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:

		# callee saved registers
		pushl %ebp
  		movl %esp, %ebp
		pushl %ebx
  		pushl %esi
  		pushl %edi

		# call update_missiles
			# caller saved 
		pushl %edx
		pushl %ecx
		pushl %eax 
		
		movl $0, notify_user 	# initialize notify_user to 0
		call update_missiles
			# restore caller saved
		popl %eax
		popl %ecx
		popl %edx

		# call redraw_bases
			# caller saved
		pushl %edx
		pushl %ecx
		pushl %eax

		call redraw_bases
			# restore caller saved
		popl %eax
		popl %ecx
		popl %edx

		# call redraw_crosshairs
			# caller saved
		pushl %edx
		pushl %ecx
		pushl %eax

		call redraw_crosshairs
			# restore caller saved
		popl %eax
		popl %ecx
		popl %edx

		# restore callee saved registers
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret


# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention
.globl mp1_ioctl
mp1_ioctl:
		# check cmd; eax = cmd
		movl 8(%esp), %eax
		# check: cmd < 0 or cmd > 4
		cmpl $0, %eax
		jl INVALID_CMD
		cmpl $4, %eax
		jg INVALID_CMD
		# jump to the corresponding function
		jmp *jump_table(,%eax,4)  				

	INVALID_CMD:
		# return -1
		movl $-1, %eax
		ret


# ----------------- Functions private to this file -------------------

# helper function: mp1_missile_remove
# Removes a missile from the linked list of missiles.
# Inputs   : long - a pointer to a missile (edx)
#          : long - a pointer to the previous missile (esi)
# Outputs  : none
# Registers: Standard C calling convention (but registers eax ebx ecx edx esi edi are unchanged)
mp1_missile_remove:
		# callee saved registers
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%eax			
		pushl	%ebx
		pushl	%ecx
		pushl	%edx
		pushl	%esi
		pushl	%edi
		# edi = cur_ptr
		movl 8(%ebp), %edi
		# esi = prev_ptr
		movl 12(%ebp), %esi
		# ebx = edi->next
		movl NEXT(%edi), %ebx
		pushl %ebx
			# erase the missile
			# get x,y
		movl X(%edi), %ecx
		shrl $16, %ecx
		movl Y(%edi), %eax
		shrl $16, %eax
			# call helper_draw(y, x, ' ')
		pushl $' '
		pushl %ecx
		pushl %eax
		call helper_draw    
		add $12, %esp
		popl %ebx

		movl mp1_missile_list, %eax
		cmpl %edi, %eax
		je REMOVE_HEAD
		# update ptr : esi->next = edi->next
		movl %ebx, NEXT(%esi)
		jmp REMVOE_IS_DONE
	REMOVE_HEAD:
		# update ptr : mp1_missile_list = edi->next
		movl %ebx, mp1_missile_list

	REMVOE_IS_DONE:
		pushl %ebx
			# free the struct point by edi
		pushl %edi
		call mp1_free
		addl $4, %esp
		popl %ebx
		# restore callee saved registers
		popl	%edi
		popl	%esi
		popl	%edx
		popl	%ecx
		popl	%ebx
		popl	%eax			
		leave
		ret
# update_missiles
# helper function for mp1_rtc_tasklet, loop through the linked list of missiles and check each missile
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
update_missiles:
		# callee saved registers
		pushl %ebp
  		movl %esp, %ebp
		pushl %ebx
  		pushl %esi
  		pushl %edi
	# maintain value in the registers
		# edi = cur_ptr
		# esi = prev_ptr
		# ecx = new x
		# edx = new y
		# eax = old x
		# ebx = old y
		movl mp1_missile_list, %edi
		movl %edi, %esi
	# loop through linked list
LOOP_FOR_MISSILE_LIST:
		# head==edi?
		movl mp1_missile_list, %eax
		cmpl %eax, %edi
		jne NOT_HEAD_CHECK
		movl %edi, %esi		# edi = esi =head
	NOT_HEAD_CHECK:
		# cur_ptr = null?
		cmpl $0, %edi
		je END_LOOP_FOR_MISSILE_LIST
		#
		movl X(%edi), %eax
		movl Y(%edi), %ebx

		# check: exploding
		cmpl $0, EXPLODED(%edi)
		jne EXPLOSION_EXIST
		jmp UPDATE_LOCATIONS
		
	EXPLOSION_EXIST:
		# not change value of X Y in this part
		# caller-save and maintain ptr value
		pushl %edi
		call missile_explode
		popl %edi
		addl %eax, notify_user
			# dec exploded field
		decl EXPLODED(%edi)
			# check: exploded field == 0 and remove	missile
		cmpl $0, EXPLODED(%edi)
		jne DRAW_EXPLOSION
		jmp REMOVE_MISSILE

	DRAW_EXPLOSION:
		# call helper_draw
		movl X(%edi), %eax
		shrl $16, %eax
		movl Y(%edi), %ebx
		shrl $16, %ebx
		pushl $EXPLOSION
		pushl %eax
		pushl %ebx
		call helper_draw    # helper_draw(y ,x ,@)
		add $12, %esp
		# drawing finish, check next missile in the linked list
		jmp PREPARE_LOOP_FOR_MISSILE_LIST


	UPDATE_LOCATIONS:
	# maintain value in ecx, edx
		movl X(%edi), %eax
		movl Y(%edi), %ebx
		# update x
		# ecx = X+VX = new X
		movl VX(%edi), %ecx
		addl %eax, %ecx
		# update y
		# edx = Y+VY = new Y
		movl VY(%edi), %edx
		addl %ebx, %edx

		# now, do not maintain value in eax, ebx

		# check: within bounds  x[0, 79]   y[0, 24] 
			# check x; only compare high 16 bits
		cmpl $0x04FFFFF, %ecx		# 0x4F=64+15=79
		jg REMOVE_MISSILE
		cmpl $0, %ecx
		jl REMOVE_MISSILE
			# check y; only compare high 16 bits
		cmpl $0x18FFFF, %edx
		jg REMOVE_MISSILE
		cmpl $0, %edx
		jl REMOVE_MISSILE

		# check if the missile reach the destination
		movl DEST_X(%edi), %eax
		movl %ecx, %ebx
		shrl $16, %ebx
		cmpl %eax, %ebx
		jne DRAW_NEW_MISSILE
		movl DEST_Y(%edi), %eax
		movl %edx, %ebx
		shrl $16, %ebx
		cmpl %eax, %ebx
		jne DRAW_NEW_MISSILE
		# missile just reach destination
			# both x,y equal; go to EXPLOSION_EXIST
		# erase the missile
		# eax = old x
		# ebx = old y
		movl X(%edi), %eax
		shrl $16, %eax
		movl Y(%edi), %ebx
		shrl $16, %ebx
			# update x,y
		movl %ecx, X(%edi)
		movl %edx, Y(%edi)	

		# since the missile reach dest, just put DEST_X and DEST_Y to X, Y
		movl DEST_X(%edi), %ecx
		movl DEST_Y(%edi), %edx
		shll $16, %ecx
		shll $16, %edx
		movl %ecx, X(%edi)
		movl %edx, Y(%edi)
		# erase previous missile
		# draw explosion after we jump to EXPLOSION EXIST
		pushl $' '
		pushl %eax
		pushl %ebx
		call helper_draw    # helper_draw(old y, old x,' ')
		add $12, %esp
		jmp EXPLOSION_EXIST
	# -------------------------------------
	DRAW_NEW_MISSILE:
		# erase 
			# erase the missile
			# eax = old x
			# ebx = old y
		movl X(%edi), %eax
		shrl $16, %eax
		movl Y(%edi), %ebx
		shrl $16, %ebx
			# update x,y
		movl %ecx, X(%edi)
		movl %edx, Y(%edi)
		shrl $16, %ecx
		shrl $16, %edx
			# now, eax, ebx contains old valid x,y
			# ecx, edx contains new valid x,y 

			# call helper_draw
		# caller-save 
		pushl %edx
		pushl %ecx
		pushl %edi
		pushl %esi
		pushl %ebx
		pushl %eax

		pushl $' '
		pushl %eax
		pushl %ebx
		call helper_draw    # helper_draw(y ,x ,' ')
		add $12, %esp
		# restore caller-save
		popl %eax
		popl %ebx
		popl %esi
		popl %edi
		popl %ecx
		popl %edx	

		# draw
# caller-save and maintain ptr value
		pushl %edx
		pushl %ecx
		pushl %edi
		pushl %esi
		pushl %ebx
		pushl %eax
			# call helper_draw
		pushl C(%edi)
		pushl %ecx
		pushl %edx
		call helper_draw    # y ,x , C
		add $12, %esp
		# restore caller-save
		popl %eax
		popl %ebx
		popl %esi
		popl %edi
		popl %ecx
		popl %edx	
		jmp PREPARE_LOOP_FOR_MISSILE_LIST

	REMOVE_MISSILE:
# caller-save and maintain ptr value
		pushl %edx
		pushl %ecx
		pushl NEXT(%edi)
			# call func
		pushl %esi
		pushl %edi
		call mp1_missile_remove  
		add $8, %esp
			# update cur_ptr
		popl %edi
		# restore caller-save
		popl %ecx
		popl %edx	
		jmp LOOP_FOR_MISSILE_LIST

	PREPARE_LOOP_FOR_MISSILE_LIST:
		# esi= edi; edi = edi->next
		movl %edi, %esi
		cmpl $0, %esi
		je END_LOOP_FOR_MISSILE_LIST
		movl NEXT(%esi), %edi
		jmp LOOP_FOR_MISSILE_LIST

END_LOOP_FOR_MISSILE_LIST:
		# check if notify user
		cmpl $0, notify_user
		je DO_NOT_CALL_NOTIFY_USER
		call mp1_notify_user
	DO_NOT_CALL_NOTIFY_USER:
		# restore callee saved registers
		popl %EDI
  		popl %ESI
  		popl %EBX
		leave
		ret
# redraw_bases
# helper function for mp1_rtc_tasklet, redraw the three bases
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
redraw_bases:
		# callee saved registers
		pushl %ebp
  		movl %esp, %ebp
		pushl %ebx
  		pushl %esi
  		pushl %edi
	# edi store prt to pic
		lea base_pic, %edi
	# esi = base_alive
		movl base_alive, %esi
		# draw base 1 (18,24)
			# chekc base 1 alive?
		andl $0xFF, %esi
		cmpl $0, %esi
		jne DRAW_BASE1
		lea dead_base_pic, %edi
	DRAW_BASE1:
	# ebx counter = 5
	# ecx = x value
		movl $5, %ebx
		movl $18, %ecx
	LOOP_FOR_DRAW_BASE1:
		cmpl $0, %ebx
		je CHECK_BASE2_DRAW_BASE2

		push (%edi)		# ASCII
		push %ecx		# x
		push $24		# y
		call helper_draw
		add $12, %esp
		# update after drawing
		incl %ecx
		decl %ebx
		incl %edi
		jmp LOOP_FOR_DRAW_BASE1
	CHECK_BASE2_DRAW_BASE2:
		# edi store prt to pic
		lea base_pic, %edi
		# esi = base_alive
		movl base_alive, %esi
		# draw base 1 (38,24)
			# chekc base 1 alive?
		andl $0xFF00, %esi
		cmpl $0, %esi
		jne DRAW_BASE2
		lea dead_base_pic, %edi
	DRAW_BASE2:
		# ebx counter = 5
		# ecx = x value
		movl $5, %ebx
		movl $38, %ecx
	LOOP_FOR_DRAW_BASE2:
		cmpl $0, %ebx
		je CHECK_BASE3_DRAW_BASE3

		push (%edi)
		push %ecx
		push $24
		call helper_draw
		add $12, %esp
		# update after drawing
		incl %ecx
		decl %ebx
		incl %edi
		jmp LOOP_FOR_DRAW_BASE2
		# draw base 3 (58,24)

	CHECK_BASE3_DRAW_BASE3:
		# edi store prt to pic
		lea base_pic, %edi
		# esi = base_alive
		movl base_alive, %esi
		# draw base 1 (58,24)
			# chekc base 1 alive?
		andl $0xFF0000, %esi
		cmpl $0, %esi
		jne DRAW_BASE3
		lea dead_base_pic, %edi
	DRAW_BASE3:
		# ebx counter = 5
		# ecx = x value
		movl $5, %ebx
		movl $58, %ecx
	LOOP_FOR_DRAW_BASE3:
		cmpl $0, %ebx
		je DRAW_BASE_DONE

		push (%edi)
		push %ecx
		push $24
		call helper_draw
		add $12, %esp
		# update after drawing
		incl %ecx
		decl %ebx
		incl %edi
		jmp LOOP_FOR_DRAW_BASE3

	DRAW_BASE_DONE:
		# restore callee saved registers
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret

# redraw_crosshairs
# helper function for mp1_rtc_tasklet, redraw the crosshairs
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
 redraw_crosshairs:
		# callee saved registers
		pushl %ebp
  		movl %esp, %ebp
		pushl %ebx
  		pushl %esi
  		pushl %edi
		#
		movl crosshairs_x, %ecx
		movl crosshairs_y, %eax
		# redraw crosshairs
		pushl $'+'
		pushl %ecx
		pushl %eax
		call helper_draw
		addl $12, %esp
		# restore callee saved registers
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret
# mp1_ioctl_startgame
# Initialize all of the variables used by the driver
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
mp1_ioctl_startgame:
	# callee saved registers
 		pushl %ebp 					
		movl %esp, %ebp 	
		pushl %ebx
  		pushl %esi
  		pushl %edi
 # set crosshairs_x and crosshairs_y to (40,12)
		movl $40, crosshairs_x
		movl $12, crosshairs_y
		# set initial value
		movl $0, mp1_score
		movl $0x00FFFFFF, base_alive			
		movl $0, mp1_missile_list
			# callee saved registers
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret
# mp1_ioctl_addmissile
# Copy the user’s missile into a dynamically allocated buffer in kernel space
# Inputs   : a pointer to a struct missile in user space
# Outputs  : -1 if fails; 0 if success
# Registers: Standard C calling convention
mp1_ioctl_addmissile:
	# callee saved registers
 		pushl %ebp 					
		movl %esp, %ebp 	
		pushl %ebx
  		pushl %esi
  		pushl %edi
			# size of struct missile
		push $36  			
		call mp1_malloc
		# allocate space in kernel
		addl $4, %esp
		cmpl $0, %eax
		je FAIL_NEW_MISSILE
			# edi = ptr to struct missile in kernel
		movl %eax, %edi
		# enough space in kernel
		push $36  			# size of struct missile	
		push 8(%ebp)  		# from (user space)
		push %edi 			# to   (kernel space)
		call mp1_copy_from_user
		addl $12, %esp    # 3 parameters
		cmpl $0, %eax
		jne CLEAN_SPACE_ALLOCATED
		# copy successful: add the new missile to the linked list
		movl mp1_missile_list, %edx
		movl %edx, NEXT(%edi)
		movl %edi, mp1_missile_list
		movl $0, %eax
		jmp END_FOR_ADD_MISSILE

		CLEAN_SPACE_ALLOCATED:
		# free the space allocated
		push %edi
		call mp1_free
		pop %edi
		jmp FAIL_NEW_MISSILE

		FAIL_NEW_MISSILE:
		movl $-1, %eax
		jmp END_FOR_ADD_MISSILE

 		END_FOR_ADD_MISSILE:
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret

# mp1_ioctl_movexhairs
# This function moves the crosshairs within boundary
# Inputs   :  a 32-bit integer; low 16 bits contain the amount by which the x component of the crosshair position should change; high 16 bits contain the amount by which the y component should change
# Outputs  : 0
# Registers: Standard C calling convention
mp1_ioctl_movexhairs:
	# callee saved registers
 		pushl %ebp 					
		movl %esp, %ebp 	
		pushl %ebx
  		pushl %esi
  		pushl %edi
			# get movement parameter
		movl 8(%ebp), %esi		
		movl 8(%ebp), %ebx
        # lower 16 bits - x; higher 16 bits - y
		andl $0x0000ffff, %ebx	
		sarl $16, %esi		
        # signed extension so movement of x is correct
		movswl %bx, %ebx		
            # ecx = old y; edx = old x
		movl crosshairs_y, %ecx	
		movl crosshairs_x, %edx 	
            # esi = new y; ebx = new x
		addl %ecx, %esi		
		addl %edx, %ebx		

        # y>0?
        cmpl $0, %esi
        jge CHECK_Y_UPPER_BOUND
        movl $0, %esi   
    CHECK_Y_UPPER_BOUND:
        cmpl $24, %esi
        jle CHECK_X_UPPER_BOUND
        movl $24, %esi 
    CHECK_X_UPPER_BOUND:
        cmpl $79, %ebx
        jle CHECK_X_LOWER_BOUND
        movl $79, %ebx          
    CHECK_X_LOWER_BOUND:
        cmpl $0, %ebx
        jge VALID_MOVEXHAIRS
        xorl %ebx, %ebx        
	VALID_MOVEXHAIRS:
        # update crosshairs
		movl %esi, crosshairs_y	
		movl %ebx, crosshairs_x
			# erase: call helper_draw(y,x,' ')
		pushl $' '
		pushl %edx
		pushl %ecx
		call helper_draw		
		addl $12, %esp
			# draw new crosshairs: call helper_draw(y,x,'+')
		pushl $'+'
		pushl %ebx
		pushl %esi
		call helper_draw		
		addl $12, %esp
			# return 0
		movl $0, %eax		
			# restore callee saved registers
		popl %edi
		popl %esi
		popl %ebx			
		leave
		ret
# mp1_ioctl_getstatus
# This function allows the user to retrieve the current score and the status of the three cities
# Inputs   :   The argument is a pointer to a 32-bit integer in user space
# Outputs  : 0 success; -1 fail
# Registers: Standard C calling convention
mp1_ioctl_getstatus:
	# callee saved registers
 		pushl %ebp 					
		movl %esp, %ebp 	
		pushl %ebx
  		pushl %esi
  		pushl %edi
		# mp1_score only 16 bits
		movl mp1_score, %edi
		andl $0xFFFF, %edi
		movl %edi, mp1_score
        # edi = mp1_score
        # check bases alive
            # first base: 0bit->16bit
        movl base_alive, %esi
        andl $0xFF, %esi
		cmpl $0, %esi
		je CHECK_BASE2
            # first base alive, set 16bit to 1
		orl $0x10000, %edi
	CHECK_BASE2:
			# second base: 8bit->17bit
		movl base_alive, %esi
		andl $0xFF00, %esi
		cmpl $0, %esi
		je CHECK_BASE3
		orl $0x20000, %edi
	CHECK_BASE3:
			# third base: 16bit->18bit
		movl base_alive, %esi
		andl $0xFF0000, %esi
		cmpl $0, %esi
		je THREE_BASES_CHECK_DONE
		orl $0x40000, %edi
	THREE_BASES_CHECK_DONE:
        # final value in edi
        movl %edi, from_ptr_of_getstatus
            # prepare parameters
        # eax = to ptr
        movl 8(%ebp), %eax
		# lea from_ptr_of_getstatus, %esi
		# caller save
			# store mp1 score in ecx to prevent modification
		movl mp1_score, %ecx
		pushl %eax
		pushl %ecx
		pushl %edx
		
		pushl $4
		pushl $from_ptr_of_getstatus
		pushl %eax
		call mp1_copy_to_user
		add $12, %esp
		# restore callee saved registers
		popl %edx
		popl %ecx
		popl %eax
			# restore mp1 score1
		movl %ecx, mp1_score

		cmpl $0, %eax
		je FAIL_GETSTATUS
		movl $0, %eax
		jmp END_GETSTATUS
	FAIL_GETSTATUS:
		movl $-1, %eax
	END_GETSTATUS:
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret
# mp1_ioctl_endgame
# free all the memory being used by the linked list and then return success
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
mp1_ioctl_endgame:
	# callee saved registers
 		pushl %ebp 					
		movl %esp, %ebp 	
		pushl %ebx
  		pushl %esi
  		pushl %edi
		# esi = ptr of current missile
		# edi = ptr of next missile
		movl mp1_missile_list, %esi
	LOOP_FOR_CLEAN_UP:
		# ptr null?
		cmpl $0, %esi
		je END_LOOP_FOR_CLEAN_UP
		# free the struct
		movl NEXT(%esi), %edi
        push %esi
		call mp1_free
        addl $4, %esp
        movl %edi, %esi
		jmp LOOP_FOR_CLEAN_UP

	END_LOOP_FOR_CLEAN_UP:
		movl $1, %eax
		popl %edi
  		popl %esi
  		popl %ebx
		leave
		ret

# helper_draw
# helper function for drawing
# Inputs: 	value of y on the screen [0,79]
#			value of x on the screen [0,24]
#			ASCII value of the character to draw
# Outputs  : none
# Registers: not change eax, ebx, ecx, edx, esi, edi
helper_draw:					
		pushl %ebp
		movl  %esp, %ebp
		pushl %ebx
  		pushl %esi
  		pushl %edi

        # save ecx 
        pushl %ecx
        pushl %eax
		pushl %edx

		movl	8(%ebp), %esi		# esi = y
		movl	12(%ebp), %ebx		# ebx = x
		movl	16(%ebp), %ecx		# ecx = ASCII

		imull $ROW_MUL_OFFSET, %esi		# esi = 160*y
        imull $COL_MUL_OFFSET, %ebx     # ebx = 2*x
        movl %esi, %eax
        add %ebx, %eax                  # eax = 160*y + 2*x
		call	mp1_poke
		# restore registers
		popl %edx
        popl %eax
        popl %ecx
		popl %edi
		popl %esi
		popl %ebx
		leave
		ret

# jump table for mp1_ioctl
jump_table:			
.long mp1_ioctl_startgame, mp1_ioctl_addmissile, mp1_ioctl_movexhairs, mp1_ioctl_getstatus, mp1_ioctl_endgame


