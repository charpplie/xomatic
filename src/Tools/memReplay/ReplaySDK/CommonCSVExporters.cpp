#include "stdafx.h"
#include "CommonCSVExporters.h"

#include "CGFPhysicsExporter.h"
#include "RangedContextExporter.h"
#include "CSVCollator.h"

static const char* s_overviewExportColumns[] = {
	"Item"
};

static const char* s_textureExportColumns[] = {
	"Texture"
};

static const char* s_renderMeshExportColumns[] = {
	"Name",
	"Type"
};

static const char* s_animationExportColumns[] = {
	"Name",
};

static const char* s_navExportColumns[] = {
	"Name"
};

static const char* s_entityExportColumns[] = {
	"Name",
	"Archetype"
};

static const char* s_soundExportColumns[] = {
	"Name"
};

namespace
{
	ContextTreeCSVExporter rootExporter;
	CGFPhysicsCSVExporter cgfPhysExporter;
	RangedContextExporter overviewExporter(1, 2, s_overviewExportColumns, false);
	RangedContextExporter textureExporter(1, 2, s_textureExportColumns, false);
	RangedContextExporter renderMeshExporter(1, 3, s_renderMeshExportColumns, false);
	RangedContextExporter animationExporter(1, 2, s_animationExportColumns, false);
	RangedContextExporter navExporter(2, 3, s_navExportColumns, false);
	RangedContextExporter entityExporter(1, 3, s_entityExportColumns, true);
	RangedContextExporter soundExporter(1, 2, s_soundExportColumns, false);
}

namespace CommonCSVExporters
{
	const ICSVExporter& RootExporter = rootExporter;
	const ICSVExporter& CgfPhysExporter = cgfPhysExporter;
	const ICSVExporter& OverviewExporter = overviewExporter;
	const ICSVExporter& TextureExporter = textureExporter;
	const ICSVExporter& RenderMeshExporter = renderMeshExporter;
	const ICSVExporter& AnimationExporter = animationExporter;
	const ICSVExporter& NavExporter = navExporter;
	const ICSVExporter& EntityExporter = entityExporter;
	const ICSVExporter& SoundExporter = soundExporter;
}
