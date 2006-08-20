	#################################################
	# display_img.s                                 #
	#                                               #
	# Image decoding routine for Simutrans          #
	#                                               #
	# Hansjörg Malthaner, Feb. 2001                 #
	#################################################

	.text
	.align 32
.globl decode_img
.globl _decode_img
#	.type	 decode_img,@function
decode_img:
_decode_img:
	pushl %ebp
	pushl %edi
	pushl %esi
	movl 16(%esp),%edi      # tp
	movl 20(%esp),%esi      # sp
	movl 24(%esp),%ebp      # height
        movl 28(%esp),%edx      # disp_width
        addl $-64, %edx		# disp_width -= 64
        xorl %ecx, %ecx
	xorl %eax, %eax

	.p2align 5
.next_line:
	movb (%esi),%al
	incl %esi
	decl %ebp
	.p2align 2
.next_run:
	movb (%esi),%cl         # get run length
	incl %esi
	addl %eax,%edi          # skip transparent pixels
	testb %cl,%cl           # empty run ?
	je .done

        cld			# copy byte run
	shr $1, %cl
	jnc .odd
	movsb
	jz .end
.odd:
        rep
	movsw

.end:
	movb (%esi),%al         # prepare for next run
	incl %esi
	testb %al,%al           # done ?
	jne .next_run
.done:
	addl %edx,%edi		# tp += disp_width
	testl %ebp,%ebp
	jne .next_line
	popl %esi
	popl %edi
	popl %ebp
	ret
.Lfe1:
#	.size	 decode_img,.Lfe1-decode_img
