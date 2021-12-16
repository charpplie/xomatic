// (c) 2001-2012 Crytek GmbH
#pragma once

#include "EditorCommonAPI.h"
#include "IEditor.h"
#include "Include/IEditorClassFactory.h"
#include "Include/IViewPane.h"
#include "QtIntegration.h"

namespace Serialization { class IArchive; }
using Serialization::IArchive;

class QWidget;

class EDITOR_COMMON_API CQtViewPaneBase : public CWnd
{
	DECLARE_DYNAMIC(CQtViewPaneBase)
public:

	CQtViewPaneBase(QWidget* widget);
	~CQtViewPaneBase();

	static const char* ClassName() { return "QtViewPane"; }
	static bool RegisterWindowClass();
	static void UnregisterWindowClass();
	virtual void DeleteThis() = 0;
	
	void OnEditorNotifyEvent( EEditorNotifyEvent event );

	DECLARE_MESSAGE_MAP()
protected:
	BOOL PreTranslateMessage(MSG* pMsg) override;
	void PostNcDestroy() override;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* oldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnFrameCanClose(WPARAM wParam, LPARAM lParam);
private:
	QWidget* m_window;
};

template<class TWidget>
class CQtViewPane : public CQtViewPaneBase
{
public: 
	CQtViewPane() : CQtViewPaneBase(new TWidget())	{}
	CRuntimeClass* GetRuntimeClass() const override { return GetThisClass();	}
	static CObject* PASCAL CreateObject()	{ return new CQtViewPane;	}
	static CRuntimeClass* PASCAL GetThisClass() { 
		static CRuntimeClass localClass = { Name(), sizeof(CQtViewPane<TWidget>), 0xffff, &CreateObject, &_GetBaseClass, NULL, NULL }; 
		return &localClass;
	}
	static void SetName(const char* name) { Name() = name; }
	static const char*& Name() { static const char* name = "QtViewPane";  return name; }
	void DeleteThis() override { delete this; }
protected:
		static CRuntimeClass* PASCAL _GetBaseClass() { return CQtViewPaneBase::GetThisClass(); }
};

// ---------------------------------------------------------------------------

template<class TWidget>
class CQtViewClass : public IViewPaneClass
{
public:
	const char* m_name;
	const char* m_category;

	CQtViewClass(const char* name, const char* category) : m_name(name), m_category(category) {}
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	static REFGUID GetClassID() 
	{
		static const GUID guid = 
		{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
		return guid;
	}

	virtual REFGUID ClassID()
	{
		return GetClassID();
	}
	virtual const char* ClassName() { return m_name; };
	virtual const char* Category() { return m_category; };

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CQtViewPane<TWidget>); };
	virtual const char* GetPaneTitle() { return m_name; };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(50,50,1000,800); };
	virtual bool SinglePane() { return false; };
	virtual bool WantIdleUpdate() { return true; };
};

template<class TWidget>
bool RegisterQtViewPane(IEditor* editor, const char* name, const char* category)
{
	if (!InitializeQt(editor))
		return false;
	CQtViewPane<TWidget>::SetName(name);
	GetIEditor()->GetClassFactory()->RegisterClass( new CQtViewClass<TWidget>(name, category) );
	if (!CQtViewPaneBase::RegisterWindowClass())
		return false;
	return true;
}

template<class TWidget>
void UnregisterQtViewPane()
{
	GetIEditor()->GetClassFactory()->UnregisterClass( CQtViewClass<TWidget>::GetClassID() );
	CQtViewPaneBase::UnregisterWindowClass();
	FinalizeQt();
}
