
#ifndef LNM_DEBUG_DRAW_H
#define LNM_DEBUG_DRAW_H

namespace LayeredNavMesh {

/**
 * Useful utility stuff used by LNM-related debug draws both in editor and
 * runtime.
 */




inline int bit(int a, int b)
{
	return (a & (1 << b)) >> b;
}

inline ColorB IntToCol(int i, unsigned char a, bool grey = false)
{
	int	r = bit(i, 0) + bit(i, 3) * 2 + 1;
	int	g = bit(i, 1) + bit(i, 4) * 2 + 1;
	int	b = bit(i, 2) + bit(i, 5) * 2 + 1;

//	r = 255/*240*/ - 63/*15*/*r;
//	g = 255/*192*/ - 63/*15*/*g;
//	b = 255/*128*/ - 63/*7*/*b;

	r = 160 + 15*(r-4);
	g = 160 + 15*(g-4);
	b = 80 /*- 10*b*/;

	if (grey)
	{
		// NOTE Nov 12, 2008: <pvl> tweaked to make dynamic overlays appear purple-ish
		// - just took opportunity of the fact that nobody else sets 'grey' to true
//		r = (240+r)/2;
//		g = (240+g)/2;
		b = (240+b)/2;
	}

	return ColorB (r, g, b, a);
}

} // namespace LayeredNavMesh


#endif // LNM_DEBUG_DRAW_H
