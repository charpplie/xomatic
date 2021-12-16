// MemReplayDoc.h : interface of the CMemReplayDoc class
//


#pragma once

#include "ReplaySDK/IReplayDatabaseConnection.h"

class CMemReplayView;
class ContextTree;

class CMemReplayDoc : public CDocument
{
protected: // create from serialization only
	CMemReplayDoc();
	DECLARE_DYNCREATE(CMemReplayDoc)

// Attributes
public:
	SharedPtr<IReplayDatabaseConnection> GetConnection() const { return m_sdkDoc; }

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void OnFileSaveAs();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CMemReplayDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CMemReplayView* GetView();

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

private:
	SharedPtr<IReplayDatabaseConnection> m_sdkDoc;
};
