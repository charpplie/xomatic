////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   brushcommon.h
//  Version:     v1.00
//  Created:     6/7/2010 by Jaesik
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __BRUSHCOMMON_H_
#define __BRUSHCOMMON_H_

struct SBrushCommon
{	
	struct STexInfo
	{
		float shift[2];
		float scale[2];
		float rotate;

		STexInfo()
		{
			shift[0] = shift[1] = 0;
			scale[0] = scale[1] = 1.0f;
			rotate = 0;
		}
	};

	enum ESolidBrushCreateType
	{
		eBrushCreateType_Box,
		eBrushCreateType_Cone,
		eBrushCreateType_Sphere,
		eBrushCreateType_Cylinder,
		eBrushCreateType_Plane,
		eBrushCreateType_Max,
	};

	enum ECSGOperationEnum
	{
		eCOE_Union,
		eCOE_Intersection,
		eCOE_Difference,
	};

	static const char* CSGOperationString[3];	

	typedef uint16 VertexFaceIndexType;	
	static VertexFaceIndexType VertexFaceMax;
};

#endif __BRUSHCOMMON_H_