#pragma once

#include <Cry_Color.h>
#include <Cry_Math.h>

#include "EditorCommonAPI.h"
#include "Serialization.h"

namespace Serialization
{
	class IArchive;
};

struct SViewportState
{
	QuatT cameraTarget;
	QuatT cameraParentFrame;
	QuatT gridOrigin;
	Vec3 gridCellOffset;
	QuatT lastCameraTarget;
	QuatT lastCameraParentFrame;
	
	SViewportState()
		: cameraParentFrame(IDENTITY)
		, gridOrigin(IDENTITY)
		, gridCellOffset(0)
		, lastCameraTarget(IDENTITY)
		, lastCameraParentFrame(IDENTITY)
	{
		cameraTarget = QuatT(Matrix34	(-0.92322f, 0.37598f, 0.07935f, -0.88123f,
			-0.38426f, -0.90333f, -0.19065f, 2.19696f,
			0.0f, -0.2065f, 0.97845f, 1.58522f));

		lastCameraTarget = cameraTarget;
	}
};

struct SViewportDebugSettings
{
	bool wireframe;
	bool origin;
	bool fps;
	ColorB originColor;

	SViewportDebugSettings()
	: wireframe(false)
	, origin(false)
	, originColor(10, 10, 10, 255)
	, fps(true)
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(wireframe, "wireframe", "Wireframe");
		ar(origin, "origin", "Origin");
		ar(originColor, "originColor", origin ? "Origin Color" : 0);	
		ar(fps, "fps");	
	}	
};

struct SViewportCameraSettings
{
	bool showViewportOrientation;

	float fov;
	float nearClip;
	float smoothPos;
	float smoothRot;

	float moveSpeed;
	float rotationSpeed;
	float zoomSpeed;
	float fastMoveMultiplier;
	float slowMoveMultiplier;

	SViewportCameraSettings()
		: showViewportOrientation(true)
		, fov(60)
		, nearClip(0.01f)
		, smoothPos(0.07f)
		, smoothRot(0.05f)
		, moveSpeed(0.7f)
		, rotationSpeed(2.0f)
		, zoomSpeed(0.1f)
		, fastMoveMultiplier(3.0f)
		, slowMoveMultiplier(0.1f)
	{	
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(showViewportOrientation, "showViewportOrientation", "Show Viewport Orientation");
		ar(Serialization::Range(fov, 20.0f, 120.0f), "fov", "FOV");
		ar(Serialization::Range(nearClip, 0.01f, 0.5f), "nearClip", "Near Clip");
		ar(Serialization::Range(moveSpeed, 0.1f, 3.0f), "moveSpeed", "Move Speed");
		ar.Doc("Relative to the scene size");
		ar(Serialization::Range(rotationSpeed, 0.1f, 4.0f), "rotationSpeed", "Rotation Speed");
		ar.Doc("Degrees per 1000 px");
		if (ar.OpenBlock("movementSmoothing", "+Movement Smoothing"))
		{
			ar(smoothPos, "smoothPos", "Position");
			ar(smoothRot, "smoothRot", "Rotation");
			ar.CloseBlock();
		}
	}
};


struct SViewportGridSettings
{
	bool showGrid;
	bool circular;
	ColorB mainColor;
	ColorB middleColor;
	int alphaFalloff;
	float spacing;
	uint16 count;
	uint16 interCount;

	SViewportGridSettings()
		: showGrid(true)
		, circular(true)
	, mainColor(255,255,255,50)
	, middleColor(255,255,255,10)
		, alphaFalloff(100)
		, spacing(1.0f)
		, count(10)
		, interCount(10)
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(showGrid, "showGrid", "Show Grid");
		ar(circular, "circular", 0);
		ar(mainColor, "mainColor", "Main Color");
		ar(middleColor, "middleColor", "Middle Color");
		ar(Serialization::Range(alphaFalloff, 0, 100), "alphaFalloff", 0);
		ar(spacing, "spacing", "Spacing");
		ar(count, "count", "Main Lines");
		ar(interCount, "interCount", "Middle Lines");
	}
};

struct SViewportLightingSettings
{
	float brightness;
	ColorB ambientColor;
	
	SViewportLightingSettings()
	: ambientColor(76,76,76,255)
	, brightness(4.0f)
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(Serialization::Range(brightness, 0.0f, 200.0f), "brightness", "Brightness");
		ar(ambientColor, "ambientColor", "Ambient Color");
	}
};

struct SViewportBackgroundSettings
{
	bool useGradient;
	ColorB topColor;
	ColorB bottomColor;
	
	SViewportBackgroundSettings()
	: useGradient(true)
	, topColor(128, 128, 128, 255)
	, bottomColor(32, 32, 32, 255)
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(useGradient, "useGradient", "Use Gradient");
		if (useGradient)
		{
			ar(topColor, "topColor", "Top Color");
			ar(bottomColor, "bottomColor", "Bottom Color");
		}
		else
		{
			ar(topColor, "topColor", "Color");
		}
	}
};

struct SViewportSettings
{
	SViewportDebugSettings debug;
	SViewportCameraSettings camera;
	SViewportGridSettings grid;
	SViewportLightingSettings lighting;
	SViewportBackgroundSettings background;

	void Serialize(Serialization::IArchive& ar)
	{
		ar(camera, "camera", "Camera");
		ar(grid, "grid", "Grid");
		ar(lighting, "lighting", "Ligthing");
		ar(background, "background", "Background");
	}
};
