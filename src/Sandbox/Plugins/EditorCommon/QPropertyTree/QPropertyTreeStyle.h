#pragma once

#include "Serialization.h"
#include "Serialization/Decorators/Range.h"

struct QPropertyTreeStyle
{
	bool compact;
	bool fullRowMode;
	bool showHorizontalLines;
	bool doNotIndentSecondLevel;
	bool groupShadows;
	bool groupRectangle;
	bool alignLabelsToRight;
	float valueColumnWidth;
	float rowSpacing;
	unsigned char levelShadowOpacity;
	float levelIndent;
	float firstLevelIndent;
	float groupShade;
	float sliderSaturation;

	QPropertyTreeStyle()
	: compact(false)
	, fullRowMode(false)
	, valueColumnWidth(.59f)
	, rowSpacing(1.1f)
	, showHorizontalLines(false)
	, firstLevelIndent(0.75f)
	, levelIndent(0.75f)
	, levelShadowOpacity(36)
	, doNotIndentSecondLevel(false)
	, groupShadows(false)
	, sliderSaturation(0.0f)
	, groupShade(0.15f)
	, groupRectangle(false)
	, alignLabelsToRight(false)
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar.Doc("Here you can define appearance of QPropertyTree control.");

		ar(valueColumnWidth, "valueColumnWidth", "Value Column Width");		ar.Doc("Defines a ratio of the value / name columns. Normalized.");
				ar(Serialization::Range(rowSpacing, 0.5f, 3.0f), "rowSpacing", "Row Spacing"); ar.Doc("Height of one row (line) in text-height units.");
		ar(alignLabelsToRight, "alignLabelsToRight", "Right Alignment");
		ar(Serialization::Range(levelIndent, 0.0f, 3.0f), "levelIndent", "Level Indent"); ar.Doc("Indentation of a every next level in text-height units.");

		ar(Serialization::Range(firstLevelIndent, 0.0f, 3.0f), "firstLevelIndent", "First Level Indent"); ar.Doc("Indentation of a very first level in text-height units.");
		ar(Serialization::Range(sliderSaturation, 0.0f, 1.0f), "sliderSaturation", "Slider Saturation");	
		ar(levelShadowOpacity, "levelShadowOpacity", "Level Shadow Opacity"); ar.Doc("Amount of background darkening that gets added to each next nested level.");

		ar(compact, "compact", "Compact"); ar.Doc("Compact mode removes expansion pluses from the level and reduces inner padding. Useful for narrowing the widget.");

		ar(showHorizontalLines, "showHorizontalLines", "Horizontal Lines"); ar.Doc("Show thin line that connects row name with its value.");

		ar(doNotIndentSecondLevel, "doNotIndentSecondLevel", "Do not indent second level");	
		ar(groupShadows, "groupShadows", "Group Shadows");	
		ar(groupRectangle, "groupRectangle", "Group Rectangle");	

		ar(Serialization::Range(groupShade, -1.0f, 1.0f), "groupShade", "Group Shade");	ar.Doc("Shade of the group.");
	}
};

