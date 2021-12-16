#pragma once

#include <vector>
#include <QWidget>

#include "TimelineContent.h"

class QPainter;
class QPaintEvent;
class QLineEdit;

struct STimelineLayout;

struct STimelineViewState
{
	float viewOrigin;
	float visibleDistance;
	float clampedViewOrigin;
	int widthPixels;
	QPoint scrollPixels;
	int maxScrollX;
	int treeWidth;
	int treeLastOpenedWidth;

	STimelineViewState()
	: viewOrigin(0.0f)
	, clampedViewOrigin(0.0f)
	, visibleDistance(1.0f)
	, scrollPixels(0, 0)
	, maxScrollX(0)
	, treeWidth(0)
	, widthPixels(1)
	{
	}	

	QPoint LocalToLayout(const QPoint& p) const;
	QPoint LayoutToLocal(const QPoint& p) const;

	int ScrollOffset(float origin) const;
	int TimeToLayout(float time) const;
	float LocalToTime(int x) const;
	int TimeToLocal(float time) const;
	float LayoutToTime(int x) const;
};

struct STrackLayout;

class EDITOR_COMMON_API CTimeline : public QWidget
{
	Q_OBJECT
public:
	CTimeline(QWidget* parent);
	~CTimeline();

	void SetContent(STimelineContent* pContent);
	STimelineContent* Content() const { return m_pContent; }

	void ContentUpdated() { UpdateLayout(); update(); }

	bool IsDragged() const { return m_mouseHandler.get() != 0; }

	// make it possible to have actual time in normalized units, but different display units
	void SetTimeUnitScale(float timeUnitScale);
	void SetTime(float time);
	void SetCycled(bool cycled);
	void SetSizeToContent(bool sizeToContent);
	void SetFramesToSnap(int numFrames, bool snapToFrames) { m_framesToSnap = numFrames; m_snapToFrames = snapToFrames; }
	void SetKeyWidth(uint width) { m_keyWidth = width; UpdateLayout(); update(); }
	void SetKeyRadius(float radius) { m_keyRadius = radius; UpdateLayout(); update(); }
	void SetTreeVisible(bool visible);
	void SetDrawSelectionIndicators(bool visible) { m_selIndicators = visible; update(); }

	float Time() const { return m_time; }

	bool HandleKeyEvent(int key);

	void paintEvent(QPaintEvent* ev) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void focusOutEvent(QFocusEvent* ev) override;
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

	void AddKeyToTrack(STimelineTrack &subTrack, float time);

	void keyPressEvent(QKeyEvent* ev) override;
	void keyReleaseEvent(QKeyEvent* ev) override;
	void resizeEvent(QResizeEvent* ev) override;
	void wheelEvent(QWheelEvent* ev) override;
	QSize sizeHint() const override;

signals:
	void SignalScrub(bool scrubThrough);
	void SignalContentChanged(bool continuous);
	void SignalSelectionChanged(bool continuous);
	void SignalPlay();
	void SignalNumberHotkey(int number);
	void SignalTreeContextMenu(const QPoint& point);

	void SignalUndo();
	void SignalRedo();
protected slots:
	void OnMenuSelectionToCursor();
	void OnMenuDuplicate();
	void OnMenuCopy();
	void OnMenuPaste();
	void OnMenuDelete();
	void OnMenuPlay();
	void OnMenuNextKey();
	void OnMenuPreviousKey();
	void OnMenuNextFrame();
	void OnMenuPreviousFrame();
	void OnFilterChanged();

private:
	struct SMouseHandler;
	struct SSelectionHandler;
	struct SMoveHandler;
	struct SPanHandler;
	struct SScrubHandler;
	struct SSplitterHandler;
	struct STreeMouseHandler;

	void ContentChanged(bool continuous);
	void UpdateLayout();
	void UpdateCursor(QMouseEvent* ev);
	void DrawMarkers(QPainter& painter, int offsetY);
	float ClampAndSnapTime(float time, bool snapToFrames) const;
	void ClampAndSetTime(float time, bool scrubThrough);
	STrackLayout* GetTrackLayoutFromPos(const QPoint& pos) const;

	// Exposed parameters
	float m_timeUnitScale;
	int m_framesToSnap;
	bool m_cycled;
	bool m_sizeToContent;
	bool m_snapToFrames;
	bool m_treeVisible;
	bool m_selIndicators;
	uint m_keyWidth;
	float m_keyRadius;

	// State
	STimelineViewState m_viewState;
	STimelineContent* m_pContent;
	float m_time;
	std::unique_ptr<STimelineLayout> m_layout;
	std::unique_ptr<SMouseHandler> m_mouseHandler;

	// Filtering
	QLineEdit* m_pFilterLineEdit;	

	// Track selection
	STrackLayout* m_pLastSelectedTrack;

	friend class CTimelineTracks;
};

class CTimelineTracks : public QWidget
{
	Q_OBJECT
public:
	CTimelineTracks(QWidget* widget) : QWidget(widget) {}
	void ConnectToTimeline(CTimeline* timeline) { m_timeline = timeline; }

private:
	CTimeline* m_timeline;
};
