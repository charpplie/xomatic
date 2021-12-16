#pragma once

#include "SRFData.h"										// CSRFData
#include "../Properties.h"							// CProperties
#include "../PolyBumpWorkerThread.h"		// CPolyBumpWorkerThread

class CPolyBumpApplicationDoc : public CDocument
{
protected: // create from serialization only
	CPolyBumpApplicationDoc();
	DECLARE_DYNCREATE(CPolyBumpApplicationDoc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);


	// Implementation
public:
	virtual ~CPolyBumpApplicationDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

//	void OnUserIdle();

protected: // ----------------------------------------------------------------

	CPolyBumpWorkerThread		m_WorkerThreadData;		//!<
	bool										m_bUpdateOnIdle;			//!< true if computation is in progress

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDocumentComputation();


	friend class CPolyBumpApplicationView;
	afx_msg void OnDocumentExportdata();
	afx_msg void OnDocumentStartcomputation();
};


