#pragma once

#include "../SimpleIndexedMesh.h"												// CSimpleIndexedMesh
#include <vector>																				// STL vector<>
#include <string>																				// STL string


class CPreviewInput : public CDialog
{
	DECLARE_DYNAMIC(CPreviewInput)

public:
  // standard constructor
	// Arguments:
	//   fRayLengthInOS - ray length for ray shooting in units
	CPreviewInput( CWnd* pParent = NULL);
	virtual ~CPreviewInput();

	void SetRayLengthInOS( const float fRayLengthInOS );

// Dialog Data
	enum { IDD = IDD_PREVIEWINPUT };

	CSimpleIndexedMesh &CreateMeshEntry( const uint32 dwColor, const char *szType, const bool bLines );

protected: // ----------------------------------------------------------

	struct SMeshEntry
	{
		// constructor
		SMeshEntry( const uint32 dwColor, const char *szType, const bool bLines ) :m_dwColor(dwColor), m_sType(szType), m_bLines(bLines)
		{
		}

		CSimpleIndexedMesh 		m_Mesh;
		uint32								m_dwColor;
		std::string						m_sType;
		bool									m_bLines;									// false=Points, true=Lines
	};

	std::vector<SMeshEntry *>		m_MeshEntries;				// pointer to avoid problems with the compy constructur


	virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

	float									m_fRayLengthInOS;						// ray length for ray shooting in units
	Vec3									m_vMin;											// (FLT_MAX,FLT_MAX,FLT_MAX) if not set yet
	Vec3									m_vMax;											// (-FLT_MAX,-FLT_MAX,-FLT_MAX) if not set yet

	Vec3									m_vOffset;									// set by RecalcLayout
	Vec3									m_vScale;										// set by RecalcLayout
	bool									m_bIsResizing;							//

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	void DrawMesh_Lines( CDC &dc, const CSimpleIndexedMesh &rMesh, const uint32 dwTriColor, const uint32 dwNormalColor );
	
	void DrawMesh_Points( CDC &dc, const CSimpleIndexedMesh &rMesh, const uint32 dwColor );

	void ExtendMinMax( CSimpleIndexedMesh &rMesh );
	
	LRESULT OnEnterSizeMove( WPARAM wparam, LPARAM lparam );

	LRESULT OnExitSizeMove( WPARAM wparam, LPARAM lparam );

	void RecalcLayout();

	CEdit m_status;
	CStatic m_canvas;
};
