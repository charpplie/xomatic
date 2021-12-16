/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   dag_node.h
$Id$
$DateTime$
Description: Small class to manage a single DAG tree node. It is
expected that you hinerit from this class to use your specific
implementation, as Behavior Tree code does.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu

*********************************************************************/
#ifndef _DAG_NODE_H_
  #define _DAG_NODE_H_

class CDagNode
{

  public:

    /*$1- Basics -------------------------------------------------------------*/
    CDagNode( CDagNode* pParent, const char* sName );
    virtual ~CDagNode();

    /*$1- Util ---------------------------------------------------------------*/
    virtual CDagNode*     AddChild( const char* sName );
    virtual void          DestroyAllChilds();
    CDagNode*             SearchNode( const char* sName, bool bRecursively = true );
    const char*           GetName() const;
    void                  SetName( const char* sName );
    virtual const char*   GetDebugText() const;
    CDagNode*             GetParent();
    CDagNode const*       GetParent() const;
		uint32									GetChildCount( bool bDepthOfOne = true ) const;
		CDagNode*             GetChild( uint32 nIndex );
		CDagNode const*       GetChild( uint32 nIndex ) const;

  protected:

    /*$1- Members ------------------------------------------------------------*/
    string                  m_sName;
    CDagNode*               m_pParent;
    std::vector<CDagNode *> m_vecChilds;
};

/*$1- Inline methods implementation --------------------------------------*/
inline CDagNode* CDagNode::GetChild( uint32 nIndex )
{
	return m_vecChilds[ nIndex ];
}

inline CDagNode const* CDagNode::GetChild( uint32 nIndex ) const
{
	return m_vecChilds[ nIndex ];
}

#endif // _DAG_NODE_H_
