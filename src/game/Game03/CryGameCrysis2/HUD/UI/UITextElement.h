/************************************************************************/
/* UI element tree for Component HUD, Jan Müller, 2009									*/
/************************************************************************/

#ifndef ___UI_TEXTELEMENT_H___
#define ___UI_TEXTELEMENT_H___

#define UI_ENABLE_UI_TEXTELEMENT 0

#if UI_ENABLE_UI_TEXTELEMENT

#include "AutoEnum.h"

#include "HUD/UI/UIElement.h"

typedef CUIElement TTextElementParent; // IUIElement

class CTextUIElement : public TTextElementParent
{
public:
	explicit CTextUIElement();//const IItemParamsNode * xmlElement, EUISubElementType eType, IUIElement *parent = NULL);

	virtual ~CTextUIElement();

	virtual void InitializeMemberData( void );
	virtual void Initialize( const IItemParamsNode* param, IUIElement* );

	virtual void Update( float frameTime );
	virtual void Draw( void ) const;

	virtual const float	GetPosX() const;
	virtual const float	GetPosY() const;
	void SetData(string newData);

	ILINE IUIElement* GetParent()
	{
		return m_parent;
	}

private:

protected:

	IUIElement* m_parent;
	//EUISubElementType m_type;
	string m_data;
	//wstring localizedData;
	int m_aspectDisplacement;

	bool m_haveValue;

	union
	{
		struct
		{
			float m_drainAmount;
		} m_textureDraining;
	} m_paramsUnion;
};

#endif // UI_ENABLE_UI_TEXTELEMENT

#endif // ___UI_TEXTELEMENT_H___
