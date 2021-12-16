#include "StdAfx.h"
#include "BrushDesignerEdgesSharpnessManager.h"
#include "BrushDesigner.h"
#include "BrushRegion.h"
#include "BrushDesignerElementManager.h"

CBrushDesignerEdgeSharpnessManager::CBrushDesignerEdgeSharpnessManager()
{
}

CBrushDesignerEdgeSharpnessManager::~CBrushDesignerEdgeSharpnessManager()
{
}

void CBrushDesignerEdgeSharpnessManager::Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo, CBrushDesigner* pDesigner )
{
	if( bLoading )
	{
		int nEdgeSharpnessCount = xmlNode->getChildCount();
		for( int i = 0; i < nEdgeSharpnessCount; ++i )
		{
			XmlNodeRef pSemiSharpCreaseNode = xmlNode->getChild(i);

			BUtil::SEdgeSharpness semiSharpCrease;
			const char* name = NULL;
			pSemiSharpCreaseNode->getAttr("name",&name);
			semiSharpCrease.name = name;
			pSemiSharpCreaseNode->getAttr("sharpness",semiSharpCrease.sharpness);
			pSemiSharpCreaseNode->getAttr("guid",semiSharpCrease.guid);
			
			int nEdgeCount = pSemiSharpCreaseNode->getChildCount();
			for( int k = 0; k < nEdgeCount; ++k )
			{
				XmlNodeRef pEdgeNode = pSemiSharpCreaseNode->getChild(k);
				BrushEdge3D e;
				pEdgeNode->getAttr("v0",e.m_v[0]);
				pEdgeNode->getAttr("v1",e.m_v[1]);
				semiSharpCrease.edges.push_back(e);
			}

			m_EdgeSharpnessList.push_back(semiSharpCrease);
		}
	}
	else
	{	
		std::vector<BUtil::SEdgeSharpness>::iterator ii = m_EdgeSharpnessList.begin();
		for( ; ii != m_EdgeSharpnessList.end(); ++ii )
		{
			const BUtil::SEdgeSharpness& semiSharpCrease = *ii;
			if( semiSharpCrease.edges.empty() )
				continue;

			XmlNodeRef pSemiSharpCreaseNode(xmlNode->newChild("SemiSharpCrease"));

			pSemiSharpCreaseNode->setAttr("name",semiSharpCrease.name);
			pSemiSharpCreaseNode->setAttr("sharpness",semiSharpCrease.sharpness);
			pSemiSharpCreaseNode->setAttr("guid",semiSharpCrease.guid);

			for( int i = 0, iEdgeCount(semiSharpCrease.edges.size()); i < iEdgeCount; ++i )
			{
				XmlNodeRef pEdgeNode = pSemiSharpCreaseNode->newChild("edge");
				pEdgeNode->setAttr("v0",semiSharpCrease.edges[i].m_v[0]);
				pEdgeNode->setAttr("v1",semiSharpCrease.edges[i].m_v[1]);
			}
		}
	}
}

void CBrushDesignerEdgeSharpnessManager::CopyFromDesigner( CBrushDesigner* pDesigner, const CBrushDesigner* pSrcDesigner )
{
	CBrushDesignerEdgeSharpnessManager* pDestEdgeSharpnessMgr = pDesigner->GetEdgeSharpnessMgr();
	const CBrushDesignerEdgeSharpnessManager* pSrcEdgeSharpnessMgr = pSrcDesigner->GetEdgeSharpnessMgr();
	pDestEdgeSharpnessMgr->Clear();
	pDestEdgeSharpnessMgr->m_EdgeSharpnessList = pSrcEdgeSharpnessMgr->m_EdgeSharpnessList;
}

bool CBrushDesignerEdgeSharpnessManager::AddEdges( const char* name, CBrushDesignerElementManager* pElements, float sharpness )
{
	std::vector<BrushEdge3D> edges;
	for( int i = 0, iCount(pElements->GetSize()); i < iCount; ++i )
	{
		const SDesignerElement& element = pElements->Get(i);
		if( element.IsEdge() )
		{
			edges.push_back(element.GetEdge());
		}
		if( element.IsFace() && element.m_pRegion )
		{
			for( int k = 0, iEdgeCount(element.m_pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
				edges.push_back(element.m_pRegion->GetEdge(k));
		}
	}
	return AddEdges(name,edges,sharpness);
}

bool CBrushDesignerEdgeSharpnessManager::AddEdges( const char* name, const std::vector<BrushEdge3D>& edges, float sharpness )
{
	if( edges.empty() || HasName(name) )
		return false;
	
	int iEdgeCount(edges.size());	
	for( int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i )
		DeleteEdge(GetEdgeInfo(edges[i]));

	BUtil::SEdgeSharpness edgeSharpness;
	edgeSharpness.name = name;
	edgeSharpness.edges = edges;
	edgeSharpness.sharpness = sharpness;
	CoCreateGuid(&edgeSharpness.guid);
	m_EdgeSharpnessList.push_back(edgeSharpness);

	return true;
}

void CBrushDesignerEdgeSharpnessManager::RemoveEdgeSharpness( const char* name )
{
	std::vector<BUtil::SEdgeSharpness>::iterator ii = m_EdgeSharpnessList.begin();
	for( ;ii != m_EdgeSharpnessList.end(); ++ii )
	{
		if( !stricmp(name, ii->name.c_str()) )
		{
			m_EdgeSharpnessList.erase(ii);
			break;
		}
	}
}

void CBrushDesignerEdgeSharpnessManager::RemoveEdgeSharpness( const BrushEdge3D& edge )
{
	BrushEdge3D invEdge = edge.GetInverted();
	std::vector<BUtil::SEdgeSharpness>::iterator ii = m_EdgeSharpnessList.begin();
	for( ;ii != m_EdgeSharpnessList.end(); )
	{
		for( int i = 0, iEdgeCount(ii->edges.size()); i < iEdgeCount; ++i )
		{
			if( ii->edges[i].IsEquivalent(edge,kDesignerEpsilon) || ii->edges[i].IsEquivalent(invEdge,kDesignerEpsilon) )
			{
				ii->edges.erase(ii->edges.begin()+i);
				break;
			}
		}
		if( ii->edges.empty() )
			ii = m_EdgeSharpnessList.erase(ii);
		else
			++ii;
	}
}

void CBrushDesignerEdgeSharpnessManager::SetSharpness( const char* name, float sharpness )
{
	BUtil::SEdgeSharpness* pEdgeSharpness = FindEdgeSharpness(name);
	if( pEdgeSharpness == NULL )
		return;
	pEdgeSharpness->sharpness = sharpness;
}

void CBrushDesignerEdgeSharpnessManager::Rename( const char* oldName, const char* newName )
{
	BUtil::SEdgeSharpness* pEdgeSharpness = FindEdgeSharpness(oldName);
	if( pEdgeSharpness == NULL )
		return;
	pEdgeSharpness->name = newName;
}

BUtil::SEdgeSharpness* CBrushDesignerEdgeSharpnessManager::FindEdgeSharpness( const char* name )
{
	if( name )
	{
		for( int i = 0, iCount(m_EdgeSharpnessList.size()); i < iCount; ++i )
		{
			if( m_EdgeSharpnessList[i].name == name )
				return &m_EdgeSharpnessList[i];
		}
	}
	return NULL;
}

float CBrushDesignerEdgeSharpnessManager::FindSharpness( const BrushEdge3D& edge ) const
{
	BrushEdge3D invEdge = edge.GetInverted();
	for( int i = 0, iCount(m_EdgeSharpnessList.size()); i < iCount; ++i )
	{
		const std::vector<BrushEdge3D>& edges = m_EdgeSharpnessList[i].edges;
		for( int k = 0, iEdgeCount(edges.size()); k < iEdgeCount; ++k )
		{
			if( edges[k].IsEquivalent(edge,kDesignerEpsilon) || edges[k].IsEquivalent(invEdge,kDesignerEpsilon) )
				return m_EdgeSharpnessList[i].sharpness;
		}
	}
	return 0;
}

bool CBrushDesignerEdgeSharpnessManager::HasName( const char* name ) const
{
	for( int i = 0, iCount(m_EdgeSharpnessList.size()); i < iCount; ++i )
	{
		if( !stricmp( m_EdgeSharpnessList[i].name.c_str(), name ) )
			return true;
	}
	return false;
}

CBrushDesignerEdgeSharpnessManager::SSharpEdgeInfo CBrushDesignerEdgeSharpnessManager::GetEdgeInfo( const BrushEdge3D& edge )
{
	SSharpEdgeInfo sei;
	BrushEdge3D invertedEdge = edge.GetInverted();

	for( int i = 0, iCount(m_EdgeSharpnessList.size()); i < iCount; ++i )
	{
		BUtil::SEdgeSharpness& edgeSharpness = m_EdgeSharpnessList[i];
		for( int k = 0, iEdgeCount(edgeSharpness.edges.size()); k < iEdgeCount; ++k )
		{
			if( edgeSharpness.edges[k].IsEquivalent(edge,kDesignerEpsilon) || edgeSharpness.edges[k].IsEquivalent(invertedEdge,kDesignerEpsilon) )
			{
				sei.sharpnessindex = i;
				sei.edgeindex = k;
				break;
			}
		}
		if( sei.edgeindex != -1 )
			break;
	}

	return sei;
}

void CBrushDesignerEdgeSharpnessManager::DeleteEdge( const SSharpEdgeInfo& edgeInfo )
{
	if( edgeInfo.sharpnessindex == -1 || edgeInfo.sharpnessindex >= m_EdgeSharpnessList.size() )
		return;

	BUtil::SEdgeSharpness& sharpness = m_EdgeSharpnessList[edgeInfo.sharpnessindex];

	if( edgeInfo.edgeindex >= sharpness.edges.size() )
		return;

	sharpness.edges.erase(sharpness.edges.begin()+edgeInfo.edgeindex);
}

string CBrushDesignerEdgeSharpnessManager::GenerateValidName( const char* baseName ) const
{	
	string validName(baseName );
	char numStr[10] = {0,};
	int i = 0;	
	while(i < 1000000)
	{
		if( !HasName(validName) )
			break;
		validName = baseName;
		sprintf(numStr,"%d",++i);
		validName += numStr;
	}
	return validName;
}