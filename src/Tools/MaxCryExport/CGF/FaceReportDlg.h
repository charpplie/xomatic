//---------------------------------------------------------------------------
// Copyright Crytek GmbH 2005
// Dialog for reporting a list of faces.
//---------------------------------------------------------------------------

#include <Windows.h>
#include <vector>
#include <string>
#include "resource.h"

class FaceReportDlg
{
public:
	enum {IDD = IDD_FACEREPORT};

	FaceReportDlg();
	void DoModal(HWND hWndParent);

	void AddFace(int nID, float fX, float fY, float fZ);
	void SetMessage(const std::string& sMessage);
	void SetCaption(const std::string& sCaption);
	void SetNode(INode* pNode);

private:
	class Position
	{
	public:
		float fX;
		float fY;
		float fZ;
	};

	class Face
	{
	public:
		int nIndex;
		Position pos;
	};

	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void UpdateControls();
	std::string GetFaceText();
	int OnInitDialog();
	int OnSize();
	void OnSelectDegenerateFaces();

	HWND m_hDlg;
	HWND m_hMessage;
	HWND m_hText;
	HWND m_hOk;
	HWND m_hSelect;
	HWND m_hIcon;
	std::vector<Face> m_vFaces;
	std::string m_sMessage;
	std::string m_sCaption;
	INode* m_pNode;
};
