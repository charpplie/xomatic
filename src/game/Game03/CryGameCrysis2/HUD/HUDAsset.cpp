#include "StdAfx.h"
#include "HUDAsset.h"

#include "HUD.h"
#include "Flash/Flash.h"

#include "IItemSystem.h"
#include "HUD/HUDObjectManager.h"
#include "Graphics/2DRenderUtils.h"
#include "HUD/HUDCVars.h"
#include "Utility/CryWatch.h"

//////////////////////////////////////////////////////////////////////////


CFlashAsset::CFlashAsset() 
: m_pPlayer(NULL)
, m_externalPlayer(false)
{
	m_pPlayer = CFlash::Get()->CreateSafeFlashPlayerInstance();
}



CFlashAsset::CFlashAsset(IFlashPlayer* pPlayer) 
: m_pPlayer(pPlayer)
, m_externalPlayer(true)
{
}



CFlashAsset::~CFlashAsset()
{
	if(!m_externalPlayer)
		SAFE_RELEASE(m_pPlayer);
}



void CFlashAsset::SetPath(const string& path)
{
	bool loaded = m_pPlayer->Load(path.c_str());
	
	if(loaded)
	{
		m_pPlayer->SetBackgroundAlpha(0.0f);
		m_info.SetAssetSize(m_pPlayer->GetWidth(), m_pPlayer->GetHeight());
	}
	else
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Failed to load flash file '%s'.", path.c_str()).c_str() );
	}
}

void CFlashAsset::Init(const IItemParamsNode * asset, ScreenLayoutManager* pScreenLayout)
{
	m_info.Init( asset, pScreenLayout );
	m_pPlayer->SetViewport( m_info.x, m_info.y, m_info.w, m_info.h );
}

void CFlashAsset::SetPositionAndSize(const float x, const float y, const float sizeX, const float sizeY)
{
	m_info.SetPositionAndSize(x, y, sizeX, sizeY);
	m_pPlayer->SetViewport( m_info.x, m_info.y, m_info.w, m_info.h );
}

void CFlashAsset::Invoke(const char* path, SFlashVarValue* pResult)
{
	m_pPlayer->Invoke0(path, pResult);
}



void CFlashAsset::Invoke(const char* path, int arg, SFlashVarValue* pResult)
{
	Invoke(path, SFlashVarValue(arg), pResult);
}



void CFlashAsset::Invoke(const char* path, float arg, SFlashVarValue* pResult)
{
	Invoke(path, SFlashVarValue(arg), pResult);
}



void CFlashAsset::Invoke(const char* path, bool arg, SFlashVarValue* pResult)
{
	Invoke(path, SFlashVarValue(arg), pResult);
}



void CFlashAsset::Invoke(const char* path, const char* arg, SFlashVarValue* pResult)
{
	Invoke(path, SFlashVarValue(arg), pResult);
}



void CFlashAsset::Invoke(const char* path, const wchar_t* arg, SFlashVarValue* pResult)
{
	Invoke(path, SFlashVarValue(arg), pResult);
}



void CFlashAsset::Invoke(const char* path, const SFlashVarValue& arg, SFlashVarValue* pResult)
{
	m_pPlayer->Invoke1(path, arg, pResult);
}



void CFlashAsset::Invoke(const char* path, SFlashVarValue* args, unsigned int numArgs, SFlashVarValue* pResult)
{
	m_pPlayer->Invoke(path, args, numArgs, pResult);
}



void CFlashAsset::SetVariable(const char* path, int arg)
{
	SetVariable(path, SFlashVarValue(arg));
}



void CFlashAsset::SetVariable(const char* path, float arg)
{
	SetVariable(path, SFlashVarValue(arg));
}



void CFlashAsset::SetVariable(const char* path, bool arg)
{
	SetVariable(path, SFlashVarValue(arg));
}



void CFlashAsset::SetVariable(const char* path, const char* arg)
{
	SetVariable(path, SFlashVarValue(arg));
}


void CFlashAsset::SetVariable(const char* path, const wchar_t* arg)
{
	SetVariable(path, SFlashVarValue(arg));
}


void CFlashAsset::SetVariable(const char* path, const SFlashVarValue& arg)
{
	m_pPlayer->SetVariable(path, arg);
}



void CFlashAsset::GetVariable(const char* path, int& arg) const
{
	SFlashVarValue flashArg(arg);
	GetVariable(path, flashArg);
	arg = (int)flashArg.GetDouble();
}



void CFlashAsset::GetVariable(const char* path, float& arg) const
{
	SFlashVarValue flashArg(arg);
	GetVariable(path, flashArg);
	arg = (float)flashArg.GetDouble();
}



void CFlashAsset::GetVariable(const char* path, string& arg) const
{
	SFlashVarValue flashArg(arg.c_str());
	GetVariable(path, flashArg);
	arg = flashArg.GetConstStrPtr();
}



void CFlashAsset::GetVariable(const char* path, SFlashVarValue& arg) const
{
	m_pPlayer->GetVariable(path, arg);
}



void	CFlashAsset::GetVariable(const char* path, IFlashVariableObject*& arg) const
{
	m_pPlayer->GetVariable(path, arg);

	if( !arg )
	{
		arg = CHUD::s_pHUDFlashvariableObjectNULL;
	}
}



void CFlashAsset::SetVariableArrayDouble(const char* const path, std::vector<double> *doubleArray)
{
	m_pPlayer->SetVariableArray(FVAT_Double, path, 0, &(*doubleArray)[0], doubleArray->size());
}



int CFlashAsset::GetWidth() const
{
	return m_pPlayer->GetWidth();
}



int CFlashAsset::GetHeight() const
{
	return m_pPlayer->GetHeight();
}



void CFlashAsset::Update(float frameTime)
{
	m_pPlayer->Advance(frameTime);
}

void CFlashAsset::Draw()
{
#if ENABLE_HUD_EXTRA_DEBUG
	m_pPlayer->SetViewport( m_info.x, m_info.y, m_info.w, m_info.h );
#endif
	m_pPlayer->Render();

#if	ENABLE_HUD_EXTRA_DEBUG
	// Debug Text (Virtual Space Funcs)
	if( g_pGame->GetHUD()->GetCVars()->hud_debugDrawAssets )
	{
		m_info.DebugDraw(m_pPlayer->GetFilePath());
	}
#endif
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SHUDSizeAndPositionInfo::SHUDSizeAndPositionInfo()
	:	x(0)
	, y(0)
	, w(100)
	, h(100)
	, m_canvasSize(1280.0f, 720.0f)
	, m_assetSize(100.0f, 100.0f)
	, m_size(-1.0f, -1.0f)
	, m_relPos(0.0f, 0.0f)
	, m_pivotAlignment(UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER)
	, m_screenLayout(NULL)
	, m_layoutFlags(eSLO_Default)
{
	CHUD::AddHUDEventListener(this, "OnResolutionChange");
}

SHUDSizeAndPositionInfo::~SHUDSizeAndPositionInfo()
{
	CHUD::RemoveHUDEventListener(this);
}

void SHUDSizeAndPositionInfo::Init(const IItemParamsNode * asset, ScreenLayoutManager* pScreenLayout)
{
	assert( pScreenLayout );
	m_screenLayout = pScreenLayout;

	const IItemParamsNode * size = asset->GetChild("Size");
	const IItemParamsNode * alignment = asset->GetChild("Alignment");

	if(const IItemParamsNode * node = asset->GetChild("Position"))
	{
		node->GetAttribute("x", m_relPos.x);
		node->GetAttribute("y", m_relPos.y);
	}

	if(size)
	{
		size->GetAttribute("x", m_size.x);
		size->GetAttribute("y", m_size.y);
	}

	if(alignment)
	{
		string strAlignment = alignment->GetAttribute("x");
		if(!strAlignment.compareNoCase("left"))
			m_pivotAlignment.x = UIDRAWHORIZONTAL_LEFT;
		else if(!strAlignment.compareNoCase("right"))
			m_pivotAlignment.x = UIDRAWHORIZONTAL_RIGHT;
		else
			m_pivotAlignment.x = UIDRAWHORIZONTAL_CENTER;

		strAlignment = alignment->GetAttribute("y");
		if(!strAlignment.compareNoCase("top"))
			m_pivotAlignment.y = UIDRAWVERTICAL_TOP;
		else if(!strAlignment.compareNoCase("bottom"))
			m_pivotAlignment.y = UIDRAWVERTICAL_BOTTOM;
		else
			m_pivotAlignment.y = UIDRAWVERTICAL_CENTER;
	}

	Recalculate();
}

void SHUDSizeAndPositionInfo::SetPositionAndSize(float x, float y, float sizeX, float sizeY)
{
	m_relPos.x = x;
	m_relPos.y = y;

	m_size.x = sizeX;
	m_size.y = sizeY;

	Recalculate();
}

void SHUDSizeAndPositionInfo::SetCanvasSize(int width, int height)
{
	m_canvasSize.x = (float)width;
	m_canvasSize.y = (float)height;
}



void SHUDSizeAndPositionInfo::SetAssetSize(int width, int height)
{
	m_assetSize.x = (float)width;
	m_assetSize.y = (float)height;
}

#if ENABLE_HUD_EXTRA_DEBUG
void SHUDSizeAndPositionInfo::DebugDraw( const char* label )
{
	if( g_pGame->GetHUD()->GetCVars()->hud_debugDrawAssets == 2 )
	{
		if( NULL == strstr( label, g_pGame->GetHUD()->GetCVars()->hud_debugDrawAssets_search->GetString() ) )
		{			
			return; // the substring wasn't found, don't debug draw this asset.
		}
	}
	// Adjust to safe areas
	float fx = (float)x;
	float fy = (float)y;
	float fw = (float)w;
	float fh = (float)h;

	ColorF col_debug(1.0f,0.8f,0.6f,0.5f);
	IFFont* pFont = gEnv->pCryFont->GetFont("default");
	C2DRenderUtils* p2dRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();

	m_screenLayout->SetState(eSLO_DoNotAdaptToSafeArea|eSLO_ScaleMethod_None);
	p2dRenderUtils->SetFont( pFont );
	m_screenLayout->ConvertFromRenderToVirtualScreenSpace( &fx, &fy );
	m_screenLayout->ConvertFromRenderToVirtualScreenSpace( &fw, &fh );
	p2dRenderUtils->DrawRect( fx,fy, fw,fh, col_debug );
	p2dRenderUtils->DrawText( fx,fy, 20.0f,20.0f, string().Format("p(%d,%d) s(%d,%d) %s", (int)fx, (int)fy, (int)fw, (int)fh, label ).c_str(), col_debug );

}
#endif

void SHUDSizeAndPositionInfo::Recalculate()
{
	CalculateSize();
	CalculatePosition();

	if( !m_screenLayout )
	{
		return;
	}

	m_screenLayout->SetState(m_layoutFlags);

	// Adjust to safe areas
	float fx = (float)x;
	float fy = (float)y;
	float fw = (float)w;
	float fh = (float)h;

	// Adjust in virtual space.
	m_screenLayout->AdjustToSafeArea( &fx, &fy, &fw, &fh, static_cast<EUIDRAWHORIZONTAL>(m_pivotAlignment.x), static_cast<EUIDRAWVERTICAL>(m_pivotAlignment.y), UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP );

	// Convert to screenspace for rendering.
	m_screenLayout->ConvertFromVirtualToRenderScreenSpace( &fx, &fy );
	m_screenLayout->ConvertFromVirtualToRenderScreenSpace( &fw, &fh );

	x = int_round(fx);
	y = int_round(fy);
	w = int_round(fw);
	h = int_round(fh);
}



void SHUDSizeAndPositionInfo::CalculateSize()
{
	bool validX = (m_size.x != -1.0f);
	bool validY = (m_size.y != -1.0f);

	m_layoutFlags = eSLO_Default;

	if( validX && validY )
	{
		m_layoutFlags = eSLO_ScaleMethod_None;
		// Use proportions in XML
		w = int_round(m_size.x*m_screenLayout->GetVirtualWidth());
		h = int_round(m_size.y*m_screenLayout->GetVirtualHeight());
	}
	else if( validX && !validY )
	{
		m_layoutFlags = eSLO_ScaleMethod_WithX;
		// scale Y with proportional changes in X
		w = int_round(m_size.x*m_screenLayout->GetVirtualWidth());
		h = int_round((float)w/m_assetSize.x*m_assetSize.y);
	}
	else if( !validX && validY )
	{
		m_layoutFlags = eSLO_ScaleMethod_WithY;
		// scale X with proportional changes in Y
		h = int_round(m_size.y*m_screenLayout->GetVirtualHeight());
		w = int_round((float)h/m_assetSize.y*m_assetSize.x);
	}
	else //if( !validX && !validY )
	{
		m_layoutFlags = eSLO_ScaleMethod_None;
		// use asset size, always.
		w = int_round(m_assetSize.x);
		h = int_round(m_assetSize.y);
	}
}



void SHUDSizeAndPositionInfo::CalculatePosition()
{
	float tempPosX = 0.0f;
	float tempPosY = 0.0f;

	tempPosX = m_relPos.x * m_screenLayout->GetVirtualWidth();
	tempPosY = m_relPos.y * m_screenLayout->GetVirtualHeight();

	x = int_round(tempPosX);
	y = int_round(tempPosY);
}

void SHUDSizeAndPositionInfo::OnHUDEvent(const SHUDEvent& event)
{
	//if(event.eventType == eHUDEvent_OnResolutionChange)
	{
		SetCanvasSize( event.GetData(0).GetInt(), event.GetData(1).GetInt() );
		Recalculate();
	}
}

//////////////////////////////////////////////////////////////////////////
