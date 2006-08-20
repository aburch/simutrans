	#################################################
	# pixcopy.s                                     #
	#                                               #
	# pixel copy routine for Simutrans              #
	#                                               #
	# Hansjörg Malthaner, Feb. 2001                 #
	#################################################


.text
	.align 32
.globl pixcopy
.globl _pixcopy
#	.type	 pixcopy,@function
pixcopy:
_pixcopy:
	pushl %edi
	pushl %esi
	movl 12(%esp),%edi	# dest
	movl 16(%esp),%esi      # src
	movl 20(%esp),%ecx	# runlen

	cld
	rep
	movsb

        popl %esi
	popl %edi
	ret
.Lfe1:
#	.size	 pixcopy,.Lfe1-pixcopy
