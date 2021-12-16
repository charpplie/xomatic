#ifndef __COMMENTNODEBOX_H__
#define __COMMENTNODEBOX_H__

#pragma once

#include "HyperGraphNode.h"

class CCommentBoxNode : public CHyperNode
{
public:
	static const char* GetClassType() {
		return "_commentbox";
	}
	CCommentBoxNode();
	~CCommentBoxNode();
	void Init();
	void Done();
	CHyperNode * Clone();
	virtual void Serialize( XmlNodeRef &node,bool bLoading, CObjectArchive* ar);
	
	virtual void SetResizeBorderRect( const Gdiplus::RectF& newRelBordersRect );
	virtual void SetBorderRect( const Gdiplus::RectF& newAbsBordersRect );
	virtual bool IsGridBound() { return false; }
	virtual const Gdiplus::RectF* GetResizeBorderRect() const 
	{ 
		return &m_resizeBorderRect;
	}
	virtual void OnZoomChange( float zoom );
	virtual void OnInputsChanged();
	virtual bool IsEditorSpecialNode() { return true; }
	virtual bool IsFlowNode() { return false; }
	virtual bool IsTooltipShowable() { return false; }


private:
	void OnPossibleSizeChange();
	
	
private:
	Gdiplus::RectF m_resizeBorderRect;
};

#endif
