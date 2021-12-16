// ValidationPage.cpp : implementation file
//

#include "stdafx.h"
#include "MemReplay.h"
#include "ValidationPage.h"
#include "MemReplayDoc.h"
#include "MainFrm.h"
#include "MemReplayView.h"
#include "VSInterop.h"

// CValidationPage dialog

IMPLEMENT_DYNAMIC(CValidationPage, CDialog)

CValidationPage::CValidationPage(const SharedPtr<ValidateResult>& validator, const SharedPtr<ISymbolTable>& symbols)
	: CDialog(CValidationPage::IDD)
	, m_validator(validator)
	, m_symbols(symbols)
{
}

CValidationPage::~CValidationPage()
{
}

void CValidationPage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_Usage);
}

void CValidationPage::OnInspectClicked()
{
	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	HTREEITEM selection = tree.GetSelectedItem();
	if (selection)
	{
		TAddress cs = static_cast<TAddress>(tree.GetItemData(selection));
		if (cs)
		{
			const char* name;
			const char* filename;
			int line;
			m_symbols->GetSymbol(cs, name, filename, line);

			VSInterop::OpenFileAtLine(filename, line);
		}
	}
}

void CValidationPage::OnFindNext(const char* searchString, u32 flags)
{
}

void CValidationPage::Export(ExcelExport& xls)
{
}

void CValidationPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CValidationPage, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CValidationPage message handlers

BOOL CValidationPage::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	m_tree.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 16, 16), this, ID_TREE);

	DWORD dwStyle = GetWindowLong(m_tree.GetTreeCtrl().m_hWnd, GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS;
	SetWindowLong(m_tree.GetTreeCtrl().m_hWnd, GWL_STYLE, dwStyle);

	CHeaderCtrl& header = m_tree.GetHeaderCtrl();

	while (header.DeleteItem(0))
		;

	HDITEM hditem;
	hditem.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	hditem.fmt = HDF_LEFT | HDF_STRING;
	hditem.cxy = 640;
	hditem.pszText = "Function";
	header.InsertItem(0, &hditem);

	hditem.cxy = 200;
	hditem.pszText = "File";
	header.InsertItem(1, &hditem);

	hditem.cxy = 50;
	hditem.pszText = "Line";
	header.InsertItem(2, &hditem);

	m_tree.UpdateColumns();

	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	size_t index = 0;

	for (ValidateErrorVector::const_iterator it = m_validator->errors.begin(), itEnd = m_validator->errors.end();
		it != itEnd && index < 10000;
		++ it, ++ index)
	{
		const ValidateError& err = *it;

		TCHAR msg[256];
		TCHAR evA[64], evB[64];
		size_t cs = (size_t)-1;
		size_t cs2 = (size_t)-1;

		switch (err.id)
		{
		case ValidateError::ERR_DoubleAlloc:
			_stprintf_s(msg, 256, _T("Double allocating pointer %08x, was %Iu bytes at %s on thread %08x now %Iu bytes at %s on thread %08x."),
				err.a.ptr, err.a.len, FormatThousands(evA, 64, err.a.allocEv), err.a.thread,
				err.b.len, FormatThousands(evB, 64, err.b.allocEv), err.b.thread);
			cs = err.a.cs;
			cs2 = err.b.cs;
			break;

		case ValidateError::ERR_DoubleFree:
			_stprintf_s(msg, 256, _T("Double freeing pointer %08x, first freed at %s on thread %08x now again at %s on thread %08x."),
				err.a.ptr, FormatThousands(evA, 64, err.a.allocEv), err.a.thread,
				FormatThousands(evB, 64, err.b.allocEv), err.b.thread);
			cs = err.a.cs;
			break;

		case ValidateError::ERR_OffsetFree:
			_stprintf_s(msg, 256, _T("Freeing unknown pointer %08x, nearest allocation at %08x, %Iu bytes\n"), err.a.ptr, err.b.ptr, err.b.len);
			cs = err.b.cs;
			break;

		default:
			_stprintf_s(msg, 256, _T("Unknown error.\n"));
			break;
		}

		HTREEITEM error = tree.InsertItem(msg);
		HTREEITEM callstackItem = error;

		if (cs != (size_t)-1 && cs2 != (size_t)-1)
		{
			callstackItem = tree.InsertItem("First callstack", error);
		}

		const ISymbolTable& symbols = *m_symbols;

		if (cs != (size_t)-1)
		{
			TAddress callstack[256];
			size_t csSize = 256;
			
			if (m_validator->callstacks.TryFindCallstack(cs, callstack, csSize))
			{
				for (size_t i = 0; i < csSize; ++ i)
				{
					const char* name = "";
					const char* filename = "";
					int line = 0;
					symbols.GetSymbol(callstack[i], name, filename, line);

					TCHAR csStr[1024];
					_stprintf_s(csStr, 1024, "%hs\t%hs\t%i", name, filename, line);

					HTREEITEM csTreeItem = tree.InsertItem(csStr, callstackItem);
					tree.SetItemData(csTreeItem, static_cast<DWORD_PTR>(callstack[i]));
				}
			}
		}
		
		if (cs != (size_t)-1 && cs2 != (size_t)-1)
		{
			callstackItem = tree.InsertItem("Second callstack", error);
		}
		
		if (cs2 != (size_t)-1)
		{
			TAddress callstack[256];
			size_t csSize = 256;
			
			if (m_validator->callstacks.TryFindCallstack(cs2, callstack, csSize))
			{
				for (size_t i = 0; i < csSize; ++ i)
				{
					const char* name = "";
					const char* filename = "";
					int line = 0;
					symbols.GetSymbol(callstack[i], name, filename, line);

					TCHAR csStr[1024];
					_stprintf_s(csStr, 1024, "%hs\t%hs\t%i", name, filename, line);

					HTREEITEM csTreeItem = tree.InsertItem(csStr, callstackItem);
					tree.SetItemData(csTreeItem, static_cast<DWORD_PTR>(callstack[i]));
				}
			}
		}
	}

	SetWindowText(_T("Validation"));

	return result;
}

void CValidationPage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (GetSafeHwnd())
	{
		CRect rcClient;
		GetClientRect(&rcClient);

		m_tree.MoveWindow(rcClient);
	}
}
