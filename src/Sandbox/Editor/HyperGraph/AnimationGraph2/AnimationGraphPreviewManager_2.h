#ifndef __ANIMATIONGRAPHPREVIEWMANAGER2_H__
#define __ANIMATIONGRAPHPREVIEWMANAGER2_H__

#include "AnimationGraph_2.h"
#include "..\CharacterEditor\ModelViewportCE.h"

class CAnimationGraphPreviewManager2
{
public:
	CAnimationGraphPreviewManager2();
	void SetViewport(CModelViewportCE* pViewport);
	ICharacterInstance* GetCharacter() const;
	void SetState(CAGState2Ptr pState);
	void EnablePreview(bool enablePreview);
	void SetParameter(const char* name, const char* value);
	
private:
	void StartPreview();
	void StopPreview();
	void ReplaceParameters( string& name ) const;

	CModelViewportCE* m_pViewport;
	CAGState2Ptr m_pState;
	bool m_enablePreview;

	TParameterizationId2 m_mapLastParamValues;
};

#endif //__ANIMATIONGRAPHPREVIEWMANAGER_H__
