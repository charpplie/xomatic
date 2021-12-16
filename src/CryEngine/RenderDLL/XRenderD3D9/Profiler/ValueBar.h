/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#pragma once 

#ifndef VALUE_BAR_H
#define VALUE_BAR_H

#include <IRenderer.h>

struct IRenderer;

class CValueBar
{
public:
	CValueBar(IRenderer& renderer, float size, float height);

	void SetValue(float value);
	void SetMax(float value);
	void SetTarget(float value);

	void SetColor(float r, float g, float b);
	void SetBackground(float r, float g, float b);

	void Draw(float x, float y) const;

private:
	IRenderer& m_renderer;

	float m_size;
	float m_height;

	float m_value;
	float m_max;
	float m_target;

	float m_color[3];
	float m_background[3];
};

#endif