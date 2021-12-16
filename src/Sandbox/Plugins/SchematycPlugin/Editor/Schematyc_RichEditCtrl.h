/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc rich edit control.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_CUSTOMRICHEDITCTRL_H__
#define __SCHEMATYC_CUSTOMRICHEDITCTRL_H__

namespace Schematyc
{
	class CCustomRichEditCtrl : public CRichEditCtrl
	{
	public:

		virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

		void SetText(const char* text);
		void AppendText(const char* text);
	};
}

#endif //__SCHEMATYC_CUSTOMRICHEDITCTRL_H__
