////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushCSGCompiler.h
//  Version:     v1.00
//  Created:     12/4/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: 
////////////////////////////////////////////////////////////////////////////

#ifndef __BRUSHCSGCOMPILER_H__
#define __BRUSHCSGCOMPILER_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "BrushPlane.h"
#include "BrushCommon.h"

struct SBrush;

class CBrushCSGCompiler
{

public:

	typedef SBrushCommon::VertexFaceIndexType BrushIndexType;
	typedef SBrushCommon::STexInfo BrushTexInfo;

	bool Compile( 
		SBrushCommon::ECSGOperationEnum operation, 
		const SBrush* brush0, 
		const SBrush* brush1, 
		_smart_ptr<SBrush>& outBrush,
		Vec3& newPivot ) const;


private:	

	class CCSGBrush;

	struct SBSP_CSGNode
	{
	public:

		enum EBSPNodeEnum
		{
			eBNE_Partition	= 0x00000001,
			eBNE_LeafFace		= 0x00000002,
			eBNE_LeafSolid	= 0x00000004
		};

	public:

		SBSP_CSGNode()
			:			m_flag(0),
						m_frontnode(NULL),
						m_backnode(NULL)
		{
		}

		~SBSP_CSGNode()
		{
			SAFE_DELETE(m_frontnode);
			SAFE_DELETE(m_backnode);
		}

		inline void SetFlag( unsigned char flag ){	
			m_flag = flag;	
		}
		inline bool IsLeaf() const	{
			return (m_flag&eBNE_LeafFace) || (m_flag&eBNE_LeafSolid);
		}
		inline bool IsSolid() const	{
			return m_flag&eBNE_LeafSolid;
		}
		static SBSP_CSGNode* CreateNode(){	
			return new SBSP_CSGNode;
		}
		const SBrushPlane& GetPlane()	 const {	
			return m_plane;
		}
		void SetPlane( const SBrushPlane& plane ){
			m_plane = plane;
		}
		SBSP_CSGNode* GetFrontNode() const {
			return m_frontnode;
		}
		void SetFrontNode( SBSP_CSGNode* node ){
			m_frontnode = node;
		}
		SBSP_CSGNode* GetBackNode()	const {
			return m_backnode;
		}
		void SetBackNode( SBSP_CSGNode* node ){
			m_backnode = node;
		}

	private:

		unsigned char				m_flag;
		SBrushPlane					m_plane;

		SBSP_CSGNode* m_frontnode;
		SBSP_CSGNode* m_backnode;
	};

	struct SCSGVertex : public CRefCountBase
	{
		SCSGVertex( const SCSGVertex& rhs )
			: m_position(rhs.m_position)
		{
		}

		SCSGVertex( const Vec3& v )
			: m_position(v)
		{
		}

		Vec3 m_position;
	};

	typedef _smart_ptr<SCSGVertex>	CSGVertexSmartPointer;

	struct SCSGFace : public CRefCountBase
	{	
		SCSGFace()
		{
			m_boundbox.Reset();
		}

		SCSGFace( const SCSGFace& rhs )
			: m_vertexindexlist(rhs.m_vertexindexlist),
				m_boundbox(rhs.m_boundbox),
				m_plane(rhs.m_plane),
				m_TexInfo(rhs.m_TexInfo),
				m_matID(rhs.m_matID)
		{
		}

		inline void ComputeBoundBox( const std::vector<SCSGVertex*>& vertexlist )
		{
			m_boundbox.Reset();
			for( size_t i(0); i < m_vertexindexlist.size(); ++i )
				m_boundbox.Add(vertexlist[m_vertexindexlist[i]]->m_position);
		}

		inline void AddVertexIndex( BrushIndexType index )
		{
			m_vertexindexlist.push_back(index);
		}

		inline void InsertVertexIndex( BrushIndexType location, BrushIndexType index )
		{
			m_vertexindexlist.insert( m_vertexindexlist.begin() + location, index );
		}

		void Invert()
		{
			std::vector<BrushIndexType> temporaryIndexList(m_vertexindexlist);	
			for( size_t i = 0; i < temporaryIndexList.size(); ++i )
				m_vertexindexlist[i] = temporaryIndexList[temporaryIndexList.size()-i-1];	
			m_plane.Invert();
		}

		std::vector<BrushIndexType> m_vertexindexlist;
		AABB m_boundbox;
		SBrushPlane m_plane;
		BrushTexInfo m_TexInfo;
		short	m_matID;
	};
	
	typedef _smart_ptr<SCSGFace>		CSGFaceSmartPointer;

	class CCSGBrush : public CRefCountBase
	{
	public:		

		CCSGBrush()
			: m_BSPRootNode(NULL)
		{
		}

		~CCSGBrush()
		{
			SAFE_DELETE(m_BSPRootNode);
		}

		bool GenerateBSPTreeFromBrush();

		bool RecursivelyClipForUnion(
			const SBSP_CSGNode* node,			
			std::vector<BrushIndexType>& facepool,			
			_smart_ptr<CCSGBrush> finalBrush );

		void AddFaceGroup(	
			const CCSGBrush* brush, 
			const std::vector<BrushIndexType>& facepool	);	

		void EliminateTJunction();
		void InvertFaces();

		void AddFace( const SCSGFace& face );		
		void AddVertex( const SCSGVertex& vertex );

		inline size_t GetNumberOfFaces() const{		
			return m_FaceList.size();
		}
		inline size_t GetNumberOfVertices() const	{
			return m_VertexList.size();
		}
		inline const SBSP_CSGNode* GetBSPRootNode() const{		
			return m_BSPRootNode;
		}
		inline const Vec3& GetVertexPosition( int index ) const	{		
			return m_VertexList[index]->m_position;
		}
		inline const std::vector<BrushIndexType>& GetFaceVertexIndexList( int index )	const{	
			return m_FaceList[index]->m_vertexindexlist;
		}
		inline const SBrushPlane& GetFacePlane( int index )	const{	
			return m_FaceList[index]->m_plane;
		}
		inline const BrushTexInfo& GetFaceTexInfo( int index ) const{	
			return m_FaceList[index]->m_TexInfo;
		}
		inline short GetFaceMatID( int index ) const{	
			return m_FaceList[index]->m_matID;	
		}
		inline void SetVertexPosition( int index, const Vec3& position ){	
			m_VertexList[index]->m_position = position;
		}	
		void PullOntoPlane( const Vec3& position, const SBrushPlane& plane, Vec3* outPosition ) const;

	private:

		struct SFindFaceOutputParameters
		{
			SFindFaceOutputParameters( 	
				std::vector<BrushIndexType>& _frontFaces,
				std::vector<BrushIndexType>& _backFaces,
				std::vector<BrushIndexType>& _onplaneFaces )
				: frontFaces(_frontFaces),
					backFaces(_backFaces),
					onplaneFaces(_onplaneFaces),
					facetouchlist(NULL)
			{
			}

			SFindFaceOutputParameters( 
				std::vector<BrushIndexType>& _frontFaces,
				std::vector<BrushIndexType>& _backFaces,
				std::vector<BrushIndexType>& _onplaneFaces,
				std::vector<bool>& _facetouchlist )
				: frontFaces(_frontFaces),
					backFaces(_backFaces),
					onplaneFaces(_onplaneFaces),
					facetouchlist(&_facetouchlist)
			{
			}

			std::vector<BrushIndexType>& frontFaces;
			std::vector<BrushIndexType>& backFaces;
			std::vector<BrushIndexType>& onplaneFaces;
			std::vector<bool>* facetouchlist;
		};

		struct SSplitFaceOutputParameters
		{
			SSplitFaceOutputParameters( 
				CSGFaceSmartPointer& _frontFace, 
				CSGFaceSmartPointer& _backFace )
				: frontFace(_frontFace),
					backFace(_backFace),
					newvertexindexlist(NULL)
			{
			}

			SSplitFaceOutputParameters( 
				CSGFaceSmartPointer& _frontFace, 
				CSGFaceSmartPointer& _backFace,
				std::vector<BrushIndexType>& _newvertexindexlist )
				: frontFace(_frontFace),
					backFace(_backFace),
					newvertexindexlist(&_newvertexindexlist)
			{
			}

			CSGFaceSmartPointer&		frontFace;
			CSGFaceSmartPointer&		backFace;
			std::vector<BrushIndexType>* newvertexindexlist;
		};

		struct SFindPlaneOutputParameters
		{
			SFindPlaneOutputParameters( SBrushPlane& plane)
																	: bestplane(plane)																		
			{
			}

			SFindPlaneOutputParameters( SBrushPlane& plane, 																	
																	std::vector<bool>& touches )
																	: bestplane(plane),																		
																		facetouches(&touches)
			{
			}

			SBrushPlane& bestplane;			
			std::vector<bool>* facetouches;
		};
		

	private:

		void CountFrontBackVertex(	
			const SBrushPlane& plane,
			std::vector<char>& vertexsign,
			BrushIndexType* numberOfFrontVertices = NULL,
			BrushIndexType* numberOfBackVertices = NULL,
			BrushIndexType* numberOfZeroVertices = NULL ) const;

		bool SplitFace( 
			const SBrushPlane& plane, 
			const SCSGFace& face, 
			const std::vector<char>& vertexsign,
			CSGFaceSmartPointer& frontFace,
			CSGFaceSmartPointer& backFace,
			CSGFaceSmartPointer& onPlaneFace );

		bool RecursivelyGenerateBSPTree(	
			SBSP_CSGNode* parent, 
			std::vector<BrushIndexType>& facepool,
			std::vector<bool>& facetouches );

		bool FindFrontAndBackFaces(	
			const SBrushPlane& plane,
			const std::vector<BrushIndexType>& facepool,
			SFindFaceOutputParameters& outputParameters );

		bool SplitFaceTo2Parts(	
			const SBrushPlane& plane,
			const SCSGFace& face,
			const std::vector<char>& vertexsign,
			SSplitFaceOutputParameters& outputPararmeters );

		bool FindBestPlane(	
			const std::vector<BrushIndexType>& facepool,
			SFindPlaneOutputParameters& outputParameters ) const;

	private:

		std::vector<CSGVertexSmartPointer>	m_VertexList;
		std::vector<CSGFaceSmartPointer>		m_FaceList;
		SBSP_CSGNode*	m_BSPRootNode;
	};

	typedef _smart_ptr<CCSGBrush>	CSGBrushSmartPointer;


private:

	bool	CreateCSGBrushFromSBrush(	
		const SBrush*	fromBrush, 
		const Vec3& pivot,
		CSGBrushSmartPointer& resultBrush ) const;

	bool	CreateSBrushFromCSGBrush(	
		const CSGBrushSmartPointer&	fromBrush, 
	_smart_ptr<SBrush>& resultBrush ) const;

	bool	ModifyPivot(	
		const Vec3& origin, 
		CSGBrushSmartPointer brush, 
		Vec3& newOrigin ) const;


private:

	static const float VER_EPS;
	static const float VER_TIGHTEPS;

};

#endif //__BRUSHCSGCOMPILER_H__