        #################################################
	# color pixcopy.s                               #
	#                                               #
	# color pixel copy routine for Simutrans        #
	#                                               #
	# Hansjörg Malthaner, Feb. 2001                 #
	#################################################

.text
	.align 32
.globl colorpixcpy
.globl _colorpixcpy
#	.type	 colorpixcpy,@function
colorpixcpy:
_colorpixcpy:
	pushl %esi
	movl 8(%esp),%edx      	# dest
	movl 12(%esp),%ecx      # src
	movl 16(%esp),%esi      # src end address
	movb 20(%esp),%ah	# player color

# we don't need the empty run case in Simutrans
#	cmpl %esi,%ecx		# src < src_end ?
#	jae .L4

	.p2align 5
.L5:
	movb (%ecx),%al		# read pixel color
	incl %ecx
	cmpb $3,%al
	ja .L6
	orb %ah,%al
.L6:
	movb %al,(%edx)		# write color to dest
        incl %edx
	cmpl %esi,%ecx
	jb .L5
.L4:
	popl %esi
	ret
.Lfe1:
#	.size	 colorpixcpy,.Lfe1-colorpixcpy
