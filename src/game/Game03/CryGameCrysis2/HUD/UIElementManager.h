#ifndef __UIELEMENTMANAGER_H__
#define __UIELEMENTMANAGER_H__

#include "HUD/UI/UIElement.h"

class CUIElementManager
{
public :
	CUIElementManager();
	~CUIElementManager();

	void Clear( void );

	XmlNodeRef LoadElementXML( const string& filename );
	void LoadTextures( const string& filename ); // load from disk
	void LoadElements( const string& filename, const CUIElement::UI2AnimationInfo* ainfo = NULL, std::vector<string> *elementsLoaded = NULL ); // load from disk or cache
	void LoadElements( const XmlNodeRef& xmlUI , const CUIElement::UI2AnimationInfo* ainfo = NULL, std::vector<string> *elementsLoaded = NULL );
	void Load( void );

	IUIElement*        FindElement(const char *pName) const;
	IUIElement*        FindSubElement(const char *pName);
	ITexture*          GetTexture(const char * textureName) const;

	void GetMemoryUsage(ICrySizer *pSizer) const
	{	
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_textures);
		pSizer->AddObject(m_elements);
	}
private :

	void ClearTextures( void );
	void ClearElements();

private :
	std::map<CryFixedStringT<32>, ITexture*> m_textures; 
	std::list<CUIElement*> m_elements;
};


#endif //__UIELEMENTMANAGER_H__