#pragma once

#include "ITabPage.h"
#include "DistributionMapWnd.h"
#include "ColumnTreeWnd.h"

#include "ReplaySDK/ISymbolTable.h"
#include "ReplaySDK/ValidateError.h"

class ReplayValidator;

class CValidationPage : public CDialog, public ITabPage
{
	DECLARE_DYNAMIC(CValidationPage)

public:
	explicit CValidationPage(const SharedPtr<ValidateResult>& validator, const SharedPtr<ISymbolTable>& symbols);
	virtual ~CValidationPage();

// Dialog Data
	enum { IDD = IDD_VALIDATIONPAGE };

public: // ITabPage Members
	void OnTabActivate();

	void OnInspectClicked();
	void SetMemoryType(int type) {}
	int GetMemoryType() const { return MEMTYPE_COMBINED; }

	void OnFindNext(const char* searchString, u32 flags);
	void Export(ExcelExport& xls);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

private:
	enum
	{
		ID_TREE = 1,
	};

private:
	SharedPtr<ValidateResult> m_validator;
	SharedPtr<ISymbolTable> m_symbols;

	CColumnTreeWnd m_tree;
};
