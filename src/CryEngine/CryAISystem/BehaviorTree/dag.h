/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   dag.h
$Id$
$DateTime$
Description: Base class for a generic DAG (Directed Acyclic Graph)
In computer science and mathematics, a directed acyclic graph, also 
called a DAG, is a directed graph with no directed cycles; that is,
for any vertex v, there is no nonempty directed path that starts and
ends on v. DAGs appear in models where it doesn't make sense for a
vertex to have a path to itself; for example, if an edge u->v
indicates that v is a part of u, such a path would indicate that u
is a part of itself, which is impossible. Informally speaking, a DAG
"flows" in a single direction.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _DAG_H_
	#define _DAG_H_

	#include "dag_grid.h"

class CDagNode;

class CDag
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CDag( const char* sRootName );
		virtual ~CDag();

		/*$1- Utils --------------------------------------------------------------*/
		bool								ReLoad();
		bool								ReLoad( const string& sFile );
		virtual void				LoadingFinished();
		void								Flush();
		CDagNode*						GetNode( const char* sName, const char* sParent = NULL, bool bRecursively = true );
		virtual CDagNode*		GetRoot() const;
		void								DebugDraw() const;
		bool								IsInfoLoaded() const;
		void								SetSkipEmptyRows( bool bSkip );

		virtual bool				LoadSheet( const string& sName, uint32 uRows, uint32 uColumns, XmlNodeRef& xml );

	protected:

		/*$1- Members ------------------------------------------------------------*/
		CDag();		
		virtual void	DebugDrawNode( float xPos, float yPos, const CDagNode* pNode ) const;
		bool					AddNode( CDagNode* pParent, const char* sName );
		bool					DestroyNode( CDagNode* pNode );
		bool					LoadRow( XmlNodeRef& xmlTable, int iRow );
		bool					LoadNode( int iRow, int iColumn, const char* sName );
		CDagNode*			FindParent( int iRow, int iColumn );
		bool					IsRowEmpty( XmlNodeRef& xmlRow ) const;

	protected:

		/*$1- Props --------------------------------------------------------------*/
		bool				m_bInfoLoaded;
		CDagGrid		m_Grid;
		CDagNode*		m_pRoot;
		string			m_sConfFile;
		bool				m_bSkipEmptyRows;
};
#endif // _DAG_H_
