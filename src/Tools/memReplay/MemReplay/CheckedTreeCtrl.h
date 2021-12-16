#pragma once

class CCheckedTreeCtrl : public CTreeCtrl
{
public:
	enum
	{
		CTC_CHECKCHANGED = WM_USER + 1
	};

	struct CheckChanged
	{
		NMHDR hdr;
		HTREEITEM hItem;
	};

protected:
	DECLARE_MESSAGE_MAP();

	void OnLButtonUp(UINT nFlags, CPoint point);

private:
	void RaiseCheckChanged(HTREEITEM item);
};
