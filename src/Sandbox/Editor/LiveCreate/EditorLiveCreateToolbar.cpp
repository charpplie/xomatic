#include "StdAfx.h"
#include "IconManager.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "LiveCreate/EditorLiveCreateToolbar.h"

#ifndef NO_LIVECREATE

//-----------------------------------------------------------------------------

class CLiveCreateToolbarButton : public CXTPControlButton
{
protected:
	CBitmap* m_pNormalBitmap;
	CBitmap* m_pDisabledBitmap;
	CBitmap* m_pActiveBitmap;

public:
	CLiveCreateToolbarButton(const char* szIconPath)
		: CXTPControlButton()
	{
		bool alpha;
		m_pNormalBitmap = GetIEditor()->GetIconManager()->GetIconBitmap(szIconPath, alpha, eIconEffect_ColorEnabled);
		m_pDisabledBitmap = GetIEditor()->GetIconManager()->GetIconBitmap(szIconPath, alpha, eIconEffect_ColorDisabled);
		m_pActiveBitmap = GetIEditor()->GetIconManager()->GetIconBitmap(szIconPath, alpha, eIconEffect_TintGreen);
	}

	virtual ~CLiveCreateToolbarButton()
	{
		SAFE_DELETE(m_pNormalBitmap);
		SAFE_DELETE(m_pDisabledBitmap);
		SAFE_DELETE(m_pActiveBitmap);
	}

	/*virtual BOOL IsTransparent() const
	{
		return TRUE;
	}*/
	
	virtual CSize GetSize(CDC* pDC)
	{
		return CSize(40,40);
	}

	virtual CBitmap* SelectIcon()
	{
		if (GetEnabled())
		{
			if (GetChecked())
			{
				return m_pActiveBitmap;
			}
			else
			{
				return m_pNormalBitmap;
			}
		}
		else
		{
			return m_pDisabledBitmap;
		}
	}

	virtual void Draw(CDC* pDC)
	{
		CXTPControl::Draw(pDC);

		CRect rect = GetRect();
		rect.InflateRect(-4,-4,-4,-4);

		DrawBitmap(pDC, SelectIcon(), rect.left, rect.top);
		//pDC->FillSolidRect(rect, m_color);
	}

	void RequestRedraw()
	{
		CXTPControl::RedrawParent();
	}

private:
	static void DrawBitmap(CDC* pDC, CBitmap* pBitmap, int x, int y)
	{
		CDC mdc;
		mdc.CreateCompatibleDC(pDC);
		mdc.SelectObject(*pBitmap);

		// bitmap size
		const CSize sz(32,32);

		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 255;
		bf.AlphaFormat = AC_SRC_ALPHA;
		pDC->AlphaBlend(x, y, sz.cx, sz.cy, &mdc, 0, 0, sz.cx, sz.cy, bf);
	}
};

class CLiveCreateToolbarButton_Main : public CLiveCreateToolbarButton
	, public LiveCreate::CEditorManager::IListener
{
private:
	CBitmap* m_pErrorBitmap;
	CBitmap* m_pStartingBitmap;
	CFont* m_pFont;
	float m_lastProgress;
	bool m_bHasError;

public:
	CLiveCreateToolbarButton_Main()
		: CLiveCreateToolbarButton("Editor/Icons/lc_sync.png")
		, m_lastProgress(-1.0f)
		, m_bHasError(false)
	{
		SetDescription("Toggle LiveCreate");

		bool alpha;
		m_pErrorBitmap = GetIEditor()->GetIconManager()->GetIconBitmap("Editor/Icons/lc_sync.png", alpha, eIconEffect_TintRed);
		m_pStartingBitmap = GetIEditor()->GetIconManager()->GetIconBitmap("Editor/Icons/lc_sync.png", alpha, eIconEffect_TintYellow);

		{
			CFont baseFont;
			baseFont.Attach((HFONT)GetStockObject(DEFAULT_GUI_FONT));

			LOGFONT fontInfo;
			memset(&fontInfo, 0, sizeof(fontInfo));
			baseFont.GetLogFont(&fontInfo);

			// bold font
			fontInfo.lfWeight = 900;
			m_pFont = new CFont();
			m_pFont->CreateFontIndirect(&fontInfo);
		}

		SetEnabled(TRUE);

		GetIEditor()->GetLiveCreate()->RegisterListener(this);
	}

	virtual ~CLiveCreateToolbarButton_Main()
	{
		GetIEditor()->GetLiveCreate()->UnregisterListener(this);

		SAFE_DELETE(m_pErrorBitmap);
		SAFE_DELETE(m_pStartingBitmap);
	}

	virtual CBitmap* SelectIcon()
	{
		LiveCreate::CEditorManager* pManager = GetIEditor()->GetLiveCreate();
		if (GetEnabled() && pManager)
		{
			if (pManager->IsStarting())
			{
				return m_pStartingBitmap;
			}
			else if (pManager->IsEnabled())
			{
				return m_pActiveBitmap;
			}
			else if (m_bHasError)
			{
				return m_pErrorBitmap;
			}
			else
			{
				return m_pNormalBitmap;
			}
		}
		else
		{
			return m_pDisabledBitmap;
		}
	}

	virtual void Draw(CDC* pDC)
	{
		CLiveCreateToolbarButton::Draw(pDC);

		LiveCreate::CEditorManager* pManager = GetIEditor()->GetLiveCreate();
		if (pManager->IsStarting() && m_lastProgress > 0.0f)
		{
			const CRect rect = GetRect();
			const CPoint middle = rect.CenterPoint();
			CRect textRect(middle, middle);

			// manual position adjustment
			textRect.top -= 6;
			textRect.left -= 1;

			char s[50];
			sprintf_s(s, "%d%%", (int)(m_lastProgress * 100.0f));
			HGDIOBJ hOld = pDC->SelectObject(*m_pFont);

			pDC->SetTextColor(RGB(0,0,0));
			pDC->DrawTextA(s, -1, textRect, DT_CENTER | DT_VCENTER | DT_NOCLIP);

			textRect.left -= 1;
			textRect.top -= 1;
			pDC->SetTextColor(RGB(255,255,255));
			pDC->DrawTextA(s, -1, textRect, DT_CENTER | DT_VCENTER | DT_NOCLIP);

			pDC->SelectObject(hOld);
		}
	}

	virtual void OnExecute()
	{
		// toggle the state
		LiveCreate::CEditorManager* pManager = GetIEditor()->GetLiveCreate();
		if (pManager->IsStarting() || pManager->IsEnabled())
		{
			pManager->StopLiveCreate();
		}
		else
		{
			CString errorReason;
			const bool bQuick = false;
			if (!pManager->StartLiveCreate(errorReason, bQuick))
			{
				pManager->SetEnabled(false);
				MessageBox( GetCommandBar()->GetSafeHwnd(), errorReason, "LiveCreate Error", MB_ICONERROR | MB_OK );
			}
		}

		RequestRedraw();
	}

private:
	virtual void OnLiveCreateStarting(const float progress)
	{
		m_bHasError = false;

		if (progress != m_lastProgress)
		{
			m_lastProgress = progress;
			RequestRedraw();
		}
	}

	virtual void OnLiveCreateStarted()
	{
		RequestRedraw();
	}

	virtual void OnLiveCreateStopped()
	{
		m_bHasError = false;
		RequestRedraw();
	}

	virtual void OnLiveCreateError()
	{
		m_bHasError = true;
		RequestRedraw();
	}
};

class CLiveCreateToolbarButton_Camera : public CLiveCreateToolbarButton
	, public LiveCreate::CEditorManager::IListener
{

public:
	CLiveCreateToolbarButton_Camera()
		: CLiveCreateToolbarButton("Editor/Icons/lc_camera.png")
	{
		SetEnabled(TRUE);
		SetDescription("Toggle Camera Sync");

		GetIEditor()->GetLiveCreate()->RegisterListener(this);
	}

	virtual ~CLiveCreateToolbarButton_Camera()
	{
		GetIEditor()->GetLiveCreate()->UnregisterListener(this);
	}

	virtual CBitmap* SelectIcon()
	{
		LiveCreate::CEditorManager* pManager = GetIEditor()->GetLiveCreate();
		if (GetEnabled() && pManager)
		{
			if (pManager->IsEnabled() && pManager->IsCameraSyncEnabled())
			{
				return m_pActiveBitmap;
			}
			else
			{
				return m_pNormalBitmap;
			}
		}
		else
		{
			return m_pDisabledBitmap;
		}
	}

	virtual void OnExecute()
	{
		LiveCreate::CEditorManager* pManager = GetIEditor()->GetLiveCreate();
		if (pManager->IsEnabled())
		{
			const bool bIsActive = pManager->IsCameraSyncEnabled();
			GetIEditor()->GetLiveCreate()->SetCameraSync(!bIsActive);
			GetIEditor()->GetLiveCreate()->SaveSettings();

			RequestRedraw();
		}
	}

	virtual void OnLiveCreateStarted()
	{
		RequestRedraw();
	}

	virtual void OnLiveCreateStopped()
	{
		RequestRedraw();
	}
};

//-----------------------------------------------------------------------------

void InitLiveCreateToolbar(CXTPToolBar* pToolbar)
{
	pToolbar->GetControls()->RemoveAll();
	pToolbar->GetControls()->Add(new CLiveCreateToolbarButton_Main());
	pToolbar->GetControls()->Add(new CLiveCreateToolbarButton_Camera());
}

#endif
