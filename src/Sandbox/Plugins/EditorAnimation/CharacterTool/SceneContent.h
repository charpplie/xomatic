#pragma once

#include <vector>
#include <QObject>
#include "FeatureTest.h"
#include "PlaybackLayers.h"

namespace CharacterTool {
using std::vector;

struct IFeatureTest;

struct AudioSetup
{
	string foleyLibrary;
	string footstepLibrary;
	string footstepEffectName;
	
	AudioSetup();
	void Serialize(IArchive& ar);
};

struct BlendShapeParameter
{
	string name;
	float weight;
	bool operator<(const BlendShapeParameter& rhs) const{ return name < rhs.name; }

	BlendShapeParameter()
	: weight(1.0f)
	{
	}
};
typedef vector<BlendShapeParameter> BlendShapeParameters;

struct BlendShapeSkin
{
	string name;
	BlendShapeParameters params;

	void Serialize(IArchive& ar);
	bool operator<(const BlendShapeSkin& rhs) const{ return name < rhs.name; }
};
typedef vector<BlendShapeSkin> BlendShapeSkins;

struct BlendShapeOptions
{
	bool overrideWeights;
	BlendShapeSkins skins;

	BlendShapeOptions()
	: overrideWeights(false)
	{
	}

	void Serialize(IArchive& ar);
};




struct SceneContent : public QObject
{
	Q_OBJECT
public:

	string characterPath;
	PlaybackLayers layers;
	int aimLayer;
	int lookLayer;
	BlendShapeOptions blendShapeOptions;
	AudioSetup audioSetup;

	bool runFeatureTest;
	_smart_ptr<IFeatureTest> featureTest;

	std::vector<char> lastLayersContent;
	std::vector<char> lastContent;
	
	SceneContent();
	void Serialize(Serialization::IArchive& ar);
	bool CheckIfPlaybackLayersChanged(bool continuous);
	void PlaybackLayersChanged(bool continuous);
	void MotionParametersChanged(bool continuousChange);
	AimParameters& GetAimParameters();
	AimParameters& GetLookParameters();
	MotionParameters& GetMotionParameters();
signals:
	void SignalCharacterChanged();
	void SignalPlaybackLayersChanged(bool continuous);
	void SignalBlendShapeOptionsChanged();
	void SignalNewLayerActivated();

	void SignalChanged(bool continuous);
};

}
