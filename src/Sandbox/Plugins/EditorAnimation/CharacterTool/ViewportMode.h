#pragma once

#include "../../EditorCommon/QViewportConsumer.h"
#include <vector>

namespace Serialization 
{ 
	class IArchive; 
	struct SStruct;
}

struct ICharacterInstance;
class QToolBar;
class QPropertyTree;

namespace CharacterTool
{
	
using std::vector;
class CharacterToolForm;
class CharacterDocument;
class TransformPanel;
struct System;
struct ExplorerEntry;

struct SModeContext
{
	System* system;
	CharacterToolForm* window;
	CharacterDocument* document;
	ICharacterInstance* character;
	TransformPanel* transformPanel;
	QToolBar* toolbar;
	std::vector<QPropertyTree*> layerPropertyTrees;
};

struct IViewportMode : public QViewportConsumer
{
	virtual ~IViewportMode() {}
	virtual void Serialize(Serialization::IArchive& ar) {}
	virtual void EnterMode(const SModeContext& context) {}
	virtual void LeaveMode() {}

	virtual void GetPropertySelection(vector<const void*>* selectedItems) const {}
	virtual void SetPropertySelection(const vector<const void*>& items) {}
};

}
