// (c) 2001-2012 Crytek GmbH
#pragma once

class QViewport;

struct SMouseEvent
{
	enum EType
	{
		NONE,
		PRESS,
		RELEASE,
		MOVE
	};

	enum EButton
	{
		BUTTON_NONE,
		BUTTON_LEFT,
		BUTTON_RIGHT,
		BUTTON_MIDDLE
	};

	EType type;
	int x;
	int y;
	EButton button;
	bool shift;
	bool control;
	QViewport* viewport;

	SMouseEvent()
	: type(NONE)
	, x(INT_MIN)
	, y(INT_MIN)
	, button(BUTTON_NONE)
	, viewport(0)
	, shift(false)
	, control(false)
	{
	}
};

struct SSelectionID
{
};

struct SInteractionEvent
{
	enum EType
	{
		NONE,
		ENTER,
		LEAVE,
		DRAG
	};

	SSelectionID selection;
	Vec3 start;
	Vec3 end;
};

struct SKeyEvent
{
	enum EType
	{
		NONE,
		PRESS,
		RELEASE
	};

	EType type;
	int key;

	SKeyEvent()
	: type(NONE)
	{
	}
};
