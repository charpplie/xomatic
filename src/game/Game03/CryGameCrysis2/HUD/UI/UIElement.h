/************************************************************************/
/* UI element tree for Component HUD, Jan Müller, 2009									*/
/************************************************************************/

#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "HUD/UI/IUIElement.h"

#include "HUD/ScreenLayoutManager.h"

#define INCLUDE_UI_ELEMENT_ANIMATION_CODE 0

//---------------------------------------------------------------------------------
// Non-Flash HUDObject interface.
class CUIElement : public IUIElement
{
public:

#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
	struct UI2AnimationInfo{

		enum EUIElementAnimation
		{
			eSEA_None=0,
			eSEA_ScorePopup,
		};

		float start;
		float dur;
		EUIElementAnimation type;
		bool  m_removeWhenDone;

		UI2AnimationInfo()
		{
			start = 0;
			dur = 0;
			type = eSEA_None;
			m_removeWhenDone = false;
		}
		UI2AnimationInfo( float in_start, float in_dur, EUIElementAnimation in_type, bool removeWhenDone )
		{
			start = in_start;
			dur = in_dur;
			type = in_type;
			m_removeWhenDone = removeWhenDone;
		}
	};
#else
	struct UI2AnimationInfo{
		uint8 foo;
	};
#endif

public:
	explicit CUIElement();
	virtual ~CUIElement();
	virtual void InitializeMemberData( void );
	virtual void Initialize( const IItemParamsNode* params, IUIElement* parent );

	virtual void Update( float frameTime );
	virtual void Draw( void ) const;
	//virtual void	Hide( bool hide );

	virtual const char*	GetName() const;
	virtual const float	GetPosX() const;
	virtual const float	GetPosY() const;
	virtual const float	GetWidth() const;
	virtual const float	GetHeight() const;
	virtual void        SetPos(float newX, float newY);
	virtual void        SetSize(float newWidth, float newHeight);

	IUIElement* GetParent( void ){ return m_parent; };

	IUIElement*    GetSubElement( const uint32 index );
	IUIElement*    GetSubElement( const char *m_name );
	const uint32   GetSubElementsCount( void ) { return m_subElements.size(); }

#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
	virtual void   SetAnimation( const UI2AnimationInfo& ainfo );
	virtual bool	 IsDone( void ) const;
#endif

	ILINE static TBitfield MakeVanishSettingsFromString(const char * str)
	{
		return AutoEnum_GetBitfieldFromString(str, s_drawnElementBitfieldNames, UIElementItemList_numBits);
	}

	static const char * s_drawnElementBitfieldNames[];

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		GetInternalMemoryUsage(pSizer); 
	}
	void GetInternalMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(m_subElements);		
	}
protected:

	void ClearSubElements();
	void UpdatePopup();

	//EUISubElementType GetElementType(const char *tagName) const;

	IUIElement* m_parent;

	typedef std::vector<IUIElement*> SubElementList;
	SubElementList m_subElements;

	CryFixedStringT<32> m_name;
	ColorF textColor;
	ColorF tintColor;

	TUISubElementFlags m_flags;

	float m_posX;
	float m_posY;

	float m_width;
	float	m_height;

#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
	UI2AnimationInfo m_animinfo;
	bool m_removeWhenDone;
#endif

	bool m_useTintColor;
	bool m_useTextColor;

	ScreenLayoutStates m_layoutType;

	//static ILocalizationManager* pLocalizationMan;
	//static ILocalizationManager* initLocalalizationManager( void );
};


static ILINE float HUDCurveFraction(float f)
{
	return (f > 0.5f) ? (1.f - 2.f * ((1.f - f) * (1.f - f))) : (f * f * 2.f);
}

#endif
