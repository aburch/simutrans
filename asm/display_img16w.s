	#################################################
	# display_img16.s                               #
	#                                               #
	# Image decoding routine for Simutrans          #
	#                                               #
	# Hansjörg Malthaner, Aug. 2004                 #
	#################################################

.Lfe28:
	# .size	 display_img_wc,.Lfe28-display_img_wc
	.align 4
	# .type	 display_img_nc,@function
	.globl display_img_nc
	.globl _display_img_nc
display_img_nc:
_display_img_nc:
	subl $28,%esp
	pushl %ebp
	pushl %edi
	movl 40(%esp),%eax
	movl _images,%edi
	leal (%eax,%eax,8),%eax
	pushl %esi
	leal 0(,%eax,4),%esi

	# Check if height > 0
	movb 3(%edi,%esi),%cl
	pushl %ebx
	testb %cl,%cl
	je .L783

	movl _textur,%ebx
	xorl %eax,%eax
	movb 2(%edi,%esi),%al
	movl 52(%esp),%edx
	addl 56(%esp),%eax
	leal (%ebx,%edx,2),%edx
	imull _disp_width,%eax
	leal (%edx,%eax,2),%ebp
	movl _disp_width,%esi
	movl 60(%esp),%ebx
	addl %esi,%esi
	xorl %edx,%edx

	.p2align 4,,7
.L796:
	movw (%ebx),%dx
	addl $2,%ebx
	decb %cl
	movl %ebp,%edi

	# Decode one line
	.p2align 4,,7
.L795:
	# Clear run
	leal (%edi,%edx,2),%edi

	# Number of colored pixels
	movw (%ebx),%dx
	addl $2,%ebx

	# copy uneven number of words?
	testb $1,%dl
	je .L790

	# Copy first word
	movw (%ebx),%ax
	addl $2,%ebx
	movw %ax,(%edi)
	addl $2,%edi
	decl %edx

	# copy remaining doublewords
.L790:
	testb %dl, %dl
	je .L789

	# doubleword copy loop
	.p2align 4,,7
.L793:
	movl (%ebx),%eax
	addl $4,%ebx
	movl %eax,(%edi)
	addl $4,%edi
	subl $2,%edx
	jne .L793

.L789:
	movw (%ebx),%dx
	addl $2,%ebx
	testb %dl,%dl
	jne .L795

	# One line finished
	addl %esi,%ebp
	testb %cl,%cl
	jne .L796
.L783:
	popl %ebx
	popl %esi
	popl %edi
	popl %ebp
	addl $28,%esp
	ret
