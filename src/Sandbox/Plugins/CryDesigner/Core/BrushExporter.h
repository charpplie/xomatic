////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   brushexporter.h
//  Version:     v1.00
//  Created:     15/12/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __brushexporter_h__
#define __brushexporter_h__
#pragma once

#include "BrushRegion.h"

// forward declarations.
class CBrushObject;
class CDesignerBrushObject;
class CAreaSolid;
class CPakFile;
class CCryMemFile;

//////////////////////////////////////////////////////////////////////////
// Brush Export structures.
//////////////////////////////////////////////////////////////////////////
#pragma pack(push,1)

struct SExportedBrushGeom
{
	enum EFlags
	{
		SUPPORT_LIGHTMAP = 0x01,
		NO_PHYSICS = 0x02,
	};
	int size; // Size of this sructure.
	char filename[128];
	int flags; //! @see EFlags
	Vec3 m_minBBox;
	Vec3 m_maxBBox;
};

struct SExportedBrushMaterial
{
	int size;
	char material[64];
};

#pragma pack(pop)
//////////////////////////////////////////////////////////////////////////

/** Export brushes from specified Indoor to .bld file.
*/
class CBrushExporter
{
public:
	void ExportBrushes( const CString &path,CPakFile &pakFile );

private:
	bool ExportAreaSolid( const CString &path, CAreaSolid* pAreaSolid, CPakFile& pakFile ) const;
	void ExportStatObj( const CString &path, IStatObj* pStatObj, CBaseObject* pObj, int renderFlag, const CString& sGeomFileName, CPakFile& pakFile );

	struct SAreaSolidStatistic
	{
		int numOfClosedPolygons;
		int numOfOpenPolygons;
		int totalSize;
	};

	static void ComputeAreaSolidMemoryStatistic( CAreaSolid* pAreaSolid, SAreaSolidStatistic& outStatistic, std::vector<CBrushRegion::RegionPtr>& optimizedRegions );	

	//////////////////////////////////////////////////////////////////////////
	std::map<CMaterial*,int> m_mtlMap;
	std::vector<SExportedBrushGeom> m_geoms;
	std::vector<SExportedBrushMaterial> m_materials;
};

#endif // __brushexporter_h__
