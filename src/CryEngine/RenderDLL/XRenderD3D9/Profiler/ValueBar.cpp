/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#include "StdAfx.h"
#include "ValueBar.h"

CValueBar::CValueBar( IRenderer& renderer, float size, float height ) : m_renderer(renderer)
	, m_size(size)
	, m_height(height)
	, m_value(0.0f)
	, m_max(1.0f)
	, m_target(0.0f)
{
	SetColor(0.0f, 0.80f, 0.0f);
	SetBackground(0.0f, 0.0f, 0.0f);
}

void CValueBar::SetValue( float value )
{
	m_value = value;
}

void CValueBar::SetMax( float value )
{
	m_max = value;
}

void CValueBar::SetTarget( float value )
{
	m_target = value;
}

void CValueBar::SetColor( float r, float g, float b )
{
	m_color[0] = r;
	m_color[1] = g;
	m_color[2] = b;
}

void CValueBar::SetBackground( float r, float g, float b )
{
	m_background[0] = r;
	m_background[1] = g;
	m_background[2] = b;
}

void CValueBar::Draw( float x, float y ) const
{
	m_renderer.Set2DMode(true,1,1);
	m_renderer.SetState( GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA );

	float width = (m_value / m_max) * m_size;

	float xx = ((float) x) / 1200 * 800;
	float yy = ((float) y) / 700 * 600;
	float ww = (width) / 1200 * 800;
	float hh = m_height;

	if (m_background[0] > 0.0f && m_background[1] > 0.0f && m_background[2] > 0.0f)
	{
		m_renderer.Draw2dImage(xx, yy, (m_size) / 1200 * 800, hh, 0, 0, 0, 0, 0, 0, m_background[0], m_background[1], m_background[2], 1);
	}

	m_renderer.Draw2dImage(xx, yy, ww, hh, 0, 0, 0, 0, 0, 0, m_color[0], m_color[1], m_color[2], 1);

	if (m_target > 0.0f)
	{
		static float offset = 100.0f;
		static float w = 1;

		float markerDistance = (m_target / m_max) * m_size;

		xx += (markerDistance) / 1200 * 800;
		yy -= 3.0f / 700 * 600;
		ww = w;
		hh = 16.0f / 700 * 600;

		m_renderer.Draw2dImage(xx, yy, ww, hh, 0, 0, 0, 0, 0, 0, 0.8f, 0, 0, 1);
	}

	m_renderer.Set2DMode(false,1,1);
}