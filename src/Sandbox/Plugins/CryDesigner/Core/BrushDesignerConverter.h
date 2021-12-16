#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerConverter.h
//  Created:     April/20/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
class CDesignerBrushObject;

class CBrushDesignerConverter
{
public:

	bool CreateNewDesignerObject();
	bool ConvertToDesignerObject();

	static bool ConvertSolidXMLToDesignerObject( XmlNodeRef pSolidNode, CDesignerBrushObject* pDesignerObject );
	static bool ConvertMeshToBrushDesigner( IIndexedMesh* pMesh, CBrushDesigner* pDesignerObject );

private:

	struct SSolidPolygon
	{
		std::vector<uint16> vIndexList;
		int matID;
		BUtil::STexInfo texinfo;
	};

	static void LoadTexInfo( BUtil::STexInfo*	texinfo, const XmlNodeRef& node );
	static void LoadPolygon( SSolidPolygon* polygon, const XmlNodeRef& polygonNode );
	static void LoadVertexList( std::vector<BrushVec3>& vertexlist, const XmlNodeRef& node );

	static void AddRegionsToDesigner( const std::vector<SSolidPolygon>& polygonList, const std::vector<BrushVec3>& vList, CDesignerBrushObject* pDesignerObject );

private:

	struct SSelectedMesh
	{
		SSelectedMesh()
		{
			m_pIndexedMesh = NULL;
			m_bLoadedIndexedMeshFromFile = false;
			m_pMaterial = NULL;
		}
		Matrix34 m_worldTM;
		CMaterial* m_pMaterial;
		IIndexedMesh* m_pIndexedMesh;
		bool m_bLoadedIndexedMeshFromFile;
		_smart_ptr<CBaseObject> m_pOriginalObject;
	};

	void GetSelectedObjects( std::vector<SSelectedMesh>& pObjects ) const;

	CDesignerBrushObject* CreateDesignerObject( IIndexedMesh* pMesh );
	bool ConvertMeshToDesignerObject( CDesignerBrushObject* pDesignerObject, IIndexedMesh* pMesh );

	void CreateDesignerObjects( std::vector<SSelectedMesh>& pSelectedMeshes, std::vector<CDesignerBrushObject*>& pOutDesignerObjects );
};