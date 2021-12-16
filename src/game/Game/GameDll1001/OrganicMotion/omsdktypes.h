/*	
*	=========================================================================================
*	Copyright(c) 2007 Organic Motion, Inc. All Rights Reserved.
*	The coded instructions, statements, computer programs, and/or related
*	material (collectively the "Code") in these files contain unpublished
*	information proprietary to Organic Motion, Inc., which is protected by
*	United States of America federal copyright law and by international treaties.
*	Use, duplication, and or distribution only with written permission from Organic Motion, Inc.
*	THE CODE IS PROVIDED "AS IS" AND WITHOUT WARRANTY.
*	=========================================================================================
*/

//=======================================================================================
//	OMSDKTypes.h
//		Types used in the Organic Motion SDK.
//=======================================================================================
#pragma once

//==============================================
//	Math types
//==============================================
typedef union _Vector2
{
	struct
	{
		float x;
		float y;
	};
	float v[2];
} Vector2, *Vector2P;

typedef union _Vector3 
{
	struct
	{
		float x;
		float y;
		float z;
	};
	float v[3];
} Vector3, *Vector3P;

typedef union _Matrix16 
{
	struct 
	{
		float        _11, _12, _13, _14;
		float        _21, _22, _23, _24;
		float        _31, _32, _33, _34;
		float        _41, _42, _43, _44;
	};
	float m[4][4];
} Matrix16, *Matrix16P;

//==============================================
//	Data types
//		Types for Bone, Mesh, and Texture data
//==============================================

//Bone data types
enum OMShapeTypeEnum
{
	OMST_Box=0,
	OMST_Sphere=1,
	OMST_Cylinder=2
};

typedef int		OMShapeType;
typedef Vector3 OMBoxDim;
typedef Vector3 OMSphereDim;
typedef Vector3 OMCylinderDim;

typedef struct _OMBoneDimension
{
	OMShapeType type;	//omshapetype
	union
	{
		OMBoxDim boxsize;
		OMSphereDim spheresize;
		OMCylinderDim cylinderdim;
	};
} OMBoneDimension, *OMBoneDimensionP;

typedef Matrix16	OMBoneMatrix;
typedef Matrix16P	OMBoneMatrixP;

//Mesh data types
typedef struct _OMMeshVertex
{
	Vector3 pos;
	Vector3 normal;
	Vector2 uv;
	Vector3 id;
} OMMeshVertex, *OMMeshVertexP;

typedef unsigned short OMMeshIndex;

//Texture data types
typedef union _OMTexResWH
{
	struct  
	{	
		unsigned short width;
		unsigned short height;
	};
	struct  
	{	
		unsigned short x;
		unsigned short y;
	};

} OMTexRes, *OMTexResP;

typedef union _OMTexResWHD
{
	struct  
	{	
		unsigned short width;
		unsigned short height;
		unsigned short depth;
	};
	struct  
	{	
		unsigned short x;
		unsigned short y;
		unsigned short d;
	};
} OMTexResWHD,*OMTexResWHDP;

typedef unsigned char OMPixelBW;

typedef union _OMPixel3
{
	struct  
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
	};
	unsigned char bytearray[3];
} OMPixel3, *OMPixel3P;

typedef union _OMPixel4
{
	struct  
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	unsigned char bytearray[4];
	unsigned long value;

} OMPixel4, *OMPixel4P;





//==============================================
//	Stream Description Types
//==============================================

/*
*	enum OMDataType
*		Enumerates the different types of data transferrable in a stream.
*			OMDT_None: place holder
*			OMDT_Bone: matrices representing absolute position (m) and orientation
*					   of each bone in Organic Motion's skeletal structure.
*			OMDT_MeshVertex:	(not yet supported)
*			OMDT_MeshIndex:		(not yet supported)
*			OMDT_Texture:		(not yet supported)
*			OMDT_Time:	Frame time information. Includes 
*							frame id - linear frame count
*							timestamp - 64 bit number representing the time of the frame
*							timestamp freqnecy - 64 bit number representing timestamp resolution.
*						To calculate the actual time difference in seconds of two timestamps use:
*							timediff(s) = (timestamp1-timestamp2) / timestampfreq
*			OMDT_Send:			(not supported)
*
*/
enum OMDataType
{
	OMDT_None		= 0x0000,
	OMDT_Bone		= 0x0001,
	OMDT_MeshVertex	= 0x0002,
	OMDT_MeshIndex	= 0x0004,
	OMDT_Texture	= 0x0008,
	OMDT_Time		= 0x0010,
	OMDT_Image		= 0x0020,
	OMDT_Send		= 0xA50FF05A
};


/*
*	OMDataKey - a structure representing a request for a certain data type to be
*	part of the stream. Members include:
*		OMDataType type:	the type of data desired in the stream
*		int param:			(reserved for future use)
*		int key:			A value set by the CreateStream() call that is later used to
*							retrieve data from a frame.
*/
typedef struct _OMDataKey
{
	OMDataType type;
	int param;
	int key;
} OMDataKey, *OMDataKeyP;

//==============================================
// Misc types
//==============================================
typedef union _OMVersion
{
	struct
	{
		short major;
		short minor;
		short fb;
		short b;
	};
	struct
	{
		int major_minor;
		int fb_b;
	};
	__int64 all;
} OMVersion;

typedef OMVersion OMSystemVersion;
typedef OMVersion OMSDKVersion;

//on windows, should match value of INFINITE (see winbase.h)
#define OMINFINITE 0xFFFFFFFF



//==============================================
//	Result types
//==============================================
typedef int OMRESULT;
#define OMOK					1
#define OMALREADYCONNECTED		2
#define OMALREADYDISCONNECTED	3

#define OMFAIL				-1
#define OMNOTIMPLEMENTED	-2
#define OMINVALIDARGUMENTS	-3
#define OMNOTCONNECTED		-4
#define OMTIMEOUT			-5


typedef uint32 OMBOOL;
#define OMTRUE				1
#define OMFALSE				0
