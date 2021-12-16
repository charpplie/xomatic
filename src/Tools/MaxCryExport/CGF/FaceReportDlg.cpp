//---------------------------------------------------------------------------
// Copyright Crytek GmbH 2005
// Dialog for reporting a list of faces.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "FaceReportDlg.h"
#include "resource.h"
#include <sstream>
#include <iomanip>
#include <set>
#include "SelectionUtils.h"

//---------------------------------------------------------------------------
FaceReportDlg::FaceReportDlg()
:	m_hDlg(0),
	m_hMessage(0),
	m_hText(0),
	m_hOk(0),
	m_hIcon(0),
	m_pNode(0)
{
}

//---------------------------------------------------------------------------
extern HINSTANCE hInstance;
void FaceReportDlg::DoModal(HWND hWndParent)
{
	// Create a dialog.
	DialogBoxParam(
		hInstance,															// hInstance,
		MAKEINTRESOURCE(IDD_FACEREPORT),				// lpTemplateName,
		hWndParent,															// hWndParent,
		DialogProc,															// lpDialogFunc,
		(LPARAM)this);													// dwInitParam);
}

//---------------------------------------------------------------------------
void FaceReportDlg::AddFace(int nIndex, float fX, float fY, float fZ)
{
	Face face;
	face.pos.fX = fX;
	face.pos.fY = fY;
	face.pos.fZ = fZ;
	face.nIndex = nIndex;
	m_vFaces.push_back(face);
}

//---------------------------------------------------------------------------
void FaceReportDlg::SetMessage(const std::string& sMessage)
{
	m_sMessage = sMessage;
}

//---------------------------------------------------------------------------
void FaceReportDlg::SetCaption(const std::string& sCaption)
{
	m_sCaption = sCaption;
}

//---------------------------------------------------------------------------
INT_PTR CALLBACK FaceReportDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Get the class instance pointer.
	FaceReportDlg* pThis = 0;
	if (uMsg == WM_INITDIALOG)
	{
#if !defined( _WIN64 )
		// SetWindowLongPtr() maps to SetWindowLong() in 32 bit land; react accordingly to 
		// keep the compiler happy, even with /Wp64.
		::SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG)((LONG_PTR)(lParam)));
#else
		::SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)(lParam));
#endif

		pThis = (FaceReportDlg*)lParam;
		pThis->m_hDlg = hwndDlg;
		pThis->m_hMessage = GetDlgItem(hwndDlg, IDC_MESSAGE);
		pThis->m_hText = GetDlgItem(hwndDlg, IDC_TEXT);
		pThis->m_hOk = GetDlgItem(hwndDlg, IDOK);
		pThis->m_hSelect = GetDlgItem(hwndDlg, IDC_SELECT_DEGENERATE_FACES);
	}
	else
	{
		pThis = (FaceReportDlg*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	}

	// Handle the message.
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return pThis->OnInitDialog();
	case WM_SIZE:
		pThis->OnSize();
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, IDOK);
			break;
		case IDC_SELECT_DEGENERATE_FACES:
			pThis->OnSelectDegenerateFaces();
			break;
		}
		return TRUE;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDCANCEL);
		return TRUE;
	default:
		return FALSE;
	}
}

//---------------------------------------------------------------------------
void FaceReportDlg::UpdateControls()
{
	RECT clientRect;
	GetClientRect(m_hDlg, &clientRect);
	int nDialogWidth = clientRect.right - clientRect.left;
	int nDialogHeight = clientRect.bottom - clientRect.top;
	RECT messageRect;
	GetWindowRect(m_hMessage, &messageRect);
	//int nMessageWidth = messageRect.right - messageRect.left;
	int nMessageHeight = messageRect.bottom - messageRect.top;
	RECT okRect;
	GetWindowRect(m_hOk, &okRect);
	int nOkWidth = okRect.right - okRect.left;
	int nOkHeight = okRect.bottom - okRect.top;
	RECT selectRect;
	GetWindowRect(m_hSelect, &selectRect);
	int nSelectWidth = selectRect.right - selectRect.left;
	int nSelectHeight = selectRect.bottom - selectRect.top;
	RECT iconRect;
	GetWindowRect(m_hIcon, &iconRect);
	int nIconWidth = iconRect.right - iconRect.left;
	int nIconHeight = iconRect.bottom - iconRect.top;

	static const int sk_nPadding = 5;

	// Position the icon at the top left of the dialog.
	MoveWindow(m_hIcon, clientRect.left, clientRect.top, nIconWidth, nIconHeight, TRUE);

	// Position the message at the top of the dialog.
	MoveWindow(m_hMessage, clientRect.left + nIconWidth + sk_nPadding, clientRect.top, nDialogWidth - nIconWidth - sk_nPadding, nMessageHeight, TRUE);

	// Position the ok button at the bottom right of the dialog.
	MoveWindow(m_hOk, clientRect.right - nOkWidth, clientRect.bottom - nOkHeight, nOkWidth, nOkHeight, TRUE);

	// Position the select button at the bottom left of the dialog.
	MoveWindow(m_hSelect, 0, clientRect.bottom - nOkHeight, nSelectWidth, nOkHeight, TRUE);

	// Position the text in the middle of the dialog.
	MoveWindow(m_hText, clientRect.left, clientRect.top + nIconHeight + sk_nPadding, clientRect.right, clientRect.bottom - nOkHeight - nIconHeight - 2 * sk_nPadding, TRUE);
}

//---------------------------------------------------------------------------
std::string FaceReportDlg::GetFaceText()
{
	std::ostringstream output;

	for (int nFace = 0; nFace < m_vFaces.size(); ++nFace)
	{
		const Face& face = m_vFaces[nFace];

		output << std::setw(12) << std::setiosflags(std::ios_base::fixed | std::ios_base::right) << std::setprecision(6) << face.pos.fX;
		output << ",";
		output << std::setw(12) << std::setiosflags(std::ios_base::fixed | std::ios_base::right) << std::setprecision(6) << face.pos.fY;
		output << ",";
		output << std::setw(12) << std::setiosflags(std::ios_base::fixed | std::ios_base::right) << std::setprecision(6) << face.pos.fZ;
		output << "\r\n";
	}
	return output.str();
}

//---------------------------------------------------------------------------
int FaceReportDlg::OnInitDialog()
{
	m_hIcon = CreateWindowEx(0,
		TEXT("STATIC"),
		NULL,
		WS_CHILD|WS_VISIBLE|SS_ICON,
		10,
		80,
		40,
		40,
		m_hDlg,
		NULL,
		hInstance,
		NULL);

	SendMessage(m_hDlg, WM_SETTEXT, 0, (LPARAM)m_sCaption.c_str());
	SendMessage(m_hMessage, WM_SETTEXT, 0, (LPARAM)m_sMessage.c_str());
	SendMessage(m_hText, WM_SETTEXT, 0, (LPARAM)GetFaceText().c_str());
	SendMessage(m_hIcon, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(0, MAKEINTRESOURCE(IDI_ERROR)));
	int nIconSizeX = GetSystemMetrics(SM_CXICON);
	int nIconSizeY = GetSystemMetrics(SM_CYICON);
	MoveWindow(m_hIcon, 0, 0, nIconSizeX, nIconSizeY, FALSE);

	UpdateControls();

	// If there is no node, then hide the select button.
	if (m_pNode == 0)
		ShowWindow(m_hSelect, SW_HIDE);

	return TRUE;
}

//---------------------------------------------------------------------------
int FaceReportDlg::OnSize()
{
	UpdateControls();
	return 0;
}

//---------------------------------------------------------------------------
void FaceReportDlg::OnSelectDegenerateFaces()
{
	// Create a set of all the faces.
	std::set<int> degenerates;
	for (std::vector<Face>::iterator itFace = m_vFaces.begin(); itFace != m_vFaces.end(); ++itFace)
		degenerates.insert((*itFace).nIndex);

	// Select the faces.
	std::string sError;
	switch (SelectionUtils::SelectFaces(m_pNode->GetObjectRef(), degenerates, &sError))
	{
	case SelectionUtils::Result_Success:
		MessageBox(m_hDlg, "Faces selected - go to face selection mode to view.", "Unable to select faces", MB_ICONINFORMATION | MB_OK);
		break;
	case SelectionUtils::Result_CannotFindMesh:
		switch (MessageBox(m_hDlg, "Cannot select faces: An edit mesh modifier is required to select faces - do you want to add one?", "Unable to select faces", MB_ICONQUESTION | MB_YESNO))
		{
		case IDYES:
			switch (SelectionUtils::CreateMeshSelectWrapper(m_pNode, &sError))
			{
			case SelectionUtils::Result_Success:
				if (SelectionUtils::Result_Success == SelectionUtils::SelectFaces(m_pNode->GetObjectRef(), degenerates, &sError))
				{
					MessageBox(m_hDlg, "Faces selected - go to face selection mode to view.", "Unable to select faces", MB_ICONINFORMATION | MB_OK);
				}
				else
				{
					std::string sMessage = "Cannot select faces: " + sError;
					MessageBox(m_hDlg, sMessage.c_str(), "Unable to select faces", MB_ICONWARNING | MB_OK);
				}
				break;
			case SelectionUtils::Result_CannotAddMeshWrapper:
				std::string sMessage = "Cannot add edit mesh: " + sError;
				MessageBox(m_hDlg, sMessage.c_str(), "Unable to add edit mesh", MB_ICONERROR | MB_OK);
				break;
			}
			break;
		case IDNO:
			break;
		}
		break;
	default:
		std::string sMessage = "Cannot select faces: " + sError;
		MessageBox(m_hDlg, sMessage.c_str(), "Unable to select faces", MB_ICONWARNING | MB_OK);
		break;
	}
}

//---------------------------------------------------------------------------
void FaceReportDlg::SetNode(INode* pNode)
{
	m_pNode = pNode;
}