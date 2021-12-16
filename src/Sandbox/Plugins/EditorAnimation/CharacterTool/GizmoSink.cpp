#include "GizmoSink.h"
#include "CharacterDocument.h"
#include <ICryAnimation.h>

namespace CharacterTool
{

static const Manip::SElement* FindElementByHandle(int lastIndex, const void* handle, int layer, const Manip::CScene* scene)
{
	if (!scene)
		return 0;

	int numElements = scene->Elements().size();
	int last = min(numElements, lastIndex + 1);
	const Manip::SElements& elements = scene->Elements();

	for (int i = last; i < numElements; ++i)
		if (elements[i].layer == layer && elements[i].originalHandle == handle)
		{
			return &elements[i];
		}

	for (int i = 0; i < last; ++i)
		if (elements[i].layer == layer && elements[i].originalHandle == handle)
		{
			return &elements[i];
		}
	return 0;
}

static Manip::SElement* FindElementByHandle(int lastIndex, const void* handle, int layer, Manip::CScene* scene)
{
	return const_cast<Manip::SElement*>(FindElementByHandle(lastIndex, handle, layer, const_cast<const Manip::CScene*>(scene)));
}

enum {
	SPACE_TYPE_SHIFT = 24
};

int CharacterSpaceProvider::FindSpaceIndexByName(int spaceType, const char* name, int parentsUp) const
{
	if (!m_document)
		return -1;
	if (!m_document->CompressedCharacter())
		return -1;
	if (spaceType == Serialization::SPACE_JOINT || 
		spaceType == Serialization::SPACE_JOINT_WITH_PARENT_ROTATION ||
		spaceType == Serialization::SPACE_JOINT_SET_RELATIVE_TO_BIND_POSE)
	{
		IDefaultSkeleton& defaultSkeleton = m_document->CompressedCharacter()->GetIDefaultSkeleton();
		int result = defaultSkeleton.GetJointIDByName(name);
		return result | (spaceType << SPACE_TYPE_SHIFT);
	}
	else if (spaceType == Serialization::SPACE_ENTITY)
	{
		return spaceType << SPACE_TYPE_SHIFT;
	}
	return -1;
}

QuatT CharacterSpaceProvider::GetTransform(int typeAndSpace) const
{
	if (typeAndSpace == -1)
		return IDENTITY;
	if (!m_document)
		return IDENTITY;
	QuatT characterLocation(m_document->PhysicalLocation());
	if (!m_document->CompressedCharacter())
		return characterLocation;
	ISkeletonPose& skeletonPose = *m_document->CompressedCharacter()->GetISkeletonPose();
	IDefaultSkeleton& defaultSkeleton = m_document->CompressedCharacter()->GetIDefaultSkeleton();
	
	unsigned char spaceType = (typeAndSpace >> SPACE_TYPE_SHIFT) & 0xff;
	unsigned int spaceIndex = typeAndSpace & ~(0xff << SPACE_TYPE_SHIFT);
	switch (spaceType)
	{
	case Serialization::SPACE_JOINT_WITH_PARENT_ROTATION:
	{
		int parent = defaultSkeleton.GetJointParentIDByID(spaceIndex);
		QuatT parentSpace = skeletonPose.GetAbsJointByID(parent);
		QuatT jointSpace = skeletonPose.GetRelJointByID(spaceIndex);
		return characterLocation * parentSpace * QuatT(IDENTITY, jointSpace.t);
	}
	case Serialization::SPACE_JOINT_SET_RELATIVE_TO_BIND_POSE:
	{
		QuatT defaultParentSpace = defaultSkeleton.GetDefaultAbsJointByID(spaceIndex);
		QuatT parentSpace = skeletonPose.GetAbsJointByID(spaceIndex);
		return characterLocation * parentSpace * defaultParentSpace.GetInverted();
	}
	case Serialization::SPACE_JOINT:
		return characterLocation * skeletonPose.GetAbsJointByID(spaceIndex);
	default:
		return characterLocation;
	};
}

Vec3 CharacterSpaceProvider::GetSpaceVisualOrigin(int typeAndSpace) const
{
	if (typeAndSpace == -1)
		return ZERO;
	if (!m_document)
		return ZERO;
	QuatT characterLocation(m_document->PhysicalLocation());
	if (!m_document->CompressedCharacter())
		return characterLocation.t;
	ISkeletonPose& skeletonPose = *m_document->CompressedCharacter()->GetISkeletonPose();
	IDefaultSkeleton& defaultSkeleton = m_document->CompressedCharacter()->GetIDefaultSkeleton();
	
	unsigned char spaceType = (typeAndSpace >> SPACE_TYPE_SHIFT) & 0xff;
	unsigned int spaceIndex = typeAndSpace & ~(0xff << SPACE_TYPE_SHIFT);
	switch (spaceType)
	{
	case Serialization::SPACE_JOINT_WITH_PARENT_ROTATION:
	{
		int parent = defaultSkeleton.GetJointParentIDByID(spaceIndex);
		QuatT parentSpace = skeletonPose.GetAbsJointByID(parent);
		QuatT jointSpace = skeletonPose.GetRelJointByID(spaceIndex);
		return (characterLocation * parentSpace * QuatT(IDENTITY, jointSpace.t)).t;
	}
	case Serialization::SPACE_JOINT_SET_RELATIVE_TO_BIND_POSE:
	{
		QuatT parentSpace = skeletonPose.GetAbsJointByID(spaceIndex);
		return (characterLocation * parentSpace).t;
	}
	case Serialization::SPACE_JOINT:
		return (characterLocation * skeletonPose.GetAbsJointByID(spaceIndex)).t;
	default:
		return characterLocation.t;
	};
}

// ---------------------------------------------------------------------------

void GizmoSink::BeginWrite(ExplorerEntry* activeEntry, GizmoLayer layer)
{
	m_lastIndex = -1;
	m_currentLayer = int(layer);

	if (!m_scene)
		return;
	m_scene->ClearLayer(int(layer));
	m_activeEntry = activeEntry;
}

void GizmoSink::BeginRead(GizmoLayer layer)
{
	m_lastIndex = -1;
	m_currentLayer = int(layer);
}

void GizmoSink::EndRead()
{
	if (!m_scene)
		return;

	Manip::SElements& elements = m_scene->Elements();
	size_t numElements = elements.size();
	for (size_t i = 0; i < numElements; ++i)
	{
		Manip::SElement& element = elements[i];
		if (element.layer == m_currentLayer && element.changed)
			element.changed = false;
	}
}

bool GizmoSink::Read(Serialization::LocalFrame* decorator, Serialization::GizmoFlags* gizmoFlags, const void* handle)
{
	const Manip::SElement* element = FindElementByHandle(m_lastIndex, handle, m_currentLayer, m_scene);
	++m_lastIndex;
	if (!element || !element->changed)
		return false;
	*decorator->value = element->placement.transform;
	return true;
}

int GizmoSink::CurrentGizmoIndex() const 
{
	return m_lastIndex + 1;
}

int GizmoSink::Write(const Serialization::LocalFrame& decorator, const Serialization::GizmoFlags& gizmoFlags, const void* handle)
{
	if (!m_scene)
		return -1;
	Manip::SElement e;
	e.placement.transform = *decorator.value;
	e.placement.size = Vec3(0.02f, 0.02f, 0.02f);
	e.originalHandle = handle;
	if (m_scene->SpaceProvider())
		e.parentSpaceIndex = m_scene->SpaceProvider()->FindSpaceIndexByName(decorator.space, decorator.parentName, 0);
	e.alwaysXRay = true;
	e.caps = Manip::CAP_SELECT | Manip::CAP_MOVE | Manip::CAP_ROTATE;
	e.shape = Manip::SHAPE_AXES;
	e.layer = m_currentLayer;
	m_scene->AddElement(e, (Manip::ElementId)handle);
	++m_lastIndex;
	return m_lastIndex;
}

bool GizmoSink::Read(Serialization::LocalPosition* decorator, Serialization::GizmoFlags* gizmoFlags, const void* handle)
{
	const Manip::SElement* element = FindElementByHandle(m_lastIndex, handle, m_currentLayer, m_scene);
	++m_lastIndex;
	if (!element || !element->changed)
		return false;
	*decorator->value = element->placement.transform.t;
	return true;
}

int GizmoSink::Write(const Serialization::LocalPosition& decorator, const Serialization::GizmoFlags& gizmoFlags, const void* handle)
{
	if (!m_scene)
		return -1;
	Manip::SElement e;
	e.placement.transform = QuatT(*decorator.value, IDENTITY);
	e.placement.size = Vec3(0.02f, 0.02f, 0.02f);
	e.originalHandle = handle;
	if (m_scene->SpaceProvider())
		e.parentSpaceIndex = m_scene->SpaceProvider()->FindSpaceIndexByName(decorator.space, decorator.parentName, 0);
	e.alwaysXRay = true;
	e.caps = Manip::CAP_SELECT | Manip::CAP_MOVE;
	e.shape = Manip::SHAPE_AXES;
	e.layer = m_currentLayer;
	m_scene->AddElement(e, (Manip::ElementId)handle);
	++m_lastIndex;
	return m_lastIndex;
}

int GizmoSink::Write(const Serialization::LocalOrientation& decorator, const Serialization::GizmoFlags& gizmoFlags, const void* handle)
{
	if (!m_scene)
		return -1;
	Manip::SElement e;
	e.placement.transform = QuatT(ZERO, *decorator.value);
	e.placement.size = Vec3(0.02f, 0.02f, 0.02f);
	e.originalHandle = handle;
	if (m_scene->SpaceProvider())
		e.parentSpaceIndex = m_scene->SpaceProvider()->FindSpaceIndexByName(decorator.space, decorator.parentName, 0);
	e.alwaysXRay = true;
	e.caps = Manip::CAP_SELECT | Manip::CAP_ROTATE;
	e.shape = Manip::SHAPE_AXES;
	e.layer = m_currentLayer;
	m_scene->AddElement(e, (Manip::ElementId)handle);
	++m_lastIndex;
	return m_lastIndex;
}

bool GizmoSink::Read(Serialization::LocalOrientation* decorator, Serialization::GizmoFlags* gizmoFlags, const void* handle)
{
	const Manip::SElement* element = FindElementByHandle(m_lastIndex, handle, m_currentLayer, m_scene);
	++m_lastIndex;
	if (!element || !element->changed)
		return false;
	*decorator->value = element->placement.transform.q;
	return true;
}

void GizmoSink::Reset(const void* handle)
{
	int lastIndex = 0;
	Manip::SElement* element = FindElementByHandle(lastIndex, handle, m_currentLayer, m_scene);
	if (!element)
		return;

	element->placement.transform = IDENTITY;
	element->changed = true;
	m_scene->SignalElementsChanged(1 << element->layer);
}

void GizmoSink::SkipRead()
{
	++m_lastIndex;
}

}
