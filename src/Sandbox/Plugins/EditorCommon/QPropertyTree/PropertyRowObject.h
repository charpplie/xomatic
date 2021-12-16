#pragma once
#include "PropertyRow.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/Pointers.h"
#include "Serialization/Object.h"

namespace Serialization {
	class IArchive;
	struct SStruct;
	class MemoryWriter;
};

class PropertyRowObject : public PropertyRow {
public:
	PropertyRowObject();
	~PropertyRowObject();
	void setValueAndContext(const Serialization::Object& obj, Serialization::IArchive& ar) { object_ = obj; }
	void setModel(PropertyTreeModel* model) { model_ = model; }
	bool isObject() const override{ return true; }
	bool assignTo(Serialization::Object* obj);
	void Serialize(Serialization::IArchive& ar);
	const Serialization::Object& object() const{ return object_; }
protected:

	Serialization::Object object_;
	PropertyTreeModel* model_;
};

