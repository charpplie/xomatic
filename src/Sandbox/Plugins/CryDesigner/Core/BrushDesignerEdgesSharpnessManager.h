#pragma once

#include "BrushRegion.h"

class CBrushDesignerElementManager;

namespace BUtil
{
	struct SEdgeSharpness
	{
		string name;
		std::vector<BrushEdge3D> edges;
		float sharpness;
		GUID guid;
	};
};

class CBrushDesignerEdgeSharpnessManager : public CRefCountBase
{
public:

	CBrushDesignerEdgeSharpnessManager();
	~CBrushDesignerEdgeSharpnessManager();

	void Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo, CBrushDesigner* pDesigner );

	void CopyFromDesigner( CBrushDesigner* pDesigner, const CBrushDesigner* pSrcDesigner );

	bool AddEdges( const char* name, CBrushDesignerElementManager* pElements, float sharpness = 0 );
	bool AddEdges( const char* name, const std::vector<BrushEdge3D>& edges, float sharpness = 0 );
	void RemoveEdgeSharpness( const char* name );
	void RemoveEdgeSharpness( const BrushEdge3D& edge );

	void SetSharpness( const char* name, float sharpness );
	void Rename( const char* oldName, const char* newName );

	bool HasName( const char* name ) const;	
	string GenerateValidName( const char* baseName = "EdgeGroup" ) const;

	int GetCount() const { return m_EdgeSharpnessList.size(); }
	const BUtil::SEdgeSharpness& Get(int n) const { return m_EdgeSharpnessList[n]; }

	float FindSharpness( const BrushEdge3D& edge ) const;
	void Clear(){ m_EdgeSharpnessList.clear(); }	

	BUtil::SEdgeSharpness* FindEdgeSharpness( const char* name );

	struct SSharpEdgeInfo
	{
		SSharpEdgeInfo() : sharpnessindex(-1),edgeindex(-1) {}
		int sharpnessindex;
		int edgeindex;
	};
	SSharpEdgeInfo GetEdgeInfo( const BrushEdge3D& edge );
	void DeleteEdge( const SSharpEdgeInfo& edgeInfo );

private:

	std::vector<BUtil::SEdgeSharpness> m_EdgeSharpnessList;
};