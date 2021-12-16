/************************************************************************/
/* UI element tree for Component HUD, Jan Müller, 2009									*/
/************************************************************************/
#if 0
#ifndef ___UI_SUBELEMENT_H___
#define ___UI_SUBELEMENT_H___

#include "AutoEnum.h"
#include "HUD/UI/UIElement.h"

typedef CUIElement TSubElementParent;

class CUIElement : public TSubElementParent
{
public:
	explicit CSubUIElement();
	virtual ~CSubUIElement();
	virtual void InitializeMemberData( void );
	virtual void Initialize( const IItemParamsNode* params, IUIElement* parent );

	virtual void Update( float frameTime );
	virtual void Draw( void ) const;

	virtual const float	GetPosX() const;
	virtual const float	GetPosY() const;

	void SetData(string newData);

protected:

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


#endif // ___UI_SUBELEMENT_H___
#endif // 0