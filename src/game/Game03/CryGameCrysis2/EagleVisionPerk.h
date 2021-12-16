#ifndef __EAGLEVISIONPERK_H__
#define __EAGLEVISIONPERK_H__

#include "IPerk.h"

class EagleVisionPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(EagleVisionPerk);

	EagleVisionPerk();
	virtual ~EagleVisionPerk() {}

	virtual void Update(const float dt);

protected:
	bool ShouldHighlight();

	bool m_seenTarget;
};

#endif
