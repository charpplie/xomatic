////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   brushexporter.cpp
//  Version:     v1.00
//  Created:     15/12/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BrushExporter.h"

#include "Objects\BrushObject.h"
#include "Core/BrushCommonInterface.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "Objects\Group.h"
#include "Material\Material.h"
#include "CryArray.h"
#include "Util\Pakfile.h"
#include "IChunkFile.h"
#include "Core/BaseBrush.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushDesignerPolygonDecomposer.h"

#include <I3DEngine.h>

#define BRUSH_SUB_FOLDER "Brush"
#define BRUSH_FILE "brush.lst"
#define BRUSH_LIST_FILE "brushlist.txt"

struct brush_sort_predicate
{
	bool operator() (const std::pair<CString,CBaseObject*>& left, const std::pair<CString,CBaseObject*>& right)
	{
		return left.first < right.first;
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CBrushExporter::ExportBrushes( const CString &path, CPakFile &pakFile )
{
	CLogFile::WriteLine( "Exporting Brushes...");

	int i;

	pakFile.RemoveDir( Path::Make( path,BRUSH_SUB_FOLDER ) );

	CString filename = Path::Make( path,BRUSH_FILE );
	CString brushListFilename = Path::Make( path,BRUSH_LIST_FILE );
	// Export first brushes geometries.
	//if (!CFileUtil::OverwriteFile( filename ))
	//return;

	// Delete the old one.
	//DeleteFile( filename );
	CCryMemFile file;

	//////////////////////////////////////////////////////////////////////////
	// Clear export data.
	//////////////////////////////////////////////////////////////////////////
	DynArray<CBaseObject*> objects;
	GetIEditor()->GetObjectManager()->GetObjects( objects );

	// Sort objects by name first.
	typedef std::vector<std::pair<CString,CBaseObject*> > SortedObjects;
	SortedObjects sortedObjects;
	for (i = 0; i < objects.size(); i++)
	{
		if( objects[i] == NULL )
			continue;
		if( objects[i]->GetType() != OBJTYPE_SOLID && objects[i]->GetType() != OBJTYPE_VOLUMESOLID )
			continue;

		CString gameFileName;
		if( CBrushCommonInterface::GenerateGameFilename(objects[i],gameFileName) == false )
			continue;

		sortedObjects.push_back( std::make_pair(gameFileName,objects[i]) );
	}

	std::sort( sortedObjects.begin(),sortedObjects.end(),brush_sort_predicate() );

	for (SortedObjects::const_iterator it = sortedObjects.begin(); it != sortedObjects.end(); ++it)
	{
		CBaseObject *pObject = it->second;

		if( pObject->GetType() != OBJTYPE_SOLID && pObject->GetType() != OBJTYPE_VOLUMESOLID )
			continue;

		if( pObject->IsKindOf(RUNTIME_CLASS(CAreaSolid)) )
		{
			CAreaSolid* obj = (CAreaSolid*)pObject;
			if( !ExportAreaSolid( path,obj,pakFile ) )
			{
				assert(0);
			}
		}
		else
		{
			if(!pObject->IsKindOf( RUNTIME_CLASS(CClipVolumeObject)))
				CBrushCommonInterface::UpdateStatObjWithoutBackFaces(pObject);

			_smart_ptr<IStatObj> pStatObj = NULL;
			if( CBrushCommonInterface::GetIStatObj(pObject, &pStatObj) == false )
				continue;
			CString gameFileName;
			if( CBrushCommonInterface::GenerateGameFilename(pObject, gameFileName) == false )
				continue;
			int nRenderFlag(0);
			if( CBrushCommonInterface::GetRenderFlag(pObject,nRenderFlag) == false )
				continue;
			ExportStatObj( path, pStatObj, pObject, nRenderFlag, gameFileName, pakFile );
			CBrushCommonInterface::UpdateStatObj(pObject);
		}
	}

	{
		// Nothing to export.
		pakFile.RemoveFile( filename );

	}

	// Save brushlist.txt
	{
		CCryMemFile brushListFile;
		string tempStr;
		for (int i = 0; i < m_geoms.size(); i++)
		{
			// write geometry description.
			tempStr = m_geoms[i].filename;
			tempStr += "\r\n";
			brushListFile.Write( tempStr.c_str(),tempStr.size() );
		}
		pakFile.UpdateFile( brushListFilename,brushListFile );
	}

	// Timur
	// Do not save brush.lst not used anymore.

	CLogFile::WriteString("Done.");
}

//////////////////////////////////////////////////////////////////////////
void CBrushExporter::ExportStatObj( const CString &path, IStatObj* pStatObj, CBaseObject* pObj, int renderFlag, const CString& sGeomFileName, CPakFile& pakFile )
{
	if( pStatObj == NULL || pObj == NULL )
		return;

	CString sRealGeomFileName = sGeomFileName;
	sRealGeomFileName.Replace( "%level%",Path::ToUnixPath(Path::RemoveBackslash(path)) );

	CString sInternalGeomFileName = sGeomFileName;

	IChunkFile *pChunkFile = NULL;
	if (pStatObj->SaveToCGF( sRealGeomFileName,&pChunkFile ))
	{
		void *pMemFile = NULL;
		int nFileSize = 0;
		pChunkFile->WriteToMemoryBuffer( &pMemFile,&nFileSize );
		pakFile.UpdateFile( sRealGeomFileName,pMemFile,nFileSize,true,ICryArchive::LEVEL_FASTER );
		pChunkFile->Release();
	}

	// add new geometry.
	SExportedBrushGeom geom;
	ZeroStruct( geom );
	geom.size = sizeof(geom);
	strcpy( geom.filename,sInternalGeomFileName );
	geom.flags = 0;
	geom.m_minBBox = pStatObj->GetBoxMin();
	geom.m_maxBBox = pStatObj->GetBoxMax();
	m_geoms.push_back( geom );
	int geomIndex = m_geoms.size()-1;	

	int mtlIndex = -1;
	CMaterial* pMaterial = pObj->GetMaterial();
	if (pMaterial)
	{
		mtlIndex = stl::find_in_map( m_mtlMap,pMaterial,-1 );
		if (mtlIndex < 0)
		{
			SExportedBrushMaterial mtl;
			mtl.size = sizeof(mtl);
			strncpy( mtl.material,pMaterial->GetFullName(),sizeof(mtl.material) );
			m_materials.push_back(mtl);
			mtlIndex = m_materials.size()-1;
			m_mtlMap[pMaterial] = mtlIndex;
		}
	}

	SExportedBrushGeom *pBrushGeom = &m_geoms[geomIndex];
	if (pBrushGeom && pStatObj )
	{
		if (pStatObj->GetPhysGeom() == NULL && pStatObj->GetPhysGeom(PHYS_GEOM_TYPE_NO_COLLIDE) == NULL)
			pBrushGeom->flags |= SExportedBrushGeom::NO_PHYSICS;		
	}
}

bool AppendDataToMemoryblock( CMemoryBlock& memoryBlock, int& memoryOffset, void* data, int datasize )
{
	if( memoryBlock.GetBuffer() == NULL )
		return false;
	int nextOffset = memoryOffset + datasize;
	if( nextOffset > memoryBlock.GetSize() )
		return false;
	memcpy( ((char*)memoryBlock.GetBuffer()) + memoryOffset, data, datasize );
	memoryOffset = nextOffset;
	return true;
}

bool ExportRegionForAreaSolid( CMemoryBlock& memoryBlock, int& offset, const BUtil::VertexList& vertices, const BUtil::FaceList& faces )
{
	int numberOfFaces(faces.size());
	for( int k = 0; k < numberOfFaces; ++k )
	{
		BUtil::FacePtr pFace = faces[k];
		int numberOfVertices(pFace->m_IndexList.size());

		if( !AppendDataToMemoryblock( memoryBlock, offset, &numberOfVertices, sizeof(numberOfVertices) ) )
			return false;

		for( int a = 0; a < numberOfVertices; ++a )
		{
			Vec3 vPos = ToVec3(vertices[pFace->m_IndexList[a]]->m_Pos);
			if( !AppendDataToMemoryblock( memoryBlock, offset, &vPos, sizeof(Vec3) ) )
				return false;
		}
	}
	return true;
}

bool CBrushExporter::ExportAreaSolid( const CString &path, CAreaSolid* pAreaSolid, CPakFile& pakFile ) const
{
	if( pAreaSolid == NULL )
		return false;
	CBaseBrush* pBrush(pAreaSolid->GetBrush());
	if( pBrush == NULL )
		return false;
	CBrushDesigner* pDesigner(pAreaSolid->GetDesigner());
	if( pDesigner == NULL )
		return false;

	std::vector<CBrushRegion::RegionPtr> optimizedRegions;
	pDesigner->GetRegionList(optimizedRegions);

	int iRegionSize = optimizedRegions.size();
	if( iRegionSize <= 0)
		return false;

	SAreaSolidStatistic statisticForAreaSolid;
	ComputeAreaSolidMemoryStatistic(pAreaSolid,statisticForAreaSolid,optimizedRegions);

	CMemoryBlock memoryBlock;
	memoryBlock.Allocate(statisticForAreaSolid.totalSize);

	int offset = 0;
	if( !AppendDataToMemoryblock( memoryBlock, offset, &statisticForAreaSolid.numOfClosedPolygons, sizeof(statisticForAreaSolid.numOfClosedPolygons) ) )
		return false;
	if( !AppendDataToMemoryblock( memoryBlock, offset, &statisticForAreaSolid.numOfOpenPolygons, sizeof(statisticForAreaSolid.numOfOpenPolygons) ) )
		return false;

	for( int i = 0; i < iRegionSize; ++i )
	{
		BUtil::VertexList vertexList;
		BUtil::FaceList faceList;
		CBrushDesignerPolygonDecomposer decomposer;
		decomposer.TriangulateRegion( optimizedRegions[i], vertexList, faceList );
		if( !ExportRegionForAreaSolid( memoryBlock, offset, vertexList, faceList ) )
			return false;
	}

	for( int i = 0; i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion(optimizedRegions[i]);
		std::vector<CBrushRegion::RegionPtr> innerRegions;
		pRegion->GetSeparatedRegions( innerRegions, CBrushRegion::eSR_InnerHull );
		for( int k = 0, iSize(innerRegions.size()); k < iSize; ++k )
		{
			if( pDesigner->QueryEquivalentRegion(innerRegions[k]) )
				continue;
			innerRegions[k]->ReverseEdges();
			BUtil::VertexList vertexList;
			BUtil::FaceList faceList;
			CBrushDesignerPolygonDecomposer decomposer;
			decomposer.TriangulateRegion( innerRegions[k], vertexList, faceList );
			if( !ExportRegionForAreaSolid( memoryBlock, offset, vertexList, faceList ) )
				return false;
		}
	}

	assert( statisticForAreaSolid.totalSize == offset );

	CString sRealGeomFileName = pAreaSolid->GenerateGameFilename();
	sRealGeomFileName.Replace( "%level%",Path::ToUnixPath(Path::RemoveBackslash(path)) );

	pakFile.UpdateFile( sRealGeomFileName, memoryBlock, true, ICryArchive::LEVEL_FASTER );
	return true;
}

int ComputeFaceSize( const BUtil::FaceList& faceList )
{
	int totalSize(0);

	for( int k = 0, iFaceSize(faceList.size()); k < iFaceSize; ++k )		
	{
		totalSize += sizeof(int); // size of number of vertices in a face.
		totalSize += faceList[k]->m_IndexList.size() * sizeof(Vec3);
	}

	return totalSize;
}

void CBrushExporter::ComputeAreaSolidMemoryStatistic( CAreaSolid* pAreaSolid, SAreaSolidStatistic& outStatistic, std::vector<CBrushRegion::RegionPtr>& optimizedRegions )
{
	if( pAreaSolid == NULL )
		return;
	CBaseBrush* pBrush(pAreaSolid->GetBrush());
	if( pBrush == NULL )
		return;
	CBrushDesigner* pDesigner(pAreaSolid->GetDesigner());
	if( pDesigner == NULL )
		return;	
	int iRegionSize = optimizedRegions.size();

	memset( &outStatistic, 0, sizeof(outStatistic) );

	outStatistic.totalSize += sizeof(outStatistic.numOfClosedPolygons);	// size of value of whole closed polygons' number
	outStatistic.totalSize += sizeof(outStatistic.numOfOpenPolygons);	// size of value of whole open polygons' number

	for( int i = 0; i < iRegionSize; ++i )
	{
		BUtil::VertexList vertexList;
		BUtil::FaceList faceList;
		CBrushDesignerPolygonDecomposer decomposer;
		decomposer.TriangulateRegion( optimizedRegions[i], vertexList, faceList );

		outStatistic.numOfClosedPolygons += faceList.size();
		outStatistic.totalSize += ComputeFaceSize(faceList);	
	}	

	for( int i = 0; i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion(optimizedRegions[i]);
		std::vector<CBrushRegion::RegionPtr> innerRegions;
		pRegion->GetSeparatedRegions( innerRegions, CBrushRegion::eSR_InnerHull );
		for( int k = 0, iSize(innerRegions.size()); k < iSize; ++k )
		{
			if( pDesigner->QueryEquivalentRegion(innerRegions[k]) )
				continue;
			innerRegions[k]->ReverseEdges();

			BUtil::VertexList vertexList;
			BUtil::FaceList faceList;
			CBrushDesignerPolygonDecomposer decomposer;
			decomposer.TriangulateRegion( innerRegions[k], vertexList, faceList );

			outStatistic.numOfOpenPolygons += faceList.size();
			outStatistic.totalSize += ComputeFaceSize( faceList );
		}
	}
}