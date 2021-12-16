#pragma once

#include <platform.h>
#include <Cry_Geo.h>
#include <Cry_Color.h>
#include "EditorCommonAPI.h"
#include <QObject>
#include "QViewportConsumer.h"


namespace Serialization { class IArchive; }

class CAxisHelper;

// Manip is a set of reusable utilities for creating interactive 3d scene that
// can be manipulated with gizmos.
namespace Manip
{

using std::auto_ptr;
using std::set;
using std::vector;

enum EElementCaps
{
	CAP_SELECT = 1 << 0,
	CAP_HIDE = 1 << 1,
	CAP_MOVE = 1 << 2,
	CAP_ROTATE = 1 << 3,
	CAP_SCALE = 1 << 4,
	CAP_DELETE = 1 << 5,
};

enum EElementShape
{
	SHAPE_AXES,
	SHAPE_BOX
};

enum EElementAction
{
	ACTION_NONE,
	ACTION_DELETE,
	ACTION_HIDE,
	ACTION_UNHIDE
};

enum EElementColorGroup
{
	ELEMENT_COLOR_PROXY,
	ELEMENT_COLOR_CLOTH
};

struct SElementPlacement
{
	QuatT transform;
	QuatT startTransform;
	Vec3 center;
	Vec3 size;

	SElementPlacement()
	: transform(IDENTITY)
	, startTransform(IDENTITY)
	, size(1.0f, 1.0f, 1.0f)
	, center(0.0f, 0.0f, 0.0f)
	{}

	void Serialize(Serialization::IArchive& ar);
};

typedef uint64 ElementId;

struct SElement
{
	ElementId id;
	union {
		int originalId;
		const void* originalHandle;
	};
	int layer;
	SElementPlacement placement;
	int caps;
	EElementAction action;
	EElementShape shape;
	EElementColorGroup colorGroup;
	int parentSpaceIndex;
	int parentOrientationSpaceIndex;
	QuatT parentSpaceConcatenation;
	int mousePickPriority;
	bool hidden;
	bool changed;
	bool alwaysXRay;

	SElement()
	: id()
	, layer(0)
	, originalHandle(0)
	, action(ACTION_NONE)
	, shape(SHAPE_AXES)
	, hidden(false)
	, changed(false)
	, colorGroup(ELEMENT_COLOR_PROXY)
	, mousePickPriority(0)
	, alwaysXRay(false)
	, parentSpaceIndex(-1)
	, parentOrientationSpaceIndex(-1)
	, parentSpaceConcatenation(IDENTITY)
	{
	}
};
typedef std::vector<SElement> SElements;

struct SElementData {};

struct IMouseDragHandler
{
	virtual bool Begin(const SMouseEvent& ev, Vec3 hitPoint) = 0;
	virtual void Update(const SMouseEvent& ev) = 0;
	virtual void Render(const SRenderContext& rc) {}
	virtual void End(const SMouseEvent& ev) = 0;
};

struct SSelectionSet
{
	SSelectionSet()	{}
	SSelectionSet(ElementId id) { items.push_back(id); }
	void Clear();
	void Add(ElementId elementId) 
	{
		items.erase(std::remove(items.begin(), items.end(), elementId), items.end());
		items.push_back(elementId);
		std::sort(items.begin(), items.end());
	}
	void Remove(int elementId);
	bool IsEmpty() const{ return items.empty(); }
	bool Contains(int id) const{ return std::find(items.begin(), items.end(), id) != items.end(); }
	size_t Size() const{ return items.size(); }

	bool operator==(const SSelectionSet& rhs) const
	{
		return items == rhs.items;
	}

	bool operator!=(const SSelectionSet& rhs) const{ return !operator==(rhs); }

	std::vector<ElementId> items;
};

struct ICommand {};

struct IElementTracer
{
	virtual bool HitRay(Vec3* intersectionPoint, const Ray& ray, const SElement& element) const = 0;
};

struct IElementDrawer
{
	virtual bool Draw(const SElement& element);
};

enum ETransformationSpace
{
	SPACE_WORLD,
	SPACE_LOCAL
};

enum ETransformationMode
{
	MODE_TRANSLATE,
	MODE_ROTATE,
	MODE_SCALE
};

struct SLookSettings
{
	ColorB proxyColor;
	ColorB proxySelectionColor;
	ColorB proxyHighlightColor;
	ColorB clothProxyColor;

	ColorB jointColor;
	ColorB jointHighlightColor;
	ColorB jointSelectionColor;

	SLookSettings()
	: proxyColor(126, 159, 243, 128)
	, proxySelectionColor(255, 255, 255, 128)
	, proxyHighlightColor(233, 255, 122, 128)
	, clothProxyColor(243, 159, 126, 128)
	, jointColor(0, 249, 48, 255)
	, jointHighlightColor(255, 248, 0, 128)
	, jointSelectionColor(255, 255, 255, 128)
	{
	}
};

struct ISpaceProvider
{
	virtual int FindSpaceIndexByName(int spaceType, const char* name, int parentsUp) const = 0;
	virtual QuatT GetTransform(int index) const = 0;
	virtual Vec3 GetSpaceVisualOrigin(int index) const = 0;
};

class EDITOR_COMMON_API CScene : public QObject, public QViewportConsumer
{
	Q_OBJECT
public:
	CScene();
	~CScene();

	void OnViewportKey(const SKeyEvent& ev) override;
	void OnViewportMouse(const SMouseEvent& ev) override;
	void OnViewportRender(const SRenderContext& rc) override;
	
	void SetTransformationMode(ETransformationMode mode);
	ETransformationMode TransformationMode() const;
	void SetTransformationSpace(ETransformationSpace space);
	ETransformationSpace TransformationSpace() const { return m_transformationSpace; }
	void SetVisibleLayerMask(unsigned int layerMask);
	unsigned int VisibleLayerMask() const { return m_visibleLayerMask; }
	bool IsLayerVisible(int layer) const;

	void Clear();
	void ClearLayer(int layer);
	void AddElement(const SElement& element);
	void AddElement(const SElement& element, ElementId id);
	void ApplyToAll(EElementAction);
	void ApplyToSelection(EElementAction);

	void SetSpaceProvider(ISpaceProvider* spaceProvider);
	ISpaceProvider* SpaceProvider() const{ return m_spaceProvider; }
	QuatT GetParentSpace(const SElement& e) const;
	Vec3 GetSpaceVisualOrigin(const SElement& e) const;

	void SetCustomTracer(IElementTracer* tracer);

	const SElements& Elements() const{ return m_elements; }
	SElements& Elements() { return m_elements; }
	void GetSelectedElements(SElements* elements) const;
	const SSelectionSet& Selection() const{ return m_selection; }
	void SetSelection(const SSelectionSet& selection);
	void AddToSelection(const SSelectionSet& selection);
	void AddToSelection(ElementId elementId);

	bool SelectionCanBeMoved() const;
	bool SelectionCanBeRotated() const;
	QuatT GetSelectionTransform(ETransformationSpace space) const;
	bool SetSelectionTransform(ETransformationSpace space, const QuatT& newTransform);

	void Serialize(Serialization::IArchive& ar);
signals:
	void SignalUndo();
	void SignalRedo();
	void SignalPushUndo(const char* description, ICommand* cause);
	void SignalElementsChanged(unsigned int layerBits);
	void SignalElementContinousChange(unsigned int layerBits);
	void SignalPropertiesChanged();
	void SignalRenderElements(const SElements& elements, const SRenderContext& rc);
	void SignalSelectionChanged();
	void SignalManipulationModeChanged();

private:
	void UpdateElements(const SElements& elements);
	int GetSelectionCaps() const;
	Vec3 GetSelectionSize() const;
	bool SetSelectionSize(const Vec3& size);
	void OnMouseMove(const SMouseEvent& ev);

	struct SBlockSelectHandler;
	struct SMoveHandler;
	struct SRotationHandler;
	struct SScalingHandler;
	struct STransformBox;

	IElementTracer* m_customTracer;
	IElementDrawer* m_customDrawer;

	SSelectionSet m_selection;
	auto_ptr<IMouseDragHandler> m_mouseDragHandler;
	ISpaceProvider* m_spaceProvider;
	auto_ptr<CAxisHelper> m_axisHelper;
	SElements m_elements;
	std::vector<ElementId> m_lastIdByLayer;
	ETransformationMode m_transformationMode;
	ETransformationSpace m_transformationSpace;
	int m_highlightItem;
	unsigned int m_visibleLayerMask;
	bool m_showGizmo;
	SLookSettings m_lookSettings;
	int m_highlightedItem;
	QuatT m_temporaryLocalDelta;
};

}
