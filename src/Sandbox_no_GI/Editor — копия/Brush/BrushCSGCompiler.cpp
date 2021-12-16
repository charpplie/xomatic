////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushCSGCompiler.cpp
//  Version:     v1.00
//  Created:     12/4/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BrushCSGCompiler.h"
#include "Brush.h"

const float CBrushCSGCompiler::VER_EPS			= 0.001f;
const float CBrushCSGCompiler::VER_TIGHTEPS = 0.0001f;

bool CBrushCSGCompiler::Compile(	
	SBrushCommon::ECSGOperationEnum	csgoperation,
	const SBrush* fromBrush0,
	const SBrush* fromBrush1,
	_smart_ptr<SBrush>& outBrush,
	Vec3& newPivot ) const
{
	Vec3 pivot = fromBrush0->GetMatrix().GetTranslation();

	if( fromBrush0->GetNumberOfFaces() == 0 || fromBrush1->GetNumberOfFaces() == 0 )
	{
		if( csgoperation == SBrushCommon::eCOE_Intersection )
		{
			outBrush = SBrush::CreateBrush(NULL);
			newPivot = pivot;
			return true;
		}
	}

	CSGBrushSmartPointer resultbrush[2];
	CSGBrushSmartPointer bspbrush[2];

	CreateCSGBrushFromSBrush( fromBrush0, pivot, resultbrush[0] );
	CreateCSGBrushFromSBrush( fromBrush1, pivot, resultbrush[1] );
	CreateCSGBrushFromSBrush( fromBrush0, pivot, bspbrush[0] );
	CreateCSGBrushFromSBrush( fromBrush1, pivot, bspbrush[1] );

	CSGBrushSmartPointer finalbrush = new CCSGBrush;

	if( csgoperation == SBrushCommon::eCOE_Difference ||
			csgoperation == SBrushCommon::eCOE_Intersection )
	{
		resultbrush[0]->InvertFaces();
		bspbrush[0]->InvertFaces();
	}

	if( csgoperation == SBrushCommon::eCOE_Intersection )	
	{
		resultbrush[1]->InvertFaces();
		bspbrush[1]->InvertFaces();
	}

	for( int i = 0; i < 2; ++i )
	{
		if( bspbrush[i]->GenerateBSPTreeFromBrush() == false )
			return false;
	}

	for( int i = 0; i < 2; ++i )
	{
		std::vector<BrushIndexType> facepool;

		for( size_t j = 0; j < resultbrush[i]->GetNumberOfFaces(); ++j )
			facepool.push_back(j);

		if( resultbrush[i]->RecursivelyClipForUnion(		
																						bspbrush[1-i]->GetBSPRootNode(), 
																						facepool,
																						finalbrush ) == false )
		{
				return false;
		}
	}

	if( csgoperation == SBrushCommon::eCOE_Difference ||
			csgoperation == SBrushCommon::eCOE_Intersection )
	{
		finalbrush->InvertFaces();
	}

	// EliminateTJunction() method is the biggest bottleneck.
	// I think this part needs to be optimized using KD-Tree or OCTree or BSPTree etc.
	// If the calculation of CSG is chain, EliminateTJunction() method may be called once at last time.
	finalbrush->EliminateTJunction();

	ModifyPivot( pivot, finalbrush, newPivot );
	CreateSBrushFromCSGBrush( finalbrush, outBrush );

	return true;
}


bool CBrushCSGCompiler::CreateCSGBrushFromSBrush(	
	const SBrush*	fromBrush, 
	const Vec3& pivot,
	CSGBrushSmartPointer& resultBrush ) const
{
	resultBrush = new CCSGBrush;

	Matrix34 fromBrushTM = fromBrush->GetMatrix();
	Vec3 relativePos = fromBrushTM.GetTranslation()-pivot;
	fromBrushTM.SetTranslation(relativePos);

	for( size_t i = 0; i < fromBrush->GetNumberOfVertices(); ++i )
	{
		Vec3 position = fromBrushTM * fromBrush->GetVertexPos(i);
		resultBrush->AddVertex(position);
	}	

	size_t numberOfFaces = fromBrush->GetNumberOfFaces();
	for( size_t i = 0; i < numberOfFaces; ++i )
	{
		size_t faceidx = resultBrush->GetNumberOfFaces();
		SCSGFace face;
		size_t numberOfFacePoints = fromBrush->GetNumberOfFacePoints(i);

		for( size_t j = 0; j < numberOfFacePoints; ++j )
		{
			BrushIndexType vertexindex = fromBrush->GetFaceVertexIndex(i,j);
			face.AddVertexIndex( vertexindex );
		}

		face.m_boundbox	= fromBrush->GetFaceBBox(i);
		face.m_TexInfo			= fromBrush->GetFaceTexInfo(i);
		face.m_matID				= fromBrush->GetFaceMaterialID(i);

		std::vector<Vec3> positionlist;
		for( size_t j = 0; j < face.m_vertexindexlist.size(); ++j )
			positionlist.push_back( resultBrush->GetVertexPosition(face.m_vertexindexlist[j]) );

		BrushIndexType triIndex[3] = {0,1,2};

		if( SBrush::FindTrianglePointsInPolygonForFace(positionlist,triIndex[0],triIndex[1],triIndex[2]) )
		{			
			face.m_plane	= SBrushPlane(	
								resultBrush->GetVertexPosition(face.m_vertexindexlist[triIndex[0]]),
								resultBrush->GetVertexPosition(face.m_vertexindexlist[triIndex[1]]),
								resultBrush->GetVertexPosition(face.m_vertexindexlist[triIndex[2]]) );
			for( size_t j = 0; j < face.m_vertexindexlist.size(); ++j )
			{
				const Vec3& position(resultBrush->GetVertexPosition(face.m_vertexindexlist[j]));
				Vec3 newP01(position);
				resultBrush->PullOntoPlane( position, face.m_plane, &newP01 );
				resultBrush->SetVertexPosition( face.m_vertexindexlist[j], newP01 );
			}
			resultBrush->AddFace(face);
		}
		else
		{
			assert(0);
		}
	}	

	return true;
}


bool CBrushCSGCompiler::CreateSBrushFromCSGBrush(	
	const CSGBrushSmartPointer& fromBrush, 
	_smart_ptr<SBrush>& resultBrush ) const
{
	resultBrush = SBrush::CreateBrush(NULL);
	std::vector<Vec3> positionlist;

	for( size_t i = 0; i < fromBrush->GetNumberOfVertices(); ++i )
		positionlist.push_back( fromBrush->GetVertexPosition(i) );
	
	resultBrush->SetVertexList(positionlist);
	
	for( size_t i = 0; i < fromBrush->GetNumberOfFaces(); ++i )
	{
		const std::vector<BrushIndexType>& FaceVertexIndexList = fromBrush->GetFaceVertexIndexList(i);
		const BrushTexInfo&	TexInfo = fromBrush->GetFaceTexInfo(i);
		short MatID = fromBrush->GetFaceMatID(i);
		resultBrush->AddFace(	FaceVertexIndexList, 
													&TexInfo,
													&MatID );
	}

	resultBrush->BuildBrush(false,false);

	return true;
}


bool CBrushCSGCompiler::CCSGBrush::GenerateBSPTreeFromBrush()
{	
	SAFE_DELETE(m_BSPRootNode);
	m_BSPRootNode	= SBSP_CSGNode::CreateNode();

	std::vector<BrushIndexType> facepool;
	for( size_t i = 0; i < m_FaceList.size(); ++i )
		facepool.push_back(i);

	std::vector<bool> facetouches;
	facetouches.resize(m_FaceList.size());
	for( size_t i = 0; i < facetouches.size(); ++i )
		facetouches[i] = false;

	SBrushPlane plane;
	if( FindBestPlane(	facepool, 
											SFindPlaneOutputParameters( plane, facetouches ) ) )
	{
		m_BSPRootNode->SetPlane(plane);
		return RecursivelyGenerateBSPTree(	m_BSPRootNode, 
																				facepool, facetouches );
	}

	return false;
}


bool CBrushCSGCompiler::CCSGBrush::RecursivelyGenerateBSPTree(	
	SBSP_CSGNode* parent, 
	std::vector<BrushIndexType>& facepool,
	std::vector<bool>& facetouches )
{
	std::vector<BrushIndexType> frontFaces;
	std::vector<BrushIndexType> backFaces;
	std::vector<BrushIndexType> onplaneFaces;

	FindFrontAndBackFaces(	
		parent->GetPlane(), 
		facepool,
		SFindFaceOutputParameters(	frontFaces, 
			backFaces, 
			onplaneFaces,
			facetouches ) );

	if( onplaneFaces.empty() )
		return false;

	if( frontFaces.size() > 0 )
	{
		bool alltouch = true;

		for( size_t i = 0; i < frontFaces.size(); ++i )
		{
			if( facetouches[frontFaces[i]] == false )
			{
				alltouch = false;
				break;
			}
		}

		parent->SetFrontNode(SBSP_CSGNode::CreateNode());
		SBrushPlane plane;

		if( alltouch == true )
		{
			parent->GetFrontNode()->SetFlag(SBSP_CSGNode::eBNE_LeafFace);
		}
		else if( FindBestPlane(	frontFaces, 
														SFindPlaneOutputParameters( plane,
																												facetouches ) ) )
		{
			parent->GetFrontNode()->SetPlane(plane);
			parent->GetFrontNode()->SetFlag(SBSP_CSGNode::eBNE_Partition);			
			if( RecursivelyGenerateBSPTree( parent->GetFrontNode(), 
																			frontFaces, 
																			facetouches ) == false )
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else if( frontFaces.size() == 0 && !onplaneFaces.empty() )
	{
		parent->SetFrontNode(SBSP_CSGNode::CreateNode());
		parent->GetFrontNode()->SetFlag(SBSP_CSGNode::eBNE_LeafFace);
	}

	if( !backFaces.empty() )
	{
		parent->SetBackNode(SBSP_CSGNode::CreateNode());
		parent->GetBackNode()->SetFlag(SBSP_CSGNode::eBNE_Partition);
		SBrushPlane plane;
		if( FindBestPlane(	backFaces,
												SFindPlaneOutputParameters( 
															plane,
															facetouches ) ) )
		{
			parent->GetBackNode()->SetPlane(plane);
			if( RecursivelyGenerateBSPTree( 
						parent->GetBackNode(), 
						backFaces, 
						facetouches ) == false )
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		parent->SetBackNode(SBSP_CSGNode::CreateNode());
		parent->GetBackNode()->SetFlag(SBSP_CSGNode::eBNE_LeafSolid);
	}

	return true;
}


void CBrushCSGCompiler::CCSGBrush::CountFrontBackVertex(	
	const SBrushPlane& plane,
	std::vector<char>& vertexsign,
	BrushIndexType* numberOfFrontVertices,
	BrushIndexType* numberOfBackVertices,
	BrushIndexType* numberOfZeroVertices ) const
{
	BrushIndexType FrontCount	= 0;
	BrushIndexType BackCount		= 0;
	BrushIndexType ZeroCount		= 0;

	vertexsign.clear();
	vertexsign.resize(m_VertexList.size());

	for( size_t i = 0; i < m_VertexList.size(); ++i )	
	{
		float distanceFromPlane = plane.Distance(m_VertexList[i]->m_position);
		if( distanceFromPlane > VER_EPS )
		{
			vertexsign[i] = 1;
			++FrontCount;
		}
		else if( distanceFromPlane < -VER_EPS )
		{
			vertexsign[i] = -1;
			++BackCount;
		}
		else
		{
			vertexsign[i] = 0;
			++ZeroCount;
		}
	}

	if( numberOfFrontVertices )
		*numberOfFrontVertices = FrontCount;

	if( numberOfBackVertices )
		*numberOfBackVertices = BackCount;

	if( numberOfZeroVertices )
		*numberOfZeroVertices = ZeroCount;
}


bool CBrushCSGCompiler::CCSGBrush::SplitFace(	
	const SBrushPlane& plane, 
	const SCSGFace& face, 
	const std::vector<char>& vertexsign,
	CSGFaceSmartPointer& frontFace,
	CSGFaceSmartPointer& backFace,
	CSGFaceSmartPointer& onPlaneFace )
{
	int			FrontCounter	= 0;
	int			BackCounter		= 0;
	int			ZeroCounter		= 0;
	size_t	VertexIndexListSize	= face.m_vertexindexlist.size();

	for( size_t i = 0; i < VertexIndexListSize; ++i )
	{
		BrushIndexType vertexindex = face.m_vertexindexlist[i];
		if( vertexsign[vertexindex] == 1 )
			++FrontCounter;
		else if( vertexsign[vertexindex] == -1 )
			++BackCounter;
		else if( vertexsign[vertexindex] == 0 )
			++ZeroCounter;
	}

	if( FrontCounter > 0 && BackCounter == 0 )
	{
		frontFace		= new SCSGFace(face);
		backFace		=	NULL;
		onPlaneFace = NULL;
	}
	else if( FrontCounter == 0 && BackCounter > 0 )
	{
		frontFace		= NULL;
		backFace		= new SCSGFace(face);
		onPlaneFace = NULL;
	}
	else if( FrontCounter == 0 && BackCounter == 0 && ZeroCounter > 0 )
	{
		frontFace		= NULL;
		backFace		= NULL;
		onPlaneFace = new SCSGFace(face);
	}
	else if( FrontCounter > 0 && BackCounter > 0 )
	{
		if( SplitFaceTo2Parts(	plane, 
														face, 
														vertexsign, 
														SSplitFaceOutputParameters( frontFace,
																												backFace ) ) == true )
		{
				frontFace->m_TexInfo	= face.m_TexInfo;
				frontFace->m_matID		= face.m_matID;

				backFace->m_TexInfo		= face.m_TexInfo;
				backFace->m_matID			= face.m_matID;

				onPlaneFace = NULL;
		}
		else
		{
				return false;
		}
	}

	return true;
}


bool CBrushCSGCompiler::CCSGBrush::SplitFaceTo2Parts(	
	const SBrushPlane& plane,
	const SCSGFace& face,
	const std::vector<char>& vertexsign,
	SSplitFaceOutputParameters& outputPararmeters )
{
	outputPararmeters.frontFace	= new SCSGFace;
	outputPararmeters.backFace	= new SCSGFace;

	size_t vertexindexlistsize = face.m_vertexindexlist.size();	

	for( int k = 0; k < 2; ++k )
	{
		CSGFaceSmartPointer currentFace	= (k == 0) ?	outputPararmeters.frontFace : 
																									outputPararmeters.backFace;
		char	sign	= (k == 0) ? 1 : -1;

		currentFace->m_plane = face.m_plane;

		for( size_t i = 0; i < vertexindexlistsize; ++i )
		{
			size_t	nexti	 = (i+1)%vertexindexlistsize;
			BrushIndexType	vertexindex			= face.m_vertexindexlist[i];
			BrushIndexType	vertexnextindex	= face.m_vertexindexlist[nexti];

			if( vertexsign[vertexindex] == 0 )
			{
				currentFace->AddVertexIndex(vertexindex);
				continue;
			}

			if( vertexsign[vertexindex] == sign )
				currentFace->AddVertexIndex(vertexindex);			

			if( vertexsign[vertexnextindex] == 0 || vertexsign[vertexindex] == vertexsign[vertexnextindex] )
				continue;

			const Vec3& p0 = m_VertexList[vertexindex]->m_position;
			const Vec3& p1 = m_VertexList[vertexnextindex]->m_position;
			Vec3 p01;

			if( plane.HitTest( p0, p1, NULL, &p01, VER_EPS ) )
			{
				size_t newvertexindex = m_VertexList.size();

				if( newvertexindex >= SBrushCommon::VertexFaceMax-1 )
						return false;

				Vec3 newP01(p01);
				PullOntoPlane( p01, face.m_plane, &newP01 );					
				AddVertex(newP01);
				currentFace->AddVertexIndex(newvertexindex);
				if( outputPararmeters.newvertexindexlist )
					(*outputPararmeters.newvertexindexlist).push_back((BrushIndexType)newvertexindex);
			}
		}
	}

	return true;
}


bool CBrushCSGCompiler::CCSGBrush::FindBestPlane(	
	const std::vector<BrushIndexType>& facepool,
	SFindPlaneOutputParameters& outputParameters ) const
{
	if( facepool.empty() )
		return false;

	BrushIndexType bestfaceindex	= facepool[0];
	outputParameters.bestplane	= m_FaceList[bestfaceindex]->m_plane;

	if( outputParameters.facetouches )
	{ 
		assert( bestfaceindex < (*outputParameters.facetouches).size() );
		(*outputParameters.facetouches)[bestfaceindex] = true;
	}

	return true;
}


bool CBrushCSGCompiler::CCSGBrush::FindFrontAndBackFaces(	
	const SBrushPlane& plane,
	const std::vector<BrushIndexType>& facepool,
	SFindFaceOutputParameters& outputParameters )
{
	std::vector<char> vertexsign;
	CountFrontBackVertex( plane, vertexsign );

	for( size_t i = 0; i < facepool.size(); ++i )
	{
		BrushIndexType faceindex = facepool[i];
		const CSGFaceSmartPointer face = m_FaceList[faceindex];

		CSGFaceSmartPointer frontFace;
		CSGFaceSmartPointer backFace;
		CSGFaceSmartPointer onPlaneFace;

		if( SplitFace( plane, *face, vertexsign, frontFace, backFace, onPlaneFace ) == false )
				return false;

		assert( frontFace || backFace || onPlaneFace );

		if( frontFace && backFace )
		{
			m_FaceList[faceindex] = frontFace;
			outputParameters.frontFaces.push_back(faceindex);

			m_FaceList.push_back(backFace);
			outputParameters.backFaces.push_back(m_FaceList.size()-1);
			if( outputParameters.facetouchlist )
				(*outputParameters.facetouchlist).push_back(false);
		}
		else if( frontFace )
		{
			m_FaceList[faceindex] = frontFace;
			outputParameters.frontFaces.push_back(faceindex);
		}
		else if( backFace )
		{	
			m_FaceList[faceindex] = backFace;
			outputParameters.backFaces.push_back(faceindex);
		}
		else if( onPlaneFace )
		{
			m_FaceList[faceindex] = onPlaneFace;
			outputParameters.onplaneFaces.push_back(faceindex);
		}
	}

	return true;
}


bool CBrushCSGCompiler::CCSGBrush::RecursivelyClipForUnion(
	const SBSP_CSGNode* node,
	std::vector<BrushIndexType>& facepool,
	_smart_ptr<CCSGBrush> finalBrush )
{
	if( node->IsLeaf() )
	{
		if( !node->IsSolid() )
			finalBrush->AddFaceGroup( this, facepool );
		return true;
	}

	std::vector<BrushIndexType> frontfaces;
	std::vector<BrushIndexType> backfaces;
	std::vector<BrushIndexType> onplanefaces;
	
	if( FindFrontAndBackFaces(	
		node->GetPlane(),
		facepool,
		SFindFaceOutputParameters(
						frontfaces,
						backfaces,
						onplanefaces ) ) == false )
	{
			return false;
	}

	for( size_t i = 0; i < onplanefaces.size(); ++i )
		backfaces.push_back(onplanefaces[i]);

	if( !frontfaces.empty() )
	{
		if( RecursivelyClipForUnion(		
			node->GetFrontNode(), 			
			frontfaces, 
			finalBrush ) == false )
		{
			return false;
		}
	}

	if( !backfaces.empty() )
	{
		if( RecursivelyClipForUnion(		
			node->GetBackNode(), 			
			backfaces,			
			finalBrush ) == false )
		{
				return false;
		}
	}

	return true;
}


void CBrushCSGCompiler::CCSGBrush::AddFaceGroup(	
	const CCSGBrush* fromBrush, 
	const std::vector<BrushIndexType>& facepool )
{
	for( size_t i = 0; i < facepool.size(); ++i )
	{
		CSGFaceSmartPointer fromFace	= fromBrush->m_FaceList[facepool[i]];
		SCSGFace toFace;

		for( size_t j = 0; j < fromFace->m_vertexindexlist.size(); ++j )
		{
			const CSGVertexSmartPointer fromVertex = fromBrush->m_VertexList[fromFace->m_vertexindexlist[j]];
			size_t existedIndex = m_VertexList.size();
			AddVertex(*fromVertex);
			toFace.AddVertexIndex((BrushIndexType)existedIndex);
		}

		toFace.m_plane				= fromFace->m_plane;
		toFace.m_boundbox	= fromFace->m_boundbox;
		toFace.m_TexInfo			=	fromFace->m_TexInfo;
		toFace.m_matID				=	fromFace->m_matID;

		AddFace(toFace);
	}
}

void CBrushCSGCompiler::CCSGBrush::EliminateTJunction()
{
	size_t	vertexlistsize = m_VertexList.size();

	for( size_t i = 0; i < vertexlistsize; ++i )
	{
		const CSGVertexSmartPointer vertex = m_VertexList[i];
		size_t	facesize = m_FaceList.size();
		bool		bFind = false;

		for( size_t j = 0; j < facesize; ++j )
		{
			CSGFaceSmartPointer face = m_FaceList[j];
			size_t pointlistsize = face->m_vertexindexlist.size();

			for( size_t k = 0; k < pointlistsize; ++k )
			{
				size_t nextk = (k+1)%pointlistsize;

				const Vec3& current_v	= m_VertexList[face->m_vertexindexlist[k]]->m_position;
				const Vec3& next_v		= m_VertexList[face->m_vertexindexlist[nextk]]->m_position;

				if( !current_v.IsEquivalent(vertex->m_position, VER_EPS) && 
						!next_v.IsEquivalent(vertex->m_position, VER_EPS) )
				{
					Vec3 d0 = next_v - current_v;
					Vec3 d1 = vertex->m_position - current_v;

					if( d0.GetLength() > d1.GetLength() )
					{
						Vec3 nd0 = d0.GetNormalizedSafe();
						Vec3 nd1 = d1.GetNormalizedSafe();

						if( nd0.IsEquivalent(nd1,VER_TIGHTEPS) )
						{
							bFind = true;
							BrushIndexType newindex = (BrushIndexType)m_VertexList.size();

							Vec3	newP01(vertex->m_position);
							PullOntoPlane( vertex->m_position, face->m_plane, &newP01 );
							AddVertex(newP01);

							if( k < pointlistsize-1 )
								face->InsertVertexIndex( nextk, newindex );
							else
								face->AddVertexIndex( newindex );

							break;
						}
					}
				}
			}
			if( bFind == true )
				break;
		}
	}
}


void CBrushCSGCompiler::CCSGBrush::InvertFaces()
{
	for( size_t i = 0; i < m_FaceList.size(); ++i )
		m_FaceList[i]->Invert();
}


void CBrushCSGCompiler::CCSGBrush::AddFace( const SCSGFace& face )
{	
	m_FaceList.push_back(new SCSGFace(face));
}


void CBrushCSGCompiler::CCSGBrush::AddVertex( const SCSGVertex& _vertex )
{	
	m_VertexList.push_back(new SCSGVertex(_vertex));
}


void CBrushCSGCompiler::CCSGBrush::PullOntoPlane( const Vec3& position, const SBrushPlane& plane, Vec3* outPosition ) const
{
	Vec3 Out;
	if( plane.HitTest( position, position+plane.normal, NULL, &Out, VER_TIGHTEPS ) )
	{
		if( outPosition )
			*outPosition = Out;
	}
}

bool CBrushCSGCompiler::ModifyPivot(	
	const Vec3& pivot, 
	CSGBrushSmartPointer brush, 
	Vec3& newPivot ) const
{
	Vec3 relativePivot(0,0,0);
	AABB aabb;
	aabb.Reset();

	for( size_t i = 0; i < brush->GetNumberOfVertices(); ++i )
		aabb.Add( brush->GetVertexPosition(i) );

	relativePivot = aabb.GetCenter();
	newPivot			= pivot + relativePivot;

	for( size_t i = 0; i < brush->GetNumberOfVertices(); ++i )
		brush->SetVertexPosition( i, brush->GetVertexPosition(i) - relativePivot );

	return true;
}