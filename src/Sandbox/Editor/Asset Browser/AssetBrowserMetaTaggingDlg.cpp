////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Asset Browser/AssetBrowserMetaTaggingDlg.h"
#include "Asset Browser/AssetBrowserManager.h"
#include "Include/IAssetItem.h"

IMPLEMENT_DYNAMIC(CAutoCompleteEdit, CEdit)

CAutoCompleteEdit::CAutoCompleteEdit() : CEdit()
{
}

CAutoCompleteEdit::~CAutoCompleteEdit()
{
}

///////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CAutoCompleteEdit, CEdit)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

///////////////////////////////////////////////////////

void CAutoCompleteEdit::OnKeyDown(UINT nChar, UINT nRepCnt,	UINT nFlags)
{
	switch (nChar)
	{
	case VK_DELETE:
	{
		CString text;
		int start = 0, end = 0;
		bool setSel = false;

		GetWindowText(text);
		GetSel(start, end);
		CString left = text.Left(start);

		int idx = -1;

		if (start != end)
		{
			idx = text.GetLength() - end;

			if (idx != 0)
			{
				setSel = true;
			}
		}
		else
		{
			if (start != text.GetLength() - 1)
			{
				idx = text.GetLength() - (start + 1);
				setSel = true;
			}
		}

		if (idx != -1)
		{
			CString right = text.Right(idx);
			left.Append(right);
		}

		SetWindowText(left);

		if (setSel)
		{
			SetSel(start, start);
		}

		return;
	}
	break;

	case VK_BACK:
	{
		CString text;
		int start = 0, end = 0;
		bool setSel = false;

		GetWindowText(text);
		GetSel(start, end);


		CString left = text.Left(start - 1);
		CString right = text.Right(text.GetLength() - end);
		CString newString = left;

		if (start == text.GetLength())
		{
			return;
		}
		else
		{
			if (end != text.GetLength())
			{
				if (start != end)
				{
					return;
				}
				else
				{
					newString = text.Left(start);
					newString.Append(right);
					setSel = true;
				}
			}
		}

		SetWindowText(newString);

		if (setSel)
		{
			SetSel(start - 1, start);
		}

		return;
	}
	break;
	}

	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}

// CAssetBrowserMetaTagging dialog

IMPLEMENT_DYNAMIC(CAssetBrowserMetaTaggingDlg, CDialog)

CAssetBrowserMetaTaggingDlg::CAssetBrowserMetaTaggingDlg(TAssetItems* assetlist, CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserMetaTaggingDlg::IDD, pParent)
{
	for (TAssetItems::iterator item = assetlist->begin(), end = assetlist->end(); item != end; ++item)
	{
		CString filename;
		filename.Format("%s%s", (*item)->GetRelativePath(), (*item)->GetFilename());
		m_assetList.push_back(filename);
	}
}


CAssetBrowserMetaTaggingDlg::CAssetBrowserMetaTaggingDlg(const CString& filename, CWnd* pParent)
	: CDialog(CAssetBrowserMetaTaggingDlg::IDD, pParent)
{
	m_assetList.push_back(filename);
}

CAssetBrowserMetaTaggingDlg::~CAssetBrowserMetaTaggingDlg()
{
	m_staticControls.erase(m_staticControls.begin(), m_staticControls.end());
	m_comboControls.erase(m_comboControls.begin(), m_comboControls.end());
}

void CAssetBrowserMetaTaggingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ASSET_TAGGING_DESCRIPTION, m_descriptionEdit);
	DDX_Control(pDX, IDC_FILENAME_STATIC, m_filenameEdit);
}


BEGIN_MESSAGE_MAP(CAssetBrowserMetaTaggingDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnOK)
	ON_EN_CHANGE(IDC_ASSET_TAGGING_DESCRIPTION, OnEditChanged)
END_MESSAGE_MAP()


// CAssetBrowserMetaTagging message handlers
BOOL CAssetBrowserMetaTaggingDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_bAttemptAutocomplete = true;

	if (m_assetList.size() != 1)
	{
		return TRUE;
	}

	CWnd* pGroup = GetDlgItem(IDC_CATEGORIES_GROUP);

	if (!pGroup)
	{
		return TRUE;
	}

	CWnd* pOkay = GetDlgItem(IDOK);

	if (!pOkay)
	{
		return TRUE;
	}

	CString assetDescription;
	CString filename = m_assetList[0];

	if (CAssetBrowserManager::Instance()->GetAssetDescription(filename, assetDescription) && assetDescription.GetLength() > 0)
	{
		m_descriptionEdit.SetWindowText(assetDescription);
	}

	m_filenameEdit.SetWindowText(filename);

	std::vector<CString> categories;
	CAssetBrowserManager::Instance()->GetAllTagCategories(categories);

	RECT clentRectGroup;
	pGroup->GetClientRect(&clentRectGroup);

	const int kInitialXOffset = 17;
	const int kControlHeight = 18;
	const int kStaticControlWidth = 100;
	const int kControlBorder = 7;
	const int kInitialGroupOffset = 130;
	const int kInitialYOffset = kInitialGroupOffset + 25;

	int currenty = kInitialYOffset;
	int currentx = kInitialXOffset;
	int count = 1;

	auto pAssetBrowserMgr = CAssetBrowserManager::Instance();

	for (std::vector<CString>::iterator item = categories.begin(), end = categories.end(); item != end; ++item)
	{
		CStatic* pStatic = new CStatic();
		CComboBox* pComboBox = new CComboBox();

		pStatic->Create((*item), WS_CHILD | WS_VISIBLE | SS_RIGHT , CRect(currentx, currenty, currentx + kStaticControlWidth, currenty + kControlHeight), this, WM_USER + count);
		++count;

		pComboBox->Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS, CRect(currentx + kStaticControlWidth + kControlBorder, currenty, clentRectGroup.right - 5, currenty + kControlHeight), this, WM_USER + count);
		++count;

		CString categoryTag;
		pAssetBrowserMgr->GetTagForAssetInCategory(categoryTag, filename, (*item));

		std::vector<CString> tags;
		pAssetBrowserMgr->GetTagsForCategory((*item), tags);

		int count = 0;

		for (std::vector<CString>::iterator itemtag = tags.begin(), endtag = tags.end(); itemtag != endtag; ++itemtag)
		{
			pComboBox->InsertString(count, (*itemtag));

			if ((*itemtag).CompareNoCase(categoryTag) == 0)
			{
				pComboBox->SetCurSel(count);
			}

			count++;
		}

		m_staticControls.push_back(pStatic);
		m_comboControls.push_back(pComboBox);
		currenty += kControlHeight + kControlBorder;
	}

	RECT mainRect, mainClientRect;
	GetWindowRect(&mainRect);
	int height = mainRect.bottom - mainRect.top;
	height += currenty - kInitialYOffset;
	SetWindowPos(0, mainRect.left, mainRect.top, mainRect.right - mainRect.left, height, 0);
	GetClientRect(&mainClientRect);

	RECT buttonRect;
	pOkay->GetWindowRect(&buttonRect);
	int buttonWidth = buttonRect.right - buttonRect.left;

	pOkay->MoveWindow(mainClientRect.right - kControlBorder - buttonWidth, mainClientRect.bottom - 22, buttonWidth, buttonRect.bottom - buttonRect.top, true);
	pGroup->MoveWindow(kControlBorder, kInitialGroupOffset, mainClientRect.right - kControlBorder * 2, mainClientRect.bottom - kInitialGroupOffset - 25, true);

	return TRUE;
}

void CAssetBrowserMetaTaggingDlg::OnOK()
{
	if (m_assetList.size() != 1)
	{
		return;
	}

	if (Validated() == false)
	{
		CString errorMsg("Please ensure you have filled out the description and selected a tag from each category.");
		AfxMessageBox(errorMsg);
		return;
	}

	auto pAssetBrowserMgr = CAssetBrowserManager::Instance();
	CString filename = m_assetList[0];
	CString description;
	m_descriptionEdit.GetWindowText(description);
	pAssetBrowserMgr->SetAssetDescription(filename, description);

	int count = m_comboControls.size();

	for (int idx = 0; idx < count; ++idx)
	{
		CString categoryText, tagText;

		m_staticControls[idx]->GetWindowTextA(categoryText);
		pAssetBrowserMgr->GetTagForAssetInCategory(tagText, filename, categoryText);
		pAssetBrowserMgr->RemoveTagFromAsset(tagText, categoryText, filename);

		m_comboControls[idx]->GetWindowText(tagText);
		pAssetBrowserMgr->AddTagsToAsset(filename, tagText, categoryText);
	}

	__super::OnOK();
}

bool CAssetBrowserMetaTaggingDlg::Validated()
{
	CString description;
	m_descriptionEdit.GetWindowText(description);

	if (description.IsEmpty())
	{
		return false;
	}

	int count = m_comboControls.size();

	for (int idx = 0; idx < count; ++idx)
	{
		CString categoryTag;
		m_comboControls[idx]->GetWindowText(categoryTag);

		if (categoryTag.IsEmpty())
		{
			return false;
		}
	}

	return true;
}

void CAssetBrowserMetaTaggingDlg::OnEditChanged()
{
	CString description, fullDescription;
	m_descriptionEdit.GetWindowText(description);

	if (description.GetLength() > 0 && m_bAttemptAutocomplete)
	{
		int start = 0, end = 0;

		if (CAssetBrowserManager::Instance()->GetAutocompleteDescription(description, fullDescription))
		{
			m_bAttemptAutocomplete = false;
			m_descriptionEdit.SetWindowText(fullDescription);
			start = description.GetLength();
			end = fullDescription.GetLength();
			m_descriptionEdit.SetSel(start, end);
			m_bAttemptAutocomplete = true;
		}
	}
}