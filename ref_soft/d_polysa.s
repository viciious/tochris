/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
//
// d_polysa.s
// x86 assembly-language polygon model drawing code
//

#include "../common/asm_i386.h"
#include "../common/quakeasm.h"
#include "asm_draw.h"
#include "d_ifacea.h"

#if	id386

// !!! if this is changed, it must be changed in d_polyse.c too !!!
#define DPS_MAXSPANS			MAXHEIGHT+1	
									// 1 extra for spanpackage that marks end

//#define	SPAN_SIZE	(((DPS_MAXSPANS + 1 + ((CACHE_SIZE - 1) / spanpackage_t_size)) + 1) * spanpackage_t_size)
#define SPAN_SIZE (1024+1+1+1)*32


	.data

	.align	4
p10_minus_p20:	.single		0
p01_minus_p21:	.single		0
temp0:			.single		0
temp1:			.single		0
Ltemp:			.single		0

aff8entryvec_table:	.long	LDraw8, LDraw7, LDraw6, LDraw5
				.long	LDraw4, LDraw3, LDraw2, LDraw1

lzistepx:		.long	0


	.text

.extern C(D_PolysetSetEdgeTable)
.extern C(D_RasterizeAliasPolySmooth)

//----------------------------------------------------------------------
// affine triangle gradient calculation code
//----------------------------------------------------------------------

#define skinwidth	4+0

.globl C(D_PolysetCalcGradients)
C(D_PolysetCalcGradients):

//	p00_minus_p20 = r_p0[0] - r_p2[0];
//	p01_minus_p21 = r_p0[1] - r_p2[1];
//	p10_minus_p20 = r_p1[0] - r_p2[0];
//	p11_minus_p21 = r_p1[1] - r_p2[1];
//
//	xstepdenominv = 1.0 / (p10_minus_p20 * p01_minus_p21 -
//			     p00_minus_p20 * p11_minus_p21);
//
//	ystepdenominv = -xstepdenominv;

	fildl	C(r_p0)+0		// r_p0[0]
	fildl	C(r_p2)+0		// r_p2[0] | r_p0[0]
	fildl	C(r_p0)+4		// r_p0[1] | r_p2[0] | r_p0[0]
	fildl	C(r_p2)+4		// r_p2[1] | r_p0[1] | r_p2[0] | r_p0[0]
	fildl	C(r_p1)+0		// r_p1[0] | r_p2[1] | r_p0[1] | r_p2[0] | r_p0[0]
	fildl	C(r_p1)+4		// r_p1[1] | r_p1[0] | r_p2[1] | r_p0[1] |
							//  r_p2[0] | r_p0[0]
	fxch	%st(3)			// r_p0[1] | r_p1[0] | r_p2[1] | r_p1[1] |
							//  r_p2[0] | r_p0[0]
	fsub	%st(2),%st(0)	// p01_minus_p21 | r_p1[0] | r_p2[1] | r_p1[1] |
							//  r_p2[0] | r_p0[0]
	fxch	%st(1)			// r_p1[0] | p01_minus_p21 | r_p2[1] | r_p1[1] |
							//  r_p2[0] | r_p0[0]
	fsub	%st(4),%st(0)	// p10_minus_p20 | p01_minus_p21 | r_p2[1] |
							//  r_p1[1] | r_p2[0] | r_p0[0]
	fxch	%st(5)			// r_p0[0] | p01_minus_p21 | r_p2[1] |
							//  r_p1[1] | r_p2[0] | p10_minus_p20
	fsubp	%st(0),%st(4)	// p01_minus_p21 | r_p2[1] | r_p1[1] |
							//  p00_minus_p20 | p10_minus_p20
	fxch	%st(2)			// r_p1[1] | r_p2[1] | p01_minus_p21 |
							//  p00_minus_p20 | p10_minus_p20
	fsubp	%st(0),%st(1)	// p11_minus_p21 | p01_minus_p21 |
							//  p00_minus_p20 | p10_minus_p20
	fxch	%st(1)			// p01_minus_p21 | p11_minus_p21 |
							//  p00_minus_p20 | p10_minus_p20
	flds	C(d_xdenom)		// d_xdenom | p01_minus_p21 | p11_minus_p21 |
							//  p00_minus_p20 | p10_minus_p20
	fxch	%st(4)			// p10_minus_p20 | p01_minus_p21 | p11_minus_p21 |
							//  p00_minus_p20 | d_xdenom
	fstps	p10_minus_p20	// p01_minus_p21 | p11_minus_p21 |
							//  p00_minus_p20 | d_xdenom
	fstps	p01_minus_p21	// p11_minus_p21 | p00_minus_p20 | xstepdenominv
	fxch	%st(2)			// xstepdenominv | p00_minus_p20 | p11_minus_p21

//// ceil () for light so positive steps are exaggerated, negative steps
//// diminished,  pushing us away from underflow toward overflow. Underflow is
//// very visible, overflow is very unlikely, because of ambient lighting
//	t0 = r_p0[4] - r_p2[4];
//	t1 = r_p1[4] - r_p2[4];

	fildl	C(r_p2)+16		// r_p2[4] | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fildl	C(r_p0)+16		// r_p0[4] | r_p2[4] | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fildl	C(r_p1)+16		// r_p1[4] | r_p0[4] | r_p2[4] | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// r_p2[4] | r_p0[4] | r_p1[4] | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// r_p2[4] | r_p2[4] | r_p0[4] | r_p1[4] |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fsubrp	%st(0),%st(2)	// r_p2[4] | t0 | r_p1[4] | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fsubrp	%st(0),%st(2)	// t0 | t1 | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21

//	r_lstepx = (int)
//			ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
//	r_lstepy = (int)
//			ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	fld		%st(0)			// t0 | t0 | t1 | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmul	%st(5),%st(0)	// t0*p11_minus_p21 | t0 | t1 | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// t1 | t0 | t0*p11_minus_p21 | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// t1 | t1 | t0 | t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmuls	p01_minus_p21	// t1*p01_minus_p21 | t1 | t0 | t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// t0 | t1 | t1*p01_minus_p21 | t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmuls	p10_minus_p20	// t0*p10_minus_p20 | t1 | t1*p01_minus_p21 |
							//  t0*p11_minus_p21 | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fxch	%st(1)			// t1 | t0*p10_minus_p20 | t1*p01_minus_p21 |
							//  t0*p11_minus_p21 | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fmul	%st(5),%st(0)	// t1*p00_minus_p20 | t0*p10_minus_p20 |
							//  t1*p01_minus_p21 | t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// t1*p01_minus_p21 | t0*p10_minus_p20 |
							//  t1*p00_minus_p20 | t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fsubp	%st(0),%st(3)	// t0*p10_minus_p20 | t1*p00_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fsubrp	%st(0),%st(1)	// t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fld		%st(2)			// xstepdenominv |
							//  t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmuls	float_minus_1	// ystepdenominv |
							//  t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmul	%st(3),%st(0)	// (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//   xstepdenominv |
							//  t1*p00_minus_p20 - t0*p10_minus_p20 |
							//   | ystepdenominv | xstepdenominv |
							//   p00_minus_p20 | p11_minus_p21
	fxch	%st(1)			// t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//   xstepdenominv | ystepdenominv |
							//   xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmul	%st(2),%st(0)	// (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//  ystepdenominv |
							//  (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//  xstepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fldcw	ceil_cw
	fistpl	C(r_lstepy)		// r_lstepx | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fistpl	C(r_lstepx)		// ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fldcw	single_cw

//	t0 = r_p0[2] - r_p2[2];
//	t1 = r_p1[2] - r_p2[2];

	fildl	C(r_p2)+8		// r_p2[2] | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fildl	C(r_p0)+8		// r_p0[2] | r_p2[2] | ystepdenominv |
							//   xstepdenominv | p00_minus_p20 | p11_minus_p21
	fildl	C(r_p1)+8		// r_p1[2] | r_p0[2] | r_p2[2] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// r_p2[2] | r_p0[2] | r_p1[2] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// r_p2[2] | r_p2[2] | r_p0[2] | r_p1[2] |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubrp	%st(0),%st(2)	// r_p2[2] | t0 | r_p1[2] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fsubrp	%st(0),%st(2)	// t0 | t1 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21

//	r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
//			xstepdenominv);
//	r_sstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
//			ystepdenominv);

	fld		%st(0)			// t0 | t0 | t1 | ystepdenominv | xstepdenominv
	fmul	%st(6),%st(0)	// t0*p11_minus_p21 | t0 | t1 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// t1 | t0 | t0*p11_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// t1 | t1 | t0 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmuls	p01_minus_p21	// t1*p01_minus_p21 | t1 | t0 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fxch	%st(2)			// t0 | t1 | t1*p01_minus_p21 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmuls	p10_minus_p20	// t0*p10_minus_p20 | t1 | t1*p01_minus_p21 |
							//  t0*p11_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(1)			// t1 | t0*p10_minus_p20 | t1*p01_minus_p21 |
							//  t0*p11_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmul	%st(6),%st(0)	// t1*p00_minus_p20 | t0*p10_minus_p20 |
							//  t1*p01_minus_p21 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fxch	%st(2)			// t1*p01_minus_p21 | t0*p10_minus_p20 |
							//  t1*p00_minus_p20 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubp	%st(0),%st(3)	// t0*p10_minus_p20 | t1*p00_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubrp	%st(0),%st(1)	// t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmul	%st(2),%st(0)	// (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//   ystepdenominv |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fxch	%st(1)			// t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//   ystepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmul	%st(3),%st(0)	// (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//  xstepdenominv |
							//  (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//  ystepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(1)			// (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//  ystepdenominv |
							//  (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//  xstepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fistpl	C(r_sstepy)		// r_sstepx | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fistpl	C(r_sstepx)		// ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21

//	t0 = r_p0[3] - r_p2[3];
//	t1 = r_p1[3] - r_p2[3];

	fildl	C(r_p2)+12		// r_p2[3] | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fildl	C(r_p0)+12		// r_p0[3] | r_p2[3] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fildl	C(r_p1)+12		// r_p1[3] | r_p0[3] | r_p2[3] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// r_p2[3] | r_p0[3] | r_p1[3] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// r_p2[3] | r_p2[3] | r_p0[3] | r_p1[3] |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubrp	%st(0),%st(2)	// r_p2[3] | t0 | r_p1[3] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fsubrp	%st(0),%st(2)	// t0 | t1 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21

//	r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
//			xstepdenominv);
//	r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
//			ystepdenominv);

	fld		%st(0)			// t0 | t0 | t1 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fmul	%st(6),%st(0)	// t0*p11_minus_p21 | t0 | t1 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// t1 | t0 | t0*p11_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// t1 | t1 | t0 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmuls	p01_minus_p21	// t1*p01_minus_p21 | t1 | t0 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fxch	%st(2)			// t0 | t1 | t1*p01_minus_p21 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmuls	p10_minus_p20	// t0*p10_minus_p20 | t1 | t1*p01_minus_p21 |
							//  t0*p11_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(1)			// t1 | t0*p10_minus_p20 | t1*p01_minus_p21 |
							//  t0*p11_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmul	%st(6),%st(0)	// t1*p00_minus_p20 | t0*p10_minus_p20 |
							//  t1*p01_minus_p21 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fxch	%st(2)			// t1*p01_minus_p21 | t0*p10_minus_p20 |
							//  t1*p00_minus_p20 | t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubp	%st(0),%st(3)	// t0*p10_minus_p20 | t1*p00_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubrp	%st(0),%st(1)	// t1*p00_minus_p20 - t0*p10_minus_p20 |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fmul	%st(2),%st(0)	// (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//   ystepdenominv |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fxch	%st(1)			// t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//  ystepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fmul	%st(3),%st(0)	// (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//  xstepdenominv |
							//  (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//  ystepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(1)			// (t1*p00_minus_p20 - t0*p10_minus_p20)*
							//  ystepdenominv |
							//  (t1*p01_minus_p21 - t0*p11_minus_p21)*
							//  xstepdenominv | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fistpl	C(r_tstepy)		// r_tstepx | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fistpl	C(r_tstepx)		// ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21

//	t0 = r_p0[5] - r_p2[5];
//	t1 = r_p1[5] - r_p2[5];

	fildl	C(r_p2)+20		// r_p2[5] | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fildl	C(r_p0)+20		// r_p0[5] | r_p2[5] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fildl	C(r_p1)+20		// r_p1[5] | r_p0[5] | r_p2[5] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fxch	%st(2)			// r_p2[5] | r_p0[5] | r_p1[5] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fld		%st(0)			// r_p2[5] | r_p2[5] | r_p0[5] | r_p1[5] |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  p11_minus_p21
	fsubrp	%st(0),%st(2)	// r_p2[5] | t0 | r_p1[5] | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 | p11_minus_p21
	fsubrp	%st(0),%st(2)	// t0 | t1 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21

//	r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
//			xstepdenominv);
//	r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
//			ystepdenominv);

	fld		%st(0)			// t0 | t0 | t1 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | p11_minus_p21
	fmulp	%st(0),%st(6)	// t0 | t1 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | t0*p11_minus_p21
	fxch	%st(1)			// t1 | t0 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | t0*p11_minus_p21
	fld		%st(0)			// t1 | t1 | t0 | ystepdenominv | xstepdenominv |
							//  p00_minus_p20 | t0*p11_minus_p21
	fmuls	p01_minus_p21	// t1*p01_minus_p21 | t1 | t0 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 |
							//  t0*p11_minus_p21
	fxch	%st(2)			// t0 | t1 | t1*p01_minus_p21 | ystepdenominv |
							//  xstepdenominv | p00_minus_p20 |
							//  t0*p11_minus_p21
	fmuls	p10_minus_p20	// t0*p10_minus_p20 | t1 | t1*p01_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  t0*p11_minus_p21
	fxch	%st(1)			// t1 | t0*p10_minus_p20 | t1*p01_minus_p21 |
							//  ystepdenominv | xstepdenominv | p00_minus_p20 |
							//  t0*p11_minus_p21
	fmulp	%st(0),%st(5)	// t0*p10_minus_p20 | t1*p01_minus_p21 |
							//  ystepdenominv | xstepdenominv |
							//  t1*p00_minus_p20 | t0*p11_minus_p21
	fxch	%st(5)			// t0*p11_minus_p21 | t1*p01_minus_p21 |
							//  ystepdenominv | xstepdenominv |
							//  t1*p00_minus_p20 | t0*p10_minus_p20
	fsubrp	%st(0),%st(1)	// t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  ystepdenominv | xstepdenominv |
							//  t1*p00_minus_p20 | t0*p10_minus_p20
	fxch	%st(3)			// t1*p00_minus_p20 | ystepdenominv |
							//  xstepdenominv |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  t0*p10_minus_p20
	fsubp	%st(0),%st(4)	// ystepdenominv | xstepdenominv |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  t1*p00_minus_p20 - t0*p10_minus_p20
	fxch	%st(1)			// xstepdenominv | ystepdenominv |
							//  t1*p01_minus_p21 - t0*p11_minus_p21 |
							//  t1*p00_minus_p20 - t0*p10_minus_p20
	fmulp	%st(0),%st(2)	// ystepdenominv |
							//  (t1*p01_minus_p21 - t0*p11_minus_p21) *
							//  xstepdenominv |
							//  t1*p00_minus_p20 - t0*p10_minus_p20
	fmulp	%st(0),%st(2)	// (t1*p01_minus_p21 - t0*p11_minus_p21) *
							//  xstepdenominv |
							//  (t1*p00_minus_p20 - t0*p10_minus_p20) *
							//  ystepdenominv
	fistpl	C(r_zistepx)	// (t1*p00_minus_p20 - t0*p10_minus_p20) *
							//  ystepdenominv
	fistpl	C(r_zistepy)

//	a_sstepxfrac = r_sstepx << 16;
//	a_tstepxfrac = r_tstepx << 16;
//
//	a_ststepxwhole = r_affinetridesc.skinwidth * (r_tstepx >> 16) +
//			(r_sstepx >> 16);

	movl	C(r_sstepx),%eax
	movl	C(r_tstepx),%edx
	shll	$16,%eax
	shll	$16,%edx
	movl	%eax,C(a_sstepxfrac)
	movl	%edx,C(a_tstepxfrac)

	movl	C(r_sstepx),%ecx
	movl	C(r_tstepx),%eax
	sarl	$16,%ecx
	sarl	$16,%eax
	imull	skinwidth(%esp)
	addl	%ecx,%eax
	movl	%eax,C(a_ststepxwhole)

	ret


//----------------------------------------------------------------------
// 8-bpp horizontal span drawing code for affine polygons, with smooth
// shading and no transparency
//----------------------------------------------------------------------

#define pspans	4+8

.globl C(D_PolysetAff8Start)
C(D_PolysetAff8Start):

.globl C(D_PolysetDrawSpans8)
C(D_PolysetDrawSpans8):
	pushl	%esi				// preserve register variables
	pushl	%ebx

	movl	pspans(%esp),%esi	// point to the first span descriptor
	movl	C(r_zistepx),%ecx

	pushl	%ebp				// preserve caller's stack frame
	pushl	%edi

	rorl	$16,%ecx			// put high 16 bits of 1/z step in low word
	movl	spanpackage_t_count(%esi),%edx

	movl	%ecx,lzistepx

LSpanLoop:

//		lcount = d_aspancount - pspanpackage->count;
//
//		errorterm += erroradjustup;
//		if (errorterm >= 0)
//		{
//			d_aspancount += d_countextrastep;
//			errorterm -= erroradjustdown;
//		}
//		else
//		{
//			d_aspancount += ubasestep;
//		}
	movl	C(d_aspancount),%eax
	subl	%edx,%eax

	movl	C(erroradjustup),%edx
	movl	C(errorterm),%ebx
	addl	%edx,%ebx
	js		LNoTurnover

	movl	C(erroradjustdown),%edx
	movl	C(d_countextrastep),%edi
	subl	%edx,%ebx
	movl	C(d_aspancount),%ebp
	movl	%ebx,C(errorterm)
	addl	%edi,%ebp
	movl	%ebp,C(d_aspancount)
	jmp		LRightEdgeStepped

LNoTurnover:
	movl	C(d_aspancount),%edi
	movl	C(ubasestep),%edx
	movl	%ebx,C(errorterm)
	addl	%edx,%edi
	movl	%edi,C(d_aspancount)

LRightEdgeStepped:
	cmpl	$1,%eax

	jl		LNextSpan
	jz		LExactlyOneLong

//
// set up advancetable
//
	movl	C(a_ststepxwhole),%ecx
	movl	C(r_affinetridesc)+atd_skinwidth,%edx

	movl	%ecx,advancetable+4	// advance base in t
	addl	%edx,%ecx

	movl	%ecx,advancetable	// advance extra in t
	movl	C(a_tstepxfrac),%ecx

	movw	C(r_lstepx),%cx
	movl	%eax,%edx			// count

	movl	%ecx,tstep
	addl	$7,%edx

	shrl	$3,%edx				// count of full and partial loops
	movl	spanpackage_t_sfrac(%esi),%ebx

	movw	%dx,%bx
	movl	spanpackage_t_pz(%esi),%ecx

	negl	%eax

	movl	spanpackage_t_pdest(%esi),%edi
	andl	$7,%eax		// 0->0, 1->7, 2->6, ... , 7->1

	subl	%eax,%edi	// compensate for hardwired offsets
	subl	%eax,%ecx

	subl	%eax,%ecx
	movl	spanpackage_t_tfrac(%esi),%edx

	movw	spanpackage_t_light(%esi),%dx
	movl	spanpackage_t_zi(%esi),%ebp

	rorl	$16,%ebp	// put high 16 bits of 1/z in low word
	pushl	%esi

	movl	spanpackage_t_ptex(%esi),%esi
	jmp		aff8entryvec_table(,%eax,4)

// %bx = count of full and partial loops
// %ebx high word = sfrac
// %ecx = pz
// %dx = light
// %edx high word = tfrac
// %esi = ptex
// %edi = pdest
// %ebp = 1/z
// tstep low word = C(r_lstepx)
// tstep high word = C(a_tstepxfrac)
// C(a_sstepxfrac) low word = 0
// C(a_sstepxfrac) high word = C(a_sstepxfrac)

LDrawLoop:

// FIXME: do we need to clamp light? We may need at least a buffer bit to
// keep it from poking into tfrac and causing problems

LDraw8:
	cmpw	(%ecx),%bp
	jl		Lp1
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,(%ecx)
	movb	0x12345678(%eax),%al
LPatch8:
	movb	%al,(%edi)
Lp1:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw7:
	cmpw	2(%ecx),%bp
	jl		Lp2
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,2(%ecx)
	movb	0x12345678(%eax),%al
LPatch7:
	movb	%al,1(%edi)
Lp2:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw6:
	cmpw	4(%ecx),%bp
	jl		Lp3
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,4(%ecx)
	movb	0x12345678(%eax),%al
LPatch6:
	movb	%al,2(%edi)
Lp3:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw5:
	cmpw	6(%ecx),%bp
	jl		Lp4
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,6(%ecx)
	movb	0x12345678(%eax),%al
LPatch5:
	movb	%al,3(%edi)
Lp4:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw4:
	cmpw	8(%ecx),%bp
	jl		Lp5
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,8(%ecx)
	movb	0x12345678(%eax),%al
LPatch4:
	movb	%al,4(%edi)
Lp5:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw3:
	cmpw	10(%ecx),%bp
	jl		Lp6
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,10(%ecx)
	movb	0x12345678(%eax),%al
LPatch3:
	movb	%al,5(%edi)
Lp6:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw2:
	cmpw	12(%ecx),%bp
	jl		Lp7
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,12(%ecx)
	movb	0x12345678(%eax),%al
LPatch2:
	movb	%al,6(%edi)
Lp7:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

LDraw1:
	cmpw	14(%ecx),%bp
	jl		Lp8
	xorl	%eax,%eax
	movb	%dh,%ah
	movb	(%esi),%al
	movw	%bp,14(%ecx)
	movb	0x12345678(%eax),%al
LPatch1:
	movb	%al,7(%edi)
Lp8:
	addl	tstep,%edx
	sbbl	%eax,%eax
	addl	lzistepx,%ebp
	adcl	$0,%ebp
	addl	C(a_sstepxfrac),%ebx
	adcl	advancetable+4(,%eax,4),%esi

	addl	$8,%edi
	addl	$16,%ecx

	decw	%bx
	jnz		LDrawLoop

	popl	%esi				// restore spans pointer
LNextSpan:
	addl	$(spanpackage_t_size),%esi	// point to next span
LNextSpanESISet:
	movl	spanpackage_t_count(%esi),%edx
	cmpl	$-999999,%edx		// any more spans?
	jnz		LSpanLoop			// yes

	popl	%edi
	popl	%ebp				// restore the caller's stack frame
	popl	%ebx				// restore register variables
	popl	%esi
	ret


// draw a one-long span

LExactlyOneLong:

	movl	spanpackage_t_pz(%esi),%ecx
	movl	spanpackage_t_zi(%esi),%ebp

	rorl	$16,%ebp	// put high 16 bits of 1/z in low word
	movl	spanpackage_t_ptex(%esi),%ebx

	cmpw	(%ecx),%bp
	jl		LNextSpan
	xorl	%eax,%eax
	movl	spanpackage_t_pdest(%esi),%edi
	movb	spanpackage_t_light+1(%esi),%ah
	addl	$(spanpackage_t_size),%esi	// point to next span
	movb	(%ebx),%al
	movw	%bp,(%ecx)
	movb	0x12345678(%eax),%al
LPatch9:
	movb	%al,(%edi)

	jmp		LNextSpanESISet

.globl C(D_PolysetAff8End)
C(D_PolysetAff8End):


#define pcolormap		4

.globl C(D_Aff8Patch)
C(D_Aff8Patch):
	movl	pcolormap(%esp),%eax
	movl	%eax,LPatch1-4
	movl	%eax,LPatch2-4
	movl	%eax,LPatch3-4
	movl	%eax,LPatch4-4
	movl	%eax,LPatch5-4
	movl	%eax,LPatch6-4
	movl	%eax,LPatch7-4
	movl	%eax,LPatch8-4
	movl	%eax,LPatch9-4

	ret



//----------------------------------------------------------------------
// Alias model polygon dispatching code, combined with subdivided affine
// triangle drawing code
//----------------------------------------------------------------------

.globl C(D_PolysetDraw)
C(D_PolysetDraw):

//	spanpackage_t	spans[DPS_MAXSPANS + 1 +
//			((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
//						// one extra because of cache line pretouching
//
//	a_spans = (spanpackage_t *)
//			(((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	subl	$(SPAN_SIZE),%esp
	movl	%esp,%eax
	addl	$(CACHE_SIZE - 1),%eax
	andl	$(~(CACHE_SIZE - 1)),%eax
	movl	%eax,C(a_spans)

//	D_DrawNonSubdiv ();
	jmp		C(D_DrawNonSubdiv)

	ret


//----------------------------------------------------------------------
// Alias model triangle left-edge scanning code
//----------------------------------------------------------------------

#define height	4+16

.globl C(D_PolysetScanLeftEdge)
C(D_PolysetScanLeftEdge):
	pushl	%ebp				// preserve caller stack frame pointer
	pushl	%esi				// preserve register variables
	pushl	%edi
	pushl	%ebx

	movl	height(%esp),%eax
	movl	C(d_sfrac),%ecx
	andl	$0xFFFF,%eax
	movl	C(d_ptex),%ebx
	orl		%eax,%ecx
	movl	C(d_pedgespanpackage),%esi
	movl	C(d_tfrac),%edx
	movl	C(d_light),%edi
	movl	C(d_zi),%ebp

// %eax: scratch
// %ebx: d_ptex
// %ecx: d_sfrac in high word, count in low word
// %edx: d_tfrac
// %esi: d_pedgespanpackage, errorterm, scratch alternately
// %edi: d_light
// %ebp: d_zi

//	do
//	{

LScanLoop:

//		d_pedgespanpackage->ptex = ptex;
//		d_pedgespanpackage->pdest = d_pdest;
//		d_pedgespanpackage->pz = d_pz;
//		d_pedgespanpackage->count = d_aspancount;
//		d_pedgespanpackage->light = d_light;
//		d_pedgespanpackage->zi = d_zi;
//		d_pedgespanpackage->sfrac = d_sfrac << 16;
//		d_pedgespanpackage->tfrac = d_tfrac << 16;
	movl	%ebx,spanpackage_t_ptex(%esi)
	movl	C(d_pdest),%eax
	movl	%eax,spanpackage_t_pdest(%esi)
	movl	C(d_pz),%eax
	movl	%eax,spanpackage_t_pz(%esi)
	movl	C(d_aspancount),%eax
	movl	%eax,spanpackage_t_count(%esi)
	movl	%edi,spanpackage_t_light(%esi)
	movl	%ebp,spanpackage_t_zi(%esi)
	movl	%ecx,spanpackage_t_sfrac(%esi)
	movl	%edx,spanpackage_t_tfrac(%esi)

// pretouch the next cache line
	movb	spanpackage_t_size(%esi),%al

//		d_pedgespanpackage++;
	addl	$(spanpackage_t_size),%esi
	movl	C(erroradjustup),%eax
	movl	%esi,C(d_pedgespanpackage)

//		errorterm += erroradjustup;
	movl	C(errorterm),%esi
	addl	%eax,%esi
	movl	C(d_pdest),%eax

//		if (errorterm >= 0)
//		{
	js		LNoLeftEdgeTurnover

//			errorterm -= erroradjustdown;
//			d_pdest += d_pdestextrastep;
	subl	C(erroradjustdown),%esi
	addl	C(d_pdestextrastep),%eax
	movl	%esi,C(errorterm)
	movl	%eax,C(d_pdest)

//			d_pz += d_pzextrastep;
//			d_aspancount += d_countextrastep;
//			d_ptex += d_ptexextrastep;
//			d_sfrac += d_sfracextrastep;
//			d_ptex += d_sfrac >> 16;
//			d_sfrac &= 0xFFFF;
//			d_tfrac += d_tfracextrastep;
	movl	C(d_pz),%eax
	movl	C(d_aspancount),%esi
	addl	C(d_pzextrastep),%eax
	addl	C(d_sfracextrastep),%ecx
	adcl	C(d_ptexextrastep),%ebx
	addl	C(d_countextrastep),%esi
	movl	%eax,C(d_pz)
	movl	C(d_tfracextrastep),%eax
	movl	%esi,C(d_aspancount)
	addl	%eax,%edx

//			if (d_tfrac & 0x10000)
//			{
	jnc		LSkip1

//				d_ptex += r_affinetridesc.skinwidth;
//				d_tfrac &= 0xFFFF;
	addl	C(r_affinetridesc)+atd_skinwidth,%ebx

//			}

LSkip1:

//			d_light += d_lightextrastep;
//			d_zi += d_ziextrastep;
	addl	C(d_lightextrastep),%edi
	addl	C(d_ziextrastep),%ebp

//		}
	movl	C(d_pedgespanpackage),%esi
	decl	%ecx
	testl	$0xFFFF,%ecx
	jnz		LScanLoop

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

//		else
//		{

LNoLeftEdgeTurnover:
	movl	%esi,C(errorterm)

//			d_pdest += d_pdestbasestep;
	addl	C(d_pdestbasestep),%eax
	movl	%eax,C(d_pdest)

//			d_pz += d_pzbasestep;
//			d_aspancount += ubasestep;
//			d_ptex += d_ptexbasestep;
//			d_sfrac += d_sfracbasestep;
//			d_ptex += d_sfrac >> 16;
//			d_sfrac &= 0xFFFF;
	movl	C(d_pz),%eax
	movl	C(d_aspancount),%esi
	addl	C(d_pzbasestep),%eax
	addl	C(d_sfracbasestep),%ecx
	adcl	C(d_ptexbasestep),%ebx
	addl	C(ubasestep),%esi
	movl	%eax,C(d_pz)
	movl	%esi,C(d_aspancount)

//			d_tfrac += d_tfracbasestep;
	movl	C(d_tfracbasestep),%esi
	addl	%esi,%edx

//			if (d_tfrac & 0x10000)
//			{
	jnc		LSkip2

//				d_ptex += r_affinetridesc.skinwidth;
//				d_tfrac &= 0xFFFF;
	addl	C(r_affinetridesc)+atd_skinwidth,%ebx

//			}

LSkip2:

//			d_light += d_lightbasestep;
//			d_zi += d_zibasestep;
	addl	C(d_lightbasestep),%edi
	addl	C(d_zibasestep),%ebp

//		}
//	} while (--height);
	movl	C(d_pedgespanpackage),%esi
	decl	%ecx
	testl	$0xFFFF,%ecx
	jnz		LScanLoop

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

//----------------------------------------------------------------------
// Alias model non-subdivided polygon dispatching code
//
// not C-callable because of stack buffer cleanup
//----------------------------------------------------------------------

.globl C(D_DrawNonSubdiv)
C(D_DrawNonSubdiv):
	pushl	%ebp				// preserve caller stack frame pointer
	movl	C(r_affinetridesc)+atd_numtriangles,%ebp
	pushl	%ebx
	shll	$(mtri_shift),%ebp
	pushl	%esi				// preserve register variables
	movl	C(r_affinetridesc)+atd_ptriangles,%esi
	pushl	%edi

//	mtriangle_t		*ptri;
//	finalvert_t		*pfv, *index0, *index1, *index2;
//	int				i;
//	int				lnumtriangles;

//	pfv = r_affinetridesc.pfinalverts;
//	ptri = r_affinetridesc.ptriangles;
//	lnumtriangles = r_affinetridesc.numtriangles;

LNDLoop:

//	for (i=0 ; i<lnumtriangles ; i++, ptri++)
//	{
//		index0 = pfv + ptri->vertindex[0];
//		index1 = pfv + ptri->vertindex[1];
//		index2 = pfv + ptri->vertindex[2];
	movl	C(r_affinetridesc)+atd_pfinalverts,%edi
	movl	mtri_vertindex+0-mtri_size(%esi,%ebp,1),%ecx
	shll	$(fv_shift),%ecx
	movl	mtri_vertindex+4-mtri_size(%esi,%ebp,1),%edx
	shll	$(fv_shift),%edx
	movl	mtri_vertindex+8-mtri_size(%esi,%ebp,1),%ebx
	shll	$(fv_shift),%ebx
	addl	%edi,%ecx
	addl	%edi,%edx
	addl	%edi,%ebx

//		d_xdenom = (index0->v[1]-index1->v[1]) *
//				(index0->v[0]-index2->v[0]) -
//				(index0->v[0]-index1->v[0])*(index0->v[1]-index2->v[1]);
	movl	fv_v+4(%ecx),%eax
	movl	fv_v+0(%ecx),%esi
	subl	fv_v+4(%edx),%eax
	subl	fv_v+0(%ebx),%esi
	imull	%esi,%eax
	movl	fv_v+0(%ecx),%esi
	movl	fv_v+4(%ecx),%edi
	subl	fv_v+0(%edx),%esi
	subl	fv_v+4(%ebx),%edi
	imull	%esi,%edi
	subl	%edi,%eax

//		if (d_xdenom >= 0)
//		{
//			continue;
	jns		LNextTri

//		}

	movl	%eax,C(d_xdenom)
	fildl	C(d_xdenom)

//		r_p0[0] = index0->v[0];		// u
//		r_p0[1] = index0->v[1];		// v
//		r_p0[2] = index0->v[2];		// s
//		r_p0[3] = index0->v[3];		// t
//		r_p0[4] = index0->v[4];		// light
//		r_p0[5] = index0->v[5];		// iz
	movl	fv_v+0(%ecx),%eax
	movl	fv_v+4(%ecx),%esi
	movl	%eax,C(r_p0)+0
	movl	%esi,C(r_p0)+4
	movl	fv_v+8(%ecx),%eax
	movl	fv_v+12(%ecx),%esi
	movl	%eax,C(r_p0)+8
	movl	%esi,C(r_p0)+12
	movl	fv_v+16(%ecx),%eax
	movl	fv_v+20(%ecx),%esi
	movl	%eax,C(r_p0)+16
	movl	%esi,C(r_p0)+20

	fdivrs	float_1

//		r_p1[0] = index1->v[0];
//		r_p1[1] = index1->v[1];
//		r_p1[2] = index1->v[2];
//		r_p1[3] = index1->v[3];
//		r_p1[4] = index1->v[4];
//		r_p1[5] = index1->v[5];
	movl	fv_v+0(%edx),%eax
	movl	fv_v+4(%edx),%esi
	movl	%eax,C(r_p1)+0
	movl	%esi,C(r_p1)+4
	movl	fv_v+8(%edx),%eax
	movl	fv_v+12(%edx),%esi
	movl	%eax,C(r_p1)+8
	movl	%esi,C(r_p1)+12
	movl	fv_v+16(%edx),%eax
	movl	fv_v+20(%edx),%esi
	movl	%eax,C(r_p1)+16
	movl	%esi,C(r_p1)+20

//		r_p2[0] = index2->v[0];
//		r_p2[1] = index2->v[1];
//		r_p2[2] = index2->v[2];
//		r_p2[3] = index2->v[3];
//		r_p2[4] = index2->v[4];
//		r_p2[5] = index2->v[5];
	movl	fv_v+0(%ebx),%eax
	movl	fv_v+4(%ebx),%esi
	movl	%eax,C(r_p2)+0
	movl	%esi,C(r_p2)+4
	movl	fv_v+8(%ebx),%eax
	movl	fv_v+12(%ebx),%esi
	movl	%eax,C(r_p2)+8
	movl	%esi,C(r_p2)+12
	movl	fv_v+16(%ebx),%eax
	movl	fv_v+20(%ebx),%esi
	movl	%eax,C(r_p2)+16
	movl	C(r_affinetridesc)+atd_ptriangles,%edi
	movl	%esi,C(r_p2)+20
	movl	mtri_facesfront-mtri_size(%edi,%ebp,1),%eax

//		if (!ptri->facesfront)
//		{
	testl	%eax,%eax
	jnz		LFacesFront

//			if (index0->flags & ALIAS_ONSEAM)
//				r_p0[2] += r_affinetridesc.seamfixupX16;
	movl	fv_flags(%ecx),%eax
	movl	fv_flags(%edx),%esi
	movl	fv_flags(%ebx),%edi
	testl	$(ALIAS_ONSEAM),%eax
	movl	C(r_affinetridesc)+atd_seamfixupX16,%eax
	jz		LOnseamDone0
	addl	%eax,C(r_p0)+8
LOnseamDone0:

//			if (index1->flags & ALIAS_ONSEAM)
// 				r_p1[2] += r_affinetridesc.seamfixupX16;
	testl	$(ALIAS_ONSEAM),%esi
	jz		LOnseamDone1
	addl	%eax,C(r_p1)+8
LOnseamDone1:

//			if (index2->flags & ALIAS_ONSEAM)
//				r_p2[2] += r_affinetridesc.seamfixupX16;
	testl	$(ALIAS_ONSEAM),%edi
	jz		LOnseamDone2
	addl	%eax,C(r_p2)+8
LOnseamDone2:

//		}

LFacesFront:

	fstps	C(d_xdenom)

//		D_PolysetSetEdgeTable ();
//		D_RasterizeAliasPolySmooth ();
		call	C(D_PolysetSetEdgeTable)
		call	C(D_RasterizeAliasPolySmooth)

LNextTri:
		movl	C(r_affinetridesc)+atd_ptriangles,%esi
		subl	$16,%ebp
		jnz		LNDLoop
//	}

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp

	addl	$(SPAN_SIZE),%esp

	ret


#endif	// id386

