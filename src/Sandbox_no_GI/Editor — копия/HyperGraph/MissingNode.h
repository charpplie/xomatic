#ifndef __MissingNode_h__
#define __MissingNode_h__

#include "HyperGraphNode.h"

#define MISSING_NODE_CLASS ( "MissingNode" )

class CMissingNode : public CHyperNode
{
public:
	CMissingNode( const char* sMissingClassName );
	virtual ~CMissingNode();

	virtual const char* GetClassName() const { return MISSING_NODE_CLASS; };

	virtual void Init() {};
	virtual CHyperNode* Clone();

	virtual void Serialize( XmlNodeRef &node, bool bLoading, CObjectArchive* ar = 0 );
	virtual CHyperNodePort* FindPort( const char *portname, bool bInput );
	virtual bool IsEditorSpecialNode() { return true; }
	virtual bool IsFlowNode() { return false; }

private:
	CString m_sMissingClassName;
	CString m_sMissingName;
	CString m_sGraphEntity;
	GUID m_entityGuid;
	int m_iOrgFlags;
};

#endif // #ifndef __MissingNode_h__