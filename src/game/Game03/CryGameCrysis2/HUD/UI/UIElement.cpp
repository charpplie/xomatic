/************************************************************************/
/* UI element tree for Component HUD, Jan Müller, 2009									*/
/************************************************************************/

#include "StdAfx.h"

#include "HUD/HUD.h"
#include "HUD/UI/UIElement.h"
#include "HUD/HUD_Impl.h"
#include "HUD/UI/UIElementFactory.h"

#include "StringUtils.h"
#include "PerkDbgDisplay.h"
#include "Graphics/2DRenderUtils.h"

const static float TEXTURE_FADEOUT_TIME = 4.0f;

AUTOENUM_BUILDNAMEARRAY(CUIElement::s_drawnElementBitfieldNames, UIElementItemList);

#if defined(_RELEASE)
	#define OUTPUT_EXTRA_WARNINGS	0
#elif defined(USER_frankh)
	#define OUTPUT_EXTRA_WARNINGS	1
#else
	#define OUTPUT_EXTRA_WARNINGS	0
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

CUIElement::CUIElement( )
: m_parent(NULL)
, textColor(1.0f,1.0f,1.0f,1.0f)
{
	InitializeMemberData( );
}

CUIElement::~CUIElement()
{
	ClearSubElements();
}

void CUIElement::InitializeMemberData( void )
{
	m_posX   = 0.0f;
	m_posY   = 0.0f;
	m_width  = 0.0f;
	m_height = 0.0f;
	m_flags  = 0;
#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
	m_removeWhenDone = false;
#endif
	m_layoutType = eSLO_ScaleMethod_WithY;
}

void CUIElement::Initialize( const IItemParamsNode* params, IUIElement *parent )
{
	const char * xmlName = params->GetAttribute("name");
	
	CRY_ASSERT(xmlName);
	m_name = xmlName;

	const char * scalemethod = params->GetAttribute("scalemethod");
	if(scalemethod)
	{
		if( !stricmp( scalemethod, "noscale" ) )
		{
			m_layoutType = eSLO_DoNotAdaptToSafeArea|eSLO_ScaleMethod_None;
		}
		else
		{
			CRY_ASSERT_MESSAGE( 0, string().Format("[HUD_Impl] Unkown scale method for element '%s'", GetName() ) );
		}
	}
	else
	{
// 		if(m_parent)
// 		{
// 			m_layoutType = m_parent->m_layoutType;
// 		}
// 		else
		{
			m_layoutType = eSLO_Default;
		}
	}

	HudLog( 1, "CUIElement::Initialize - reading element '%s'", m_name.c_str() );

	m_posX = 0.0f;
	m_posY = 0.0f;
	m_width  = (float)g_pGame->GetHUD()->GetLayoutManager()->GetVirtualWidth();
	m_height = (float)g_pGame->GetHUD()->GetLayoutManager()->GetVirtualHeight();

	
	const IItemParamsNode * xmlPos = params->GetChild("Position");
	if(xmlPos)
	{
		xmlPos->GetAttribute("x", m_posX);
		xmlPos->GetAttribute("y", m_posY);
	}

	const IItemParamsNode * xmlSize = params->GetChild("Size");
	if(xmlSize)
	{
		xmlSize->GetAttribute("x", m_width);
		xmlSize->GetAttribute("y", m_height);
	}


/*
	if(parent)
	{
		posX += parent->GetPosX();
		posY += parent->GetPosY();
	}
*/
	const IItemParamsNode *  xmlColor = params->GetChild("TextColor");
	if(xmlColor)
	{
		m_useTextColor = true;

		xmlColor->GetAttribute("r", textColor.r);
		xmlColor->GetAttribute("g", textColor.g);
		xmlColor->GetAttribute("b", textColor.b);
		if (!xmlColor->GetAttribute("a", textColor.a))
		{
			textColor.a = 1.f;
		}
	}
	else 
	{
		m_useTextColor = false;
	}

	xmlColor = NULL;
	xmlColor = params->GetChild("TintColor");
	if(xmlColor)
	{
		m_useTintColor = true;

		xmlColor->GetAttribute("r", tintColor.r);
		xmlColor->GetAttribute("g", tintColor.g);
		xmlColor->GetAttribute("b", tintColor.b);
		if (!xmlColor->GetAttribute("a", tintColor.a))
		{
			tintColor.a = 1.f;
		}
	}
	else 
	{
		m_useTintColor = false;
	}

	const int iChildren = params->GetChildCount();
	for(int i = 0; i < iChildren; ++i)
	{
		const IItemParamsNode * child = params->GetChild(i);
		const char* tagText = child->GetName();
		IUIElement* subElement = ElementFactory::GetClassInstance(tagText);

		if( !subElement )
			continue;

		subElement->Initialize(child, this);
		m_subElements.push_back(subElement);
		//CryLog("Creating element of type %d from '%s' name='%s'", eType, tagText, child->GetAttribute("name"));
	}
}

#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
void CUIElement::UpdatePopup()
{
	switch( m_animinfo.type )
	{		
	case UI2AnimationInfo::eSEA_ScorePopup :
	{

		static const float riseTime = 1.0f;   //1s
		static const float riseDist = -100.0f; //100px
		const float curTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		float dTime = curTime - m_animinfo.start;
		if( dTime > m_animinfo.dur )
		{
			dTime = 0.0f;
		}
		float t = dTime/riseTime;
		float v = 1.0f - (1.0f-t)*(1.0f-t);

		//const float fHeightMid = 600.0f*0.5f;// TODO: FIXEME-> (float)gEnv->pRenderer->GetHeight() * 0.5f;

		m_posY = v*riseDist;
		//width *= v;
		//height *= v;
		break;
	}

	case UI2AnimationInfo::eSEA_None :
		break;

	default :
		HudLog(3, "UI2 : Unknown animation type! %d", (int)m_animinfo.type );
	}
}
#endif

void CUIElement::Update( float frameTime )
{
#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
	UpdatePopup();
#endif

	int numElements = m_subElements.size();
	for(int e = 0; e < numElements; ++e)
		m_subElements[e]->Update(frameTime);
}
 
void CUIElement::Draw( void ) const
{
	if( m_flags & eSEF_NORENDER )
	{
		return;
	}

	// Get current layout state.
	C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
	ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();


	pLayoutManager->SetState(m_layoutType);

	SubElementList::const_iterator it = m_subElements.begin();
	SubElementList::const_iterator end = m_subElements.end();
	for(; it != end; ++it )
	{
		(*it)->Draw();
	}

	return;
}

const char*	CUIElement::GetName() const 
{ 
	return m_name.c_str(); 
}

const float	CUIElement::GetPosX() const 
{ 
	return m_posX; 
}

const float	CUIElement::GetPosY() const 
{ 
	return m_posY; 
}

const float	CUIElement::GetWidth() const 
{ 
	return m_width; 
}

const float	CUIElement::GetHeight() const 
{ 
	return m_height; 
}

void CUIElement::SetPos(float newX, float newY)
{
	m_posX = newX;
	m_posY = newY;
}

void CUIElement::SetSize(float newWidth, float newHeight)
{
	m_width = newWidth;
	m_height = newHeight;
}

/*void CUIElement::Hide( bool hide )
{
	if( hide )
	{
		m_flags |= eSEF_NORENDER;
	}
	else
	{
		m_flags &= ~eSEF_NORENDER;
	}
	return;
}*/

void CUIElement::ClearSubElements()
{

	SubElementList::iterator it = m_subElements.begin();
	SubElementList::iterator end = m_subElements.end();
	while(it!=end)
	{
		SAFE_DELETE( *it );
		++it;
	}

	m_subElements.clear();
	assert(!m_subElements.size());
}

//const CSubUIElement* CUIElement::GetSubElement( const char *name ) const
IUIElement* CUIElement::GetSubElement( const char *name )
{
	//SubElementList::const_iterator it = m_subElements.begin();
	//SubElementList::const_iterator end = m_subElements.end();

	SubElementList::iterator it = m_subElements.begin();
	SubElementList::iterator end = m_subElements.end();

	const IUIElement *pElement = NULL;

	while(it != end)
	{
		if(!stricmp((*it)->GetName(), name))
		{
			return *it;
		}

		//const IUIElement *pElement = (*it)->GetSubElement(name);
		IUIElement *pElement = (*it)->GetSubElement(name);
		if( pElement )
		{
			return pElement;
		}

		++it;
	}

	return NULL;
}

IUIElement* CUIElement::GetSubElement( const uint32 index )
{
	if (m_subElements.size() > index)
	{
		return m_subElements[index];
	}
	return NULL;
}

#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
void CUIElement::SetAnimation( const UI2AnimationInfo& ainfo )
{ 
	m_animinfo = ainfo; 
	m_removeWhenDone = ainfo.m_removeWhenDone;
}

bool CUIElement::IsDone( void ) const
{
	if( m_removeWhenDone )
	{
		const float curTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		if( (m_animinfo.start + m_animinfo.dur) < curTime )
		{
			return true;
		}
	}
	return false;
}
#endif

#if 0
void CUIElement::DoOnAllSubElementsRecursive(funcptr_1cuielement1void func, void *data)
{
	SubElementList::iterator  end = m_subElements.end();
	for (SubElementList::iterator it=m_subElements.begin(); it!=end; ++it)
	{
		if (func)
		{
			(*func)((*it), data);
			(*it)->DoOnAllSubElementsRecursive(func, data);
		}
	}
}
#endif
