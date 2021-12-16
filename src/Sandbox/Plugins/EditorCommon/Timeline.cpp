#include "Timeline.h"
#include "QtUtil.h"
#include "DrawingPrimitives/TimeSlider.h"
#include "DrawingPrimitives/Ruler.h"

#include <vector>
#include <algorithm>
#include <VectorSet.h>
#include <StringUtils.h>

#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QLineEdit>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

int STimelineViewState::ScrollOffset(float origin) const
{
	return int((origin / visibleDistance + 0.5f) * widthPixels);
}

int STimelineViewState::TimeToLayout(float time) const
{
	return int((time / visibleDistance) * widthPixels + 0.5f);
}

int STimelineViewState::TimeToLocal(float time) const
{
	return TimeToLayout(time) + treeWidth + scrollPixels.x();
}

float STimelineViewState::LayoutToTime(int x) const
{
	return (float(x) - 0.5f) / widthPixels * visibleDistance;
}

float STimelineViewState::LocalToTime(int x) const
{
	return LayoutToTime(x - treeWidth - scrollPixels.x());
}

QPoint STimelineViewState::LocalToLayout(const QPoint& p) const
{
	return p - scrollPixels - QPoint(treeWidth, 0);
}

QPoint STimelineViewState::LayoutToLocal(const QPoint& p) const
{
	return p + scrollPixels + QPoint(treeWidth, 0);
}

enum
{
	THUMB_WIDTH = 12,
	THUMB_HEIGHT = 24,

	RULER_HEIGHT = 16,
	RULER_SHADOW_HEIGHT = 6,
	RULER_MARK_HEIGHT = 8,

	DEFAULT_KEY_WIDTH = 8,
	VERTICAL_PADDING = 4,
	TRACK_DESCRIPTION_INDENT = 8,

	SELECTION_WIDTH = 4,
	SCROLL_SHADOW_WIDTH = 8,

	MAX_PUSH_OUT = VERTICAL_PADDING * 2,
	PUSH_OUT_DISTANCE = 3,

	SPLITTER_WIDTH = 10,

	DEFAULT_TREE_WIDTH = 200,
	TREE_LEFT_MARGIN = 6,
	TREE_INDENT_MULTIPLIER = 12,
	TREE_BRANCH_INDICATOR_SIZE = 8
};

namespace
{
	const float DEFAULT_KEY_RADIUS = 0.1f;
	const int TIMELINE_PADDING = 20;	
}

struct STimelineContentElementRef
{
	STimelineContentElementRef()
		: pTrack(nullptr)
		, index(0)
	{}

	STimelineContentElementRef(STimelineTrack* pTrack, size_t index)
		: pTrack(pTrack)
		, index(index)
	{}

	STimelineElement& GetElement() const
	{
		return pTrack->elements[index];
	}

	bool IsValid() const { return pTrack != 0; }

	bool operator<(const STimelineContentElementRef& rhs) const
	{
		if (pTrack == rhs.pTrack)
			return index < rhs.index;
		else
		{
			if (!pTrack && rhs.pTrack)
				return true;
			else if (pTrack && !rhs.pTrack)
				return false;
			else if (pTrack && rhs.pTrack)
				return pTrack->name < rhs.pTrack->name;
			return false;
		}
	}

	STimelineTrack* pTrack;
	size_t index;
};

struct SElementLayout
{
	STimelineElement::EType type;
	int caps;
	float pushOutDistance;
	QRect rect;
	ColorB color;

	STimelineContentElementRef elementRef;
	std::vector<STimelineContentElementRef> subElements;

	bool IsSelected() const
	{
		if ((elementRef.pTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) == 0)
		{
			return elementRef.GetElement().selected;
		}
		else
		{
			bool bSelected = false;
			const size_t numSubElements = subElements.size();

			for (size_t i = 0; i < numSubElements; ++i)
			{
				bSelected = bSelected || subElements[i].GetElement().selected;
			}

			return bSelected;
		}
	}

	void SetSelected(const bool bSelected) const
	{
		if ((elementRef.pTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) == 0)
		{
			elementRef.GetElement().selected = bSelected;
		}
		else
		{
			const size_t numSubElements = subElements.size();
			for (size_t i = 0; i < numSubElements; ++i)
			{
				subElements[i].GetElement().selected = bSelected;
			}
		}
	}

	SElementLayout() 
	: pushOutDistance(0.0f)
	, caps(0)
	, type(STimelineElement::KEY) 
	{}
};

struct STrackLayout;
typedef std::vector<STrackLayout> STrackLayouts;

struct STrackLayout
{
	QRect rect;
	int indent;

	STimelineTrack* pTimelineTrack;

	std::vector<SElementLayout> elements;
	STrackLayouts tracks;

	STrackLayout()
	: indent(0)
	, pTimelineTrack(0)
	{
	}
};

struct STimelineLayout
{
	int thumbPositionX;
	STrackLayouts tracks;
	float minStartTime;
	float maxEndTime;
	QSize size;

	STimelineLayout()
	: thumbPositionX(0)
	, minStartTime(0.0f)
	, maxEndTime(1.0f)
	, size(1,1)
	{
	}
};

namespace
{
	QColor InterpolateColor(const QColor& a, const QColor& b, float k)
	{
		float mk = 1.0f - k;
		return QColor(a.red() * mk  + b.red() * k,
		              a.green() * mk + b.green() * k,
		              a.blue() * mk + b.blue() * k,
		              a.alpha() * mk + b.alpha() * k);
	}

	void ClampViewOrigin(STimelineViewState* viewState, const STimelineLayout& layout)
	{
		float zoomOffset = viewState->visibleDistance * 0.5f;
		
		const float padding = viewState->LayoutToTime(TIMELINE_PADDING);
		float maxViewOrigin = layout.minStartTime - zoomOffset + padding;
		float minViewOrigin = std::min(viewState->visibleDistance - layout.maxEndTime - zoomOffset - padding, maxViewOrigin);

		viewState->viewOrigin = clamp_tpl(viewState->viewOrigin, minViewOrigin, maxViewOrigin);
	}

	SElementLayout& AddElementToTrackLayout(STimelineTrack& track, STrackLayout& trackLayout, const STimelineElement& element,
	                                        const STimelineViewState& viewState, uint keyWidth, int treeWidth, int& currentTop, size_t elementIndex)
	{
		trackLayout.elements.push_back(SElementLayout());
		SElementLayout& elementl = trackLayout.elements.back();
		elementl.color = element.color;
		elementl.type = element.type;
		elementl.caps = element.caps;
		elementl.elementRef.pTrack = &track;
		elementl.elementRef.index = elementIndex;

		if (element.type == STimelineElement::KEY)
		{
			int left = (viewState.TimeToLayout(element.start) - keyWidth / 2);
			int right = left + keyWidth;
			elementl.rect = QRect(left, currentTop + VERTICAL_PADDING, right - left, track.height - VERTICAL_PADDING * 2);
		}
		else
		{
			int left = viewState.TimeToLayout(element.start);
			int right = viewState.TimeToLayout(element.end);
			elementl.rect = QRect(left, currentTop + VERTICAL_PADDING, right - left, track.height - VERTICAL_PADDING * 2);
		}

		return elementl;
	}

	void AddCompoundElementsToTrackLayout(STimelineTrack& track, STimelineLayout* layout, const STimelineViewState& viewState, int trackId, uint keyWidth, int treeWidth, int& currentTop)
	{
		const size_t numSubTracks = track.tracks.size();
		float currentElementTime = -std::numeric_limits<float>::max();
		size_t* pCurrentSubTrackIndices = static_cast<size_t*>(alloca(sizeof(size_t) * numSubTracks));
		memset(pCurrentSubTrackIndices, 0, sizeof(size_t) * numSubTracks);

		while (true)
		{
			bool bElementFound = false;
			float minElementTime = std::numeric_limits<float>::max();

			// First search for minimum element time for current track positions
			for (size_t i = 0; i < numSubTracks; ++i)
			{
				const STimelineTrack& subTrack = *track.tracks[i];
				const STimelineElements& elements = subTrack.elements;
				const size_t numTrackElements = elements.size();
				const size_t index = pCurrentSubTrackIndices[i];

				if (index < numTrackElements)
				{
					const float elementTime = elements[index].start;
					minElementTime = std::min(elementTime, minElementTime);
					bElementFound = true;
				}
			}

			if (!bElementFound)
			{
				break;
			}

			STimelineElement compoundElement;
			compoundElement.start = minElementTime;
			compoundElement.end = minElementTime;

			// If elements were found create a compound element
			STrackLayout& trackLayout = layout->tracks[trackId];
			SElementLayout& compoundElementLayout = AddElementToTrackLayout(track, trackLayout, compoundElement, viewState, keyWidth, treeWidth, currentTop, 0);
			currentElementTime = minElementTime;

			// Advance track positions and add elements IDs to compound element if times match
			for (size_t i = 0; i < numSubTracks; ++i)
			{
				STimelineTrack* pSubTrack = track.tracks[i];
				const STimelineElements& elements = pSubTrack->elements;
				const size_t numTrackElements = elements.size();
				size_t& index = pCurrentSubTrackIndices[i];

				if (index < numTrackElements)
				{
					const float elementTime = elements[index].start;

					if (elementTime == minElementTime)
					{
						STimelineContentElementRef ref;
						ref.pTrack = pSubTrack;
						ref.index = index;
						compoundElementLayout.subElements.push_back(ref);
						++index;
					}
				}
			}
		}
	}

	bool FilterTracks(const STimelineTrack &track, std::unordered_set<const STimelineTrack*>& invisibleTracks, const char* filterString)
	{
		bool bAnyChildVisible = false;
		const bool bNameMatchesFilter = CryStringUtils::stristr(track.name, filterString) != nullptr;

		if (!bNameMatchesFilter)
		{
			const size_t numChildTracks = track.tracks.size();
			for (size_t i = 0; i < numChildTracks; ++i)
			{
				bAnyChildVisible = FilterTracks(*track.tracks[i], invisibleTracks, filterString) || bAnyChildVisible;
			}
		}

		if (!bNameMatchesFilter && !bAnyChildVisible)
		{
			invisibleTracks.insert(&track);			
		}

		return bNameMatchesFilter || bAnyChildVisible;
	}

	void CalculateTrackLayout(STimelineLayout* layout, int& currentTop, int currentIndent, STimelineTrack& parentTrack, const STimelineViewState& viewState,
		float thumbTime, uint keyWidth, int treeWidth, const std::unordered_set<const STimelineTrack*>& invisibleTracks)
	{
		const size_t numTracks = parentTrack.tracks.size();
		for (size_t i = 0; i < numTracks; ++i)
		{
			STimelineTrack& track = *parentTrack.tracks[i];

			if (stl::find(invisibleTracks, &track))
			{
				continue;
			}

			layout->tracks.push_back(STrackLayout());
			STrackLayout& trackLayout = layout->tracks.back();
			trackLayout.elements.reserve(track.elements.size());
			trackLayout.indent = currentIndent;
			trackLayout.pTimelineTrack = &track;

			layout->minStartTime = std::min(layout->minStartTime, track.startTime);
			layout->maxEndTime = std::max(layout->maxEndTime, track.endTime);

			const bool bIsCompositeTrack = (track.caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

			if (bIsCompositeTrack)
			{
				const int trackLayoutId = layout->tracks.size() - 1;
				AddCompoundElementsToTrackLayout(track, layout, viewState, trackLayoutId, keyWidth, treeWidth, currentTop);
			}
			else
			{
				for (size_t i = 0; i < track.elements.size(); ++i)
				{
					const STimelineElement& element = track.elements[i];
					AddElementToTrackLayout(track, trackLayout, element, viewState, keyWidth, treeWidth, currentTop, i);
				}
			}

			int left = viewState.TimeToLayout(track.startTime);
			int right = viewState.TimeToLayout(track.endTime);
			trackLayout.rect = QRect(left, currentTop, right - left, track.height);
			currentTop += track.height;

			if (track.expanded)
			{
				CalculateTrackLayout(layout, currentTop, currentIndent + 1, track, viewState, thumbTime, keyWidth, treeWidth, invisibleTracks);
			}
		}

		float maxPushOut = 0.0f;

		for (size_t i = 0; i < numTracks; ++i)
		{
			STrackLayout& track = layout->tracks[i];

			const size_t numElements = track.elements.size();
			for (size_t i = 0; i < numElements; ++i)
			{
				if (track.elements[i].type != STimelineElement::KEY)
				{
					continue;
				}

				for (size_t j = 0; j < numElements; ++j)
				{
					if ((track.elements[j].type != STimelineElement::KEY) || (j == i))
					{
						continue;
					}

					float distance = track.elements[j].rect.left() - track.elements[i].rect.left();
					float delta = clamp_tpl(1.0f - fabsf(distance) / keyWidth, 0.0f, 1.0f);

					if (delta == 0.0f)
					{
						continue;
					}

					float& pushOutDistance = track.elements[i].pushOutDistance;
					pushOutDistance += (i < j) ? -delta : delta;

					if (fabsf(pushOutDistance) > maxPushOut)
					{
						maxPushOut = fabsf(pushOutDistance);
					}
				}
			}
		}

		float maxPushOutNormalized = float(MAX_PUSH_OUT) / PUSH_OUT_DISTANCE;
		float pushOutScale = 1.0f;

		if (maxPushOut > maxPushOutNormalized && maxPushOut > 0.0f)
		{
			pushOutScale = maxPushOutNormalized / maxPushOut;
		}

		for (size_t i = 0; i < numTracks; ++i)
		{
			STrackLayout& track = layout->tracks[i];

			for (size_t i = 0; i < track.elements.size(); ++i)
			{
				track.elements[i].rect.translate(QPoint(0, int(pushOutScale * track.elements[i].pushOutDistance * PUSH_OUT_DISTANCE)));
			}
		}
	}

	void CalculateLayout(STimelineLayout* layout, STimelineContent& content, const STimelineViewState& viewState, const QLineEdit* pFilterLineEdit, float thumbTime, uint keyWidth, bool treeVisible)
	{
		layout->thumbPositionX = viewState.TimeToLayout(thumbTime);

		if (!content.track.tracks.empty())
		{
			layout->minStartTime = FLT_MAX;
			layout->maxEndTime = -FLT_MAX;
		}
		else
		{
			layout->minStartTime = 0.0f;
			layout->maxEndTime = 1.0f;
		}

		int currentTop = RULER_HEIGHT + VERTICAL_PADDING;
		const int treeWidth = treeVisible ? viewState.treeWidth : 0;

		std::unordered_set<const STimelineTrack*> invisibleTracks;
		if (pFilterLineEdit && !pFilterLineEdit->text().isEmpty())
		{
			FilterTracks(content.track, invisibleTracks, QtUtil::ToString(pFilterLineEdit->text()));
		}

		CalculateTrackLayout(layout, currentTop, 0, content.track, viewState, thumbTime, keyWidth, treeWidth, invisibleTracks);

		layout->size = QSize(viewState.TimeToLayout(layout->maxEndTime), currentTop + VERTICAL_PADDING);
	}

	STrackLayout *HitTestTrack(STrackLayouts& tracks, const QPoint& point)
	{
		auto findIter = std::upper_bound(tracks.begin(), tracks.end(), point.y(), [&](int y, const STrackLayout& track)
		{
			return y < track.rect.bottom();
		});

		if (findIter != tracks.end() && findIter->rect.contains(point))
		{
			return &(*findIter);
		}

		return nullptr;
	}

	void ForEachTrack(STimelineTrack& track, std::function<void (STimelineTrack& track)> fun)
	{
		fun(track);

		for (size_t i = 0; i < track.tracks.size(); ++i)
		{
			STimelineTrack& subTrack = *track.tracks[i];			
			ForEachTrack(subTrack, fun);
		}
	}

	void ForEachElement(STimelineTrack& track, std::function<void (STimelineTrack &track, STimelineElement& element)> fun)
	{
		ForEachTrack(track, [&](STimelineTrack& subTrack)
		{
			for (size_t i = 0; i < subTrack.elements.size(); ++i)
			{
				fun(subTrack, subTrack.elements[i]);
			}
		});
	}

	void ForEachElementWithIndex(STimelineTrack& track, std::function<void (STimelineTrack &track, STimelineElement& element, size_t elementIndex)> fun)
	{
		ForEachTrack(track, [&](STimelineTrack& subTrack)
		{
			for (size_t i = 0; i < subTrack.elements.size(); ++i)
			{
				fun(subTrack, subTrack.elements[i], i);
			}
		});
	}

	void ClearTrackSelection(STimelineTrack& track)
	{
		ForEachTrack(track, [](STimelineTrack &track) 
		{ 
			track.selected = false;
		});
	}

	void GetSelectedTracks(STimelineTrack& track, std::vector<STimelineTrack*>& tracks)
	{
		ForEachTrack(track, [&](STimelineTrack &track) 
		{ 
			if (track.selected)
			{
				tracks.push_back(&track);
			}
		});
	}

	void ClearElementSelection(STimelineTrack& track)
	{
		ForEachElement(track, [](STimelineTrack &track, STimelineElement& element) 
		{ 
			track.keySelectionChanged = track.keySelectionChanged || element.selected;
			element.selected = false; 
		});
	}

	void SetSelectedElementTimes(STimelineTrack& track, const std::vector<float>& times)
	{
		auto iter = times.begin();
		ForEachElement(track, [&](STimelineTrack &track, STimelineElement& element) 
		{ 
			if (element.selected) 
			{ 
				track.modified = true;
				element.start = *(iter++); 
			} 
		});
	}

	std::vector<float> GetSelectedElementTimes(STimelineTrack& track)
	{
		std::vector<float> times;
		ForEachElement(track, [&](STimelineTrack &track, STimelineElement& element) { if (element.selected) { times.push_back(element.start); } });
		return times;
	}

	VectorSet<float> GetSelectedElementsTimeSet(STimelineTrack& track)
	{
		VectorSet<float> timeSet;
		ForEachElement(track, [&](STimelineTrack &track, STimelineElement& element) { if (element.selected) { timeSet.insert(element.start); } });
		return timeSet;
	}

	typedef std::vector<std::pair<STimelineTrack*, STimelineElement*>> TSelectedElements;
	TSelectedElements GetSelectedElements(STimelineTrack& track)
	{
		TSelectedElements elements;

		ForEachElement(track, [&](STimelineTrack &track, STimelineElement& element) 
		{ 
			if (element.selected) 
			{ 
				elements.push_back(std::make_pair(&track, &element));
			} 
		});

		return elements;
	}

	void MoveSelectedElements(STimelineTrack& track, float delta)
	{
		ForEachElement(track, [&](STimelineTrack &track, STimelineElement & element) 
		{ 
			if (element.selected) 
			{ 
				track.modified = true;
				element.start += delta; 
			} 
		});
	}

	void DeletedMarkedElements(STimelineTrack& track)
	{
		ForEachTrack(track, [&](STimelineTrack &track) 
		{ 
			for (auto iter = track.elements.begin(); iter != track.elements.end();)
			{
				if (iter->deleted)
				{
					iter = track.elements.erase(iter);
				}
				else
				{
					++iter;
				}
			}
		});
	}

	void SelectElementsInRect(const STrackLayouts& tracks, const QRect& rect)
	{
		for (size_t j = 0; j < tracks.size(); ++j)
		{
			const STrackLayout& track = tracks[j];

			for (size_t i = 0; i < track.elements.size(); ++i)
			{
				const SElementLayout& element = track.elements[i];

				if ((element.caps & STimelineElement::CAP_SELECT) == 0)
				{
					continue;
				}

				STimelineTrack* pTimelineTrack = element.elementRef.pTrack;
				const bool bIsCompoundTrack = (pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

				if (element.rect.intersects(rect))
				{
					if (!bIsCompoundTrack)
					{
						if (!element.elementRef.GetElement().selected)
						{
							element.elementRef.GetElement().selected = true;
							element.elementRef.pTrack->keySelectionChanged = true;
						}								
					}
					else
					{
						const size_t numSubElements = element.subElements.size();

						for (size_t k = 0; k < numSubElements; ++k)
						{
							if (!element.subElements[k].GetElement().selected)
							{
								element.subElements[k].GetElement().selected = true;
								element.subElements[k].pTrack->keySelectionChanged = true;
							}
						}
					}
				}
			}

			SelectElementsInRect(track.tracks, rect);
		}
	}

	typedef std::vector<SElementLayout*> SElementLayoutPtrs;
	bool HitTestElements(STrackLayouts& tracks, const QRect& rect, SElementLayoutPtrs& out)
	{
		bool bHit = false;

		for (size_t j = 0; j < tracks.size(); ++j)
		{
			STrackLayout& track = tracks[j];

			for (size_t i = 0; i < track.elements.size(); ++i)
			{
				SElementLayout& element = track.elements[i];

				if (element.rect.intersects(rect))
				{
					out.push_back(&element);
					bHit = true;
				}
			}

			bHit = bHit || HitTestElements(track.tracks, rect, out);
		}

		return bHit;
	}

	enum EPass
	{
		PASS_BACKGROUND,
		PASS_SELECTION,
		PASS_SHADOW,
		PASS_MAIN,
		NUM_PASSES
	};

	QBrush PickTrackBrush(const QPalette& palette, const STrackLayout& track)
	{
		const QColor trackColor = InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Background), 0.96f);
		const QColor descriptionTrackColor = InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Background), 0.9f);
		const QColor compositeTrackColor = InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Background), 0.85f);
		const QColor selectionColor = InterpolateColor(palette.color(QPalette::Highlight), palette.color(QPalette::Background), 0.5f);

		const bool bIsDescriptionTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_DESCRIPTION_TRACK) != 0;
		const bool bIsCompositeTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

		const QColor color = bIsDescriptionTrack ? descriptionTrackColor : (bIsCompositeTrack ? compositeTrackColor : trackColor);
		return QBrush(track.pTimelineTrack->selected ? InterpolateColor(color, selectionColor, 0.3f) : color);
	}

	void DrawTracks(QPainter& painter, const uint startPass, const uint endPass, const STrackLayouts& tracks, const STimelineViewState& viewState,
	                const QPalette& palette, const QPoint &mousePos, bool hasFocus, bool treeVisible, int width, float keyRadius)
	{
		const int trackAreaLeft = viewState.LocalToLayout(QPoint(viewState.treeWidth, 0)).x();
		const int trackAreaRight = trackAreaLeft + width;

		const QColor textColor = palette.buttonText().color();
		QPen descriptionTextPen = QPen(InterpolateColor(textColor, palette.color(QPalette::Background), 0.5f));

		for (int i = startPass; i <= endPass; ++i)
		{
			EPass pass = (EPass)i;

			for (size_t j = 0; j < tracks.size(); ++j)
			{
				const STrackLayout& track = tracks[j];

				if (pass == PASS_BACKGROUND)
				{
					const bool bIsDescriptionTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_DESCRIPTION_TRACK) != 0;
					const bool bIsCompositeTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

					painter.setPen(Qt::NoPen);
					painter.setBrush(PickTrackBrush(palette, track));
					QRect backgroundRect = track.rect;
					backgroundRect.setLeft(-viewState.scrollPixels.x());
					backgroundRect.setWidth(width);
					painter.drawRect(backgroundRect);

					if (bIsDescriptionTrack)
					{
						painter.setPen(descriptionTextPen);
						QRect textRect = track.rect;
						textRect.moveLeft(textRect.left() - viewState.scrollPixels.x() + TRACK_DESCRIPTION_INDENT);
						textRect.moveTop(textRect.top() + 1);
						painter.drawText(textRect, QString(track.pTimelineTrack->name));
					}

					const int lineY = track.rect.bottom() + 1;
					painter.setPen(QPen(InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Background), 0.75f)));
					painter.drawLine(QPoint(trackAreaLeft, lineY), QPoint(trackAreaRight, lineY));
					continue;
				}

				for (size_t i = 0; i < track.elements.size(); ++i)
				{
					const SElementLayout& element = track.elements[i];
					QRectF rect = QRectF(element.rect);
					float ratio = 1.0f;

					if (rect.width() != 0)
					{
						ratio = rect.height() != 0 ? rect.width() / float(rect.height()) : 1.0f;
					}

					const bool bSelected = element.IsSelected();

					if (element.type == STimelineElement::KEY)
					{
						float rx = keyRadius * 200.0f / ratio;
						float ry = keyRadius * 200.0f;

						if (pass == PASS_SELECTION)
						{
							if (bSelected)
							{
								QRectF selectionRect = rect.adjusted(-SELECTION_WIDTH * 0.5f + 0.5f, -SELECTION_WIDTH * 0.5f + 0.5f, SELECTION_WIDTH * 0.5f - 0.5f, SELECTION_WIDTH * 0.5f - 0.5f);
								painter.setPen(QPen(palette.color(hasFocus ? QPalette::Highlight : QPalette::Shadow), SELECTION_WIDTH));
								painter.setBrush(QBrush(Qt::NoBrush));
								painter.drawRoundedRect(selectionRect, rx, ry, Qt::RelativeSize);
							}
						}
						else if (pass != PASS_SHADOW)
						{
							QRectF shadowRect = rect.adjusted(-1.0f, -0.5f, 1.0f, 1.5f);
							painter.setPen(QPen(QColor(0, 0, 0, 128), 2.0f));
							painter.setBrush(QBrush(Qt::NoBrush));
							painter.drawRoundedRect(shadowRect, rx, ry, Qt::RelativeSize);

							painter.setPen(QPen(QColor(element.color.r, element.color.g, element.color.b, 255)));
							painter.setBrush(QBrush(QColor(element.color.r, element.color.g, element.color.b, 255)));
							painter.drawRoundedRect(rect, rx, ry, Qt::RelativeSize);
						}
					}
					else
					{
						float radius = 0.2f;
						float rx = radius * 200.0f / ratio;
						float ry = radius * 200.0f;

						if (pass == PASS_SHADOW)
						{
							if (bSelected)
							{
								QRectF selectionRect = rect.adjusted(-SELECTION_WIDTH * 0.5f + 0.5f, -SELECTION_WIDTH * 0.5f + 0.5f, SELECTION_WIDTH * 0.5f - 0.5f, SELECTION_WIDTH * 0.5f - 0.5f);
								painter.setPen(QPen(palette.color(hasFocus ? QPalette::Highlight : QPalette::Shadow), SELECTION_WIDTH));
								painter.setBrush(QBrush(Qt::NoBrush));
								painter.drawRoundedRect(selectionRect, rx, ry, Qt::RelativeSize);
							}
						}
						else if (pass == PASS_SHADOW)
						{
							QRectF shadowRect = rect.adjusted(0.0f, 0.0f, 0.0f, 1.0f);
							painter.setPen(QPen(QColor(0, 0, 0, 128), 2.0f));
							painter.setBrush(QBrush(Qt::NoBrush));
							painter.drawRoundedRect(shadowRect, radius * 200.0f / ratio, radius * 200.0f, Qt::RelativeSize);
						}
						else
						{
							painter.setPen(QPen(QColor(element.color.r, element.color.g, element.color.b, 255)));
							painter.setBrush(QBrush(QColor(element.color.r, element.color.g, element.color.b, 128)));
							painter.drawRoundedRect(rect, radius * 200.0f / ratio, radius * 200.0f, Qt::RelativeSize);
						}
					}
				}
			}
		}
	}
	
	void DrawSelectionLines(QPainter& painter, const QPalette& palette, const STimelineViewState &viewState, STimelineContent& content, int rulerPrecision, int width, int height, float time, float timeUnitScale, bool hasFocus)
	{
		const VectorSet<float> times = GetSelectedElementsTimeSet(content.track);

		QColor indicatorColor = palette.color(hasFocus ? QPalette::Highlight : QPalette::Shadow);
		indicatorColor.setAlpha(70);

		for (auto iter = times.begin(); iter != times.end(); ++iter)
		{
			const float indicatorX = viewState.TimeToLocal(*iter) + 0.5f;
			painter.setPen(indicatorColor);
			painter.drawLine(QPointF(indicatorX, 0), QPointF(indicatorX, height));
		}
	}

	void DrawTree(QPainter& painter, const QRect& treeRect, const QPalette& palette, QWidget* timeline, const STimelineContent& content, const STrackLayouts& tracks, const STimelineViewState& viewState)
	{
		painter.save();

		painter.fillRect(treeRect, palette.color(QPalette::Background));
		painter.setClipRect(treeRect);
		painter.setClipping(true);
		
		QTextOption textOption;
		textOption.setWrapMode(QTextOption::NoWrap);

		const QColor textColor = palette.buttonText().color();

		QStyleOption opt;
		opt.palette = palette;
		opt.state = QStyle::State_Enabled;
		opt.rect = QRect(treeRect.left(), treeRect.top() - 1, treeRect.width(), treeRect.height() + 2);

		// Draw frame around tree
		timeline->style()->drawPrimitive(QStyle::PE_Frame, &opt, &painter, timeline);

		for (size_t i = 0; i < tracks.size(); ++i)
		{
			const STrackLayout& track = tracks[i];

			const QRect backgroundRect(1, track.rect.top() + 1, viewState.treeWidth - SPLITTER_WIDTH - 1, track.rect.height() - 1);

			const bool bIsDescriptionTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_DESCRIPTION_TRACK) != 0;
			const bool bIsCompositeTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

			painter.setPen(Qt::NoPen);
			painter.setBrush(PickTrackBrush(palette, track));
			painter.drawRect(backgroundRect);

			const int branchLeft = TREE_LEFT_MARGIN + track.indent * TREE_INDENT_MULTIPLIER;

			if (track.pTimelineTrack->tracks.size() > 0)
			{
				QStyleOption opt;
				opt.rect = QRect(branchLeft, track.rect.top() + 1, TREE_BRANCH_INDICATOR_SIZE, track.rect.height() - 2);
				opt.state = QStyle::State_Enabled | QStyle::State_Children;
				opt.state |= track.pTimelineTrack->expanded ? QStyle::State_Open : QStyle::State_None;

				timeline->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, &painter, timeline);
			}

			const int textLeft = branchLeft + TREE_BRANCH_INDICATOR_SIZE + 4;
			const int textWidth = std::max(treeRect.width() - textLeft - 4, 0);
			const QRect textRect(textLeft, track.rect.top() + 1, textWidth, track.rect.height() - 2);
			painter.setPen(QPen(textColor));
			painter.drawText(textRect, QString(track.pTimelineTrack->name), textOption);
		}

		for (size_t i = 0; i < tracks.size(); ++i)
		{
			const STrackLayout& track = tracks[i];

			const int lineY = track.rect.bottom() + 1;
			painter.setPen(QPen(InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Background), 0.75f)));
			painter.drawLine(QPoint(0, lineY), QPoint(treeRect.width(), lineY));
		}

		painter.restore();
	}

	void DrawSplitter(QPainter& painter, const QRect& splitterRect, const QPalette& palette, QWidget* timeline)
	{
		painter.fillRect(splitterRect, palette.color(QPalette::Background));

		QStyleOption opt;
		opt.palette = palette;
		opt.state = QStyle::State_Enabled;

		// Draw frame around splitter
		opt.rect = QRect(splitterRect.left(), splitterRect.top() - 1, splitterRect.width(), splitterRect.height() + 2);
		timeline->style()->drawPrimitive(QStyle::PE_Frame, &opt, &painter, timeline);

		// Draw resize handle dots
		opt.rect = QRect(splitterRect.left(), splitterRect.top() - 1, splitterRect.width() - 2, splitterRect.height() + 2);
		timeline->style()->drawPrimitive(QStyle::PE_IndicatorDockWidgetResizeHandle, &opt, &painter, timeline);
	}
}

struct CTimeline::SMouseHandler
{
	virtual void mousePressEvent(QMouseEvent* ev) {}
	virtual void mouseDoubleClickEvent(QMouseEvent* ev) {}
	virtual void mouseMoveEvent(QMouseEvent* ev) {}
	virtual void mouseReleaseEvent(QMouseEvent* ev) {}
	virtual void focusOutEvent(QFocusEvent* ev) {}
	virtual void paintOver(QPainter& painter) {}
};

struct CTimeline::SSelectionHandler : SMouseHandler
{
	CTimeline* m_timeline;
	QPoint m_startPoint;
	QRect m_rect;
	bool m_add;

	SSelectionHandler(CTimeline* timeline, bool add): m_timeline(timeline), m_add(add) {}

	void mousePressEvent(QMouseEvent* ev) override
	{
		m_startPoint = m_timeline->m_viewState.LocalToLayout(ev->pos());
		m_rect = QRect(m_startPoint, m_startPoint + QPoint(1, 1));
	}

	void mouseMoveEvent(QMouseEvent* ev) override
	{
		m_rect = QRect(m_startPoint, m_timeline->m_viewState.LocalToLayout(ev->pos()) + QPoint(1, 1));
		Apply(true);
	}

	void Apply(bool continuous)
	{
		if (m_timeline->m_pContent)
		{
			const TSelectedElements selectedElements = GetSelectedElements(m_timeline->m_pContent->track);

			ClearElementSelection(m_timeline->m_pContent->track);
			SelectElementsInRect(m_timeline->m_layout->tracks, m_rect);

			if (selectedElements != GetSelectedElements(m_timeline->m_pContent->track))
			{
				m_timeline->SignalSelectionChanged(continuous);
			}			
		}
	}

	void mouseReleaseEvent(QMouseEvent* ev) override
	{
		Apply(false);
	}

	void paintOver(QPainter& painter) override
	{
		painter.save();
		QColor highlightColor = m_timeline->palette().color(QPalette::Highlight);
		QColor highlightColorA = QColor(highlightColor.red(), highlightColor.green(), highlightColor.blue(), 128);
		painter.setPen(QPen(highlightColor));
		painter.setBrush(QBrush(highlightColorA));
		painter.drawRect(QRectF(m_rect));
		painter.restore();
	}
};

static STimelineElement* NextSelectedElement(const SElementLayoutPtrs& array, STimelineElement* nextToValue, STimelineElement* defaultValue)
{
	for (size_t i = 0; i < array.size(); ++i)
		if (&array[i]->elementRef.GetElement() == nextToValue)
			return &array[(i + 1) % array.size()]->elementRef.GetElement();
	return defaultValue;
}

struct CTimeline::SMoveHandler : SMouseHandler
{
	CTimeline* m_timeline;
	QPoint m_startPoint;
	bool m_cycleSelection;
	float m_startTime;
	float m_newTime;
	std::vector<float> m_elementTimes;

	SMoveHandler(CTimeline* timeline, bool cycleSelection) : m_timeline(timeline), m_cycleSelection(cycleSelection) {}

	void mousePressEvent(QMouseEvent* ev) override
	{
		m_startTime = m_timeline->m_time;
		m_newTime = m_startTime;
		QPoint currentPos = ev->pos();
		m_startPoint = m_timeline->m_viewState.LocalToLayout(QPoint(currentPos));		
		m_elementTimes = GetSelectedElementTimes(m_timeline->m_pContent->track);
	}

	float CalculateShortestSnapOffset(const STimelineElements& elements, float deltaTime) const
	{
		if (m_timeline->m_framesToSnap == 0 || !m_timeline->m_snapToFrames || elements.empty())
		{
			return 0;
		}

		std::vector<float> times;
		times.reserve(elements.size() * 2);

		for (size_t i = 0; i < elements.size(); ++i)
		{
			times.push_back(elements[i].start);

			if (elements[i].type == STimelineElement::CLIP)
			{
				times.push_back(elements[i].end);
			}
		}

		float smallestOffset = FLT_MAX;

		for (size_t i = 0; i < times.size(); ++i)
		{
			float t = times[i] + deltaTime;
			float offset = m_timeline->ClampAndSnapTime(t, m_timeline->m_snapToFrames) - t;

			if (fabsf(offset) < smallestOffset)
			{
				smallestOffset = offset;
			}
		}

		return smallestOffset;
	}

	void mouseMoveEvent(QMouseEvent* ev) override
	{
		if (m_timeline->m_viewState.widthPixels == 0)
		{
			return;
		}

		QPoint currentPos = ev->pos();
		int delta = m_timeline->m_viewState.LocalToLayout(currentPos).x() - m_startPoint.x();

		const TSelectedElements selectedElements = GetSelectedElements(m_timeline->m_pContent->track);

		float moveDelta = float(delta) / m_timeline->m_viewState.widthPixels * m_timeline->m_viewState.visibleDistance;
		//float snapOffset = CalculateShortestSnapOffset(m_elements, moveDelta);
		float deltaTime = moveDelta; // + snapOffset;

		SetSelectedElementTimes(m_timeline->m_pContent->track, m_elementTimes);

		float minDeltaTime = -std::numeric_limits<float>::max();
		float maxDeltaTime = std::numeric_limits<float>::max();			

		for (size_t i = 0; i < selectedElements.size(); ++i)
		{
			const STimelineTrack& track = *selectedElements[i].first;
			const STimelineElement& element = *selectedElements[i].second;

			float minStartDelta = track.startTime - element.start;
			minDeltaTime = std::max(minStartDelta, minDeltaTime);
			float maxEndDelta = track.endTime - (element.type == element.CLIP ? element.end : element.start);
			maxDeltaTime = std::min(maxEndDelta, maxDeltaTime);
		}

		deltaTime = clamp_tpl(deltaTime, minDeltaTime, maxDeltaTime);

		m_newTime = m_startTime + deltaTime;
		
		MoveSelectedElements(m_timeline->m_pContent->track, deltaTime);
			
		m_timeline->ContentChanged(true);

		m_timeline->setCursor(Qt::SizeHorCursor);
		m_cycleSelection = false;
	}

	void focusOutEvent(QFocusEvent* ev)
	{
		SetSelectedElementTimes(m_timeline->m_pContent->track, m_elementTimes);
		m_timeline->UpdateLayout();
	}

	void mouseReleaseEvent(QMouseEvent* ev)
	{
		if (m_cycleSelection)
		{
			SElementLayoutPtrs hitElements;
			QPoint posInLayoutSpace = m_timeline->m_viewState.LocalToLayout(ev->pos());
			bool bHit = HitTestElements(m_timeline->m_layout->tracks, QRect(posInLayoutSpace - QPoint(2, 2), posInLayoutSpace + QPoint(2, 2)), hitElements);

			if (!hitElements.empty())
			{
				TSelectedElements selectedElements = GetSelectedElements(m_timeline->m_pContent->track);
				if (selectedElements.size() == 1)
				{
					STimelineElement* lastSelection = selectedElements[0].second;

					ClearElementSelection(m_timeline->m_pContent->track);
					NextSelectedElement(hitElements, lastSelection, &hitElements.back()->elementRef.GetElement())->selected = true;
					m_timeline->SignalSelectionChanged(false);
				}
				else
				{
					ClearElementSelection(m_timeline->m_pContent->track);
					hitElements.back()->elementRef.GetElement().selected = true;
					m_timeline->SignalSelectionChanged(false);
				}
			}
		}

		m_timeline->ContentChanged(false);
	}
};

struct CTimeline::SPanHandler : SMouseHandler
{
	CTimeline* m_timeline;
	QPoint m_startPoint;
	float m_startOrigin;

	SPanHandler(CTimeline* timeline) : m_timeline(timeline)
	{
	}

	void mousePressEvent(QMouseEvent* ev) override
	{
		m_startPoint = QPoint(int(ev->x()), int(ev->y()));
		m_startOrigin = m_timeline->m_viewState.viewOrigin;
	}

	void mouseMoveEvent(QMouseEvent* ev) override
	{
		QPoint pos(int(ev->x()), int(ev->y()));
		float delta = 0.0f;

		if (m_timeline->m_viewState.widthPixels != 0)
		{
			delta = (pos - m_startPoint).x() * m_timeline->m_viewState.visibleDistance / m_timeline->m_viewState.widthPixels;
		}

		m_timeline->m_viewState.viewOrigin = m_startOrigin + delta;
		ClampViewOrigin(&m_timeline->m_viewState, *m_timeline->m_layout);
	}

	void mouseReleaseEvent(QMouseEvent* ev) override
	{
	}
};

struct CTimeline::SScrubHandler : SMouseHandler
{
	CTimeline* m_timeline;
	SScrubHandler(CTimeline* timeline) : m_timeline(timeline) {}
	float m_startThumbPosition;
	QPoint m_startPoint;

	void SetThumbPositionX(int positionX)
	{
		int nThumbEndPadding = (THUMB_WIDTH / 2) + 2;
		float fVisualRange = float(m_timeline->m_viewState.widthPixels - nThumbEndPadding * 2);
		float time = m_timeline->m_viewState.LayoutToTime(positionX);

		m_timeline->ClampAndSetTime(time, false);
	}

	void mousePressEvent(QMouseEvent* ev) override
	{
		QPoint point = QPoint(ev->pos().x(), ev->pos().y());

		QPoint posInLayout = m_timeline->m_viewState.LocalToLayout(point);

		int thumbPositionX = m_timeline->m_viewState.TimeToLayout(m_timeline->m_time);
		QRect thumbRect(thumbPositionX - THUMB_WIDTH / 2, 0, THUMB_WIDTH, THUMB_HEIGHT);

		if (!thumbRect.contains(posInLayout))
		{
			SetThumbPositionX(m_timeline->m_viewState.LocalToLayout(point).x());
		}

		m_startThumbPosition = m_timeline->m_time;
		m_startPoint = point;
	}

	void Apply(QMouseEvent* ev, bool continuous)
	{
		QPoint point = QPoint(ev->pos().x(), ev->pos().y());

		bool shift = ev->modifiers().testFlag(Qt::ShiftModifier);
		bool control = ev->modifiers().testFlag(Qt::ControlModifier);

		float delta = 0.0f;

		if (m_timeline->m_viewState.widthPixels != 0)
		{
			delta = (point.x() - m_startPoint.x()) * m_timeline->m_viewState.visibleDistance / m_timeline->m_viewState.widthPixels;
		}

		if (shift)
		{
			delta *= 0.01f;
		}

		if (control)
		{
			delta *= 0.1f;
		}

		m_timeline->ClampAndSetTime(m_startThumbPosition + delta, true);
	}

	void mouseMoveEvent(QMouseEvent* ev) override
	{
		Apply(ev, true);
	}

	void mouseReleaseEvent(QMouseEvent* ev) override
	{
		Apply(ev, false);
	}
};

struct CTimeline::SSplitterHandler : SMouseHandler
{
	CTimeline* m_timeline;
	int m_offset;
	bool m_movedSlider;

	SSplitterHandler(CTimeline* timeline) : m_timeline(timeline), m_offset(0), m_movedSlider(false) {}

	void mousePressEvent(QMouseEvent* ev) override
	{
		m_offset = m_timeline->m_viewState.treeWidth - ev->pos().x();
	}

	void mouseReleaseEvent(QMouseEvent* ev) override
	{
		if (!m_movedSlider)
		{
			STimelineViewState& viewState = m_timeline->m_viewState;

			if (viewState.treeWidth == SPLITTER_WIDTH)
			{
				viewState.treeWidth = viewState.treeLastOpenedWidth;
			}
			else
			{
				viewState.treeLastOpenedWidth = viewState.treeWidth;
				viewState.treeWidth = SPLITTER_WIDTH;
			}

			m_timeline->UpdateLayout();
			m_timeline->update();
		}
	}

	void mouseMoveEvent(QMouseEvent* ev) override
	{
		m_timeline->setCursor(QCursor(Qt::SplitHCursor));
		uint treeWidth = clamp_tpl<int>(ev->pos().x(), SPLITTER_WIDTH, m_timeline->width()) + m_offset;
		m_timeline->m_viewState.treeWidth = treeWidth;
		m_timeline->m_viewState.treeLastOpenedWidth = treeWidth;
		m_timeline->UpdateLayout();
		m_timeline->update();
		m_movedSlider = true;
	}
};

struct CTimeline::STreeMouseHandler : SMouseHandler
{
	CTimeline* m_timeline;
	STreeMouseHandler(CTimeline* timeline) : m_timeline(timeline) {}

	void mousePressEvent(QMouseEvent* ev) override
	{
		const bool bCtrlPressed = (ev->modifiers() & Qt::CTRL) != 0;
		const bool bShiftPressed = (ev->modifiers() & Qt::SHIFT) != 0;

		STrackLayout* pTrackLayout = m_timeline->GetTrackLayoutFromPos(ev->pos());

		if (!pTrackLayout)
		{
			if (!bShiftPressed && !bCtrlPressed)
			{
				ClearTrackSelection(m_timeline->m_pContent->track);
			}
		}
		else
		{
			const int left = TREE_LEFT_MARGIN + pTrackLayout->indent * TREE_INDENT_MULTIPLIER;
			const int right = left + TREE_BRANCH_INDICATOR_SIZE;

			const int x = ev->pos().x();

			if (x >= left && x <= right)
			{
				ToggleTrackExpansion(pTrackLayout);
			}
			else
			{
				const bool bPreviousState = pTrackLayout->pTimelineTrack->selected;

				if (!bCtrlPressed)
				{
					ClearTrackSelection(m_timeline->m_pContent->track);
				}

				if (bCtrlPressed)
				{
					pTrackLayout->pTimelineTrack->selected = !bPreviousState;
				}
				else if (bShiftPressed)
				{
					STrackLayouts& tracks = m_timeline->m_layout->tracks;

					auto startFindIter = std::find_if(tracks.begin(), tracks.end(), [=](const STrackLayout& track) { return (pTrackLayout == &track); });
					auto endFindIter = std::find_if(tracks.begin(), tracks.end(), [=](const STrackLayout& track) { return (m_timeline->m_pLastSelectedTrack == &track); });

					if (startFindIter != tracks.end() && endFindIter != tracks.end())
					{
						if (startFindIter > endFindIter)
						{
							std::swap(startFindIter, endFindIter);
						}

						for (auto iter = startFindIter; iter <= endFindIter; ++iter)
						{
							iter->pTimelineTrack->selected = true;
						}
					}
				}
				else
				{
					pTrackLayout->pTimelineTrack->selected = true;
				}

				if (!bShiftPressed && pTrackLayout->pTimelineTrack->selected)
				{
					m_timeline->m_pLastSelectedTrack = pTrackLayout;
				}
			}
		}
	}

	void mouseDoubleClickEvent(QMouseEvent* ev) override
	{
		if (ev->modifiers() == 0)
		{
			STrackLayout* pTrackLayout = m_timeline->GetTrackLayoutFromPos(ev->pos());
			ToggleTrackExpansion(pTrackLayout);
		}
	}

private:
	void ToggleTrackExpansion(STrackLayout* pTrackLayout)
		{
		if (pTrackLayout && pTrackLayout->pTimelineTrack)
		{
			pTrackLayout->pTimelineTrack->expanded = !pTrackLayout->pTimelineTrack->expanded;
			m_timeline->UpdateLayout();
			m_timeline->update();
		}
	}
};

// ---------------------------------------------------------------------------

CTimeline::CTimeline(QWidget* parent)
	: QWidget(parent)
	, m_cycled(true)
	, m_sizeToContent(false)
	, m_framesToSnap(0)
	, m_snapToFrames(false)
	, m_treeVisible(false)
	, m_selIndicators(false)
	, m_layout(new STimelineLayout)
	, m_keyWidth(DEFAULT_KEY_WIDTH)
	, m_keyRadius(DEFAULT_KEY_RADIUS)
	, m_pContent(nullptr)
	, m_timeUnitScale(1.0f)
	, m_time(0.0f)
	, m_pFilterLineEdit(nullptr)
	, m_pLastSelectedTrack(nullptr)
{
	setMinimumWidth(THUMB_WIDTH * 3);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	setFocusPolicy(Qt::WheelFocus);
	setMouseTracking(true);

	m_viewState.visibleDistance = 1.0f;
}

CTimeline::~CTimeline()
{
}

void CTimeline::paintEvent(QPaintEvent* ev)
{
	const QPoint mousePos = mapFromGlobal(QCursor::pos());

	QPainter painter(this);
	painter.save();
	painter.translate(0.5f, 0.5f);

	if (m_viewState.visibleDistance != 0)
	{
		float totalDuration = m_layout->maxEndTime - m_layout->minStartTime;
		m_viewState.scrollPixels = QPoint(m_viewState.ScrollOffset(m_viewState.viewOrigin), 0);
		m_viewState.maxScrollX = int(m_viewState.widthPixels * totalDuration / m_viewState.visibleDistance) - m_viewState.widthPixels;
	}
	else
	{
		m_viewState.scrollPixels = QPoint(0, 0);
		m_viewState.maxScrollX = 0;
	}

	const QPoint localToLayoutTranslate = m_viewState.LayoutToLocal(QPoint(0, 0));

	int rulerPrecision = 0;

	painter.translate(localToLayoutTranslate);
	painter.setRenderHint(QPainter::Antialiasing);

	DrawTracks(painter, PASS_BACKGROUND, PASS_BACKGROUND, m_layout->tracks, m_viewState,
	           palette(), mousePos, hasFocus(), m_treeVisible, width(), m_keyRadius);

	painter.translate(-localToLayoutTranslate);

	DrawingPrimitives::SRulerOptions rulerOptions;	
	rulerOptions.m_rect = QRect(m_viewState.treeWidth, 0, size().width() - m_viewState.treeWidth, RULER_HEIGHT + 2);
	rulerOptions.m_visibleRange = Range(m_viewState.LocalToTime(m_viewState.treeWidth) * m_timeUnitScale, m_viewState.LocalToTime(size().width()) * m_timeUnitScale);
	rulerOptions.m_rulerRange = Range(m_layout->minStartTime * m_timeUnitScale, m_layout->maxEndTime * m_timeUnitScale);
	rulerOptions.m_markHeight = RULER_MARK_HEIGHT;
	rulerOptions.m_shadowSize = RULER_SHADOW_HEIGHT;
	DrawingPrimitives::DrawRuler(painter, palette(), rulerOptions, &rulerPrecision);

	if (m_pContent && isEnabled())
	{
		DrawingPrimitives::STimeSliderOptions timeSliderOptions;
		timeSliderOptions.m_rect = rect();
		timeSliderOptions.m_precision = rulerPrecision;
		timeSliderOptions.m_position = m_viewState.TimeToLocal(m_time);
		timeSliderOptions.m_time = m_time * m_timeUnitScale;
		timeSliderOptions.m_bHasFocus = hasFocus();
		DrawingPrimitives::DrawTimeSlider(painter, palette(), timeSliderOptions);

		DrawSelectionLines(painter, palette(), m_viewState, *m_pContent, rulerPrecision, width(), height(), m_time, m_timeUnitScale, hasFocus());
	}

	painter.translate(localToLayoutTranslate);

	DrawTracks(painter, PASS_SELECTION, PASS_MAIN, m_layout->tracks, m_viewState,
	           palette(), mousePos, hasFocus(), m_treeVisible, width(), m_keyRadius);

	if (m_mouseHandler)
	{
		m_mouseHandler->paintOver(painter);
	}

	painter.translate(-localToLayoutTranslate);

	if (m_viewState.scrollPixels.x() < 0)
	{
		QRect rect(m_viewState.treeWidth, 0, SCROLL_SHADOW_WIDTH, height());
		QLinearGradient grad(rect.left(), rect.top(), rect.right(), rect.top());
		grad.setColorAt(0.0f, QColor(0, 0, 0, 96));
		grad.setColorAt(1.0f, QColor(0, 0, 0, 0));
		painter.fillRect(rect, QBrush(grad));
	}

	float totalDuration = m_layout->maxEndTime - m_layout->minStartTime;

	if (m_viewState.scrollPixels.x() > -m_viewState.maxScrollX)
	{
		QRect rect(width() - SCROLL_SHADOW_WIDTH, 0, SCROLL_SHADOW_WIDTH, height());
		QLinearGradient grad(rect.left(), rect.top(), rect.right(), rect.top());
		grad.setColorAt(0.0f, QColor(0, 0, 0, 0));
		grad.setColorAt(1.0f, QColor(0, 0, 0, 96));
		painter.fillRect(rect, QBrush(grad));
	}

	{
		QColor color = palette().color(QPalette::Dark);
		color.setAlpha(128);
		painter.setPen(QPen(color));
		painter.drawLine(QPoint(0, 0), QPoint(0, height()));
		painter.drawLine(QPoint(width() - 1, 0), QPoint(width() - 1, height()));
		painter.drawLine(QPoint(1, 0), QPoint(width() - 1, 0));
		painter.drawLine(QPoint(0, height()), QPoint(width() - 1, height()));
	}

	painter.restore();

	if (m_treeVisible)
	{
		if (m_pContent)
		{
			QRect treeRect(0, 0, m_viewState.treeWidth - SPLITTER_WIDTH + 1, height());
			DrawTree(painter, treeRect, palette(), this, *m_pContent, m_layout->tracks, m_viewState);
		}

		QRect splitterRect(m_viewState.treeWidth - SPLITTER_WIDTH, 0, SPLITTER_WIDTH, height());
		DrawSplitter(painter, splitterRect, palette(), this);
	}
	
	if (!isEnabled())
	{
		QColor disabledOverlayColor = palette().color(QPalette::Disabled, QPalette::Button);
		disabledOverlayColor.setAlpha(128);
		painter.fillRect(0, 0, width(), height(), QBrush(disabledOverlayColor));
	}
}

void CTimeline::keyPressEvent(QKeyEvent* ev)
{
	const QPoint mousePos = mapFromGlobal(QCursor::pos());
	QMouseEvent mouseEvent(QEvent::MouseMove, mousePos, Qt::NoButton, Qt::NoButton, ev->modifiers());
	mouseMoveEvent(&mouseEvent);
	int k = ev->key() | ev->modifiers();


	if (QKeySequence(k) == QKeySequence(Qt::Key_Z | Qt::CTRL))
		SignalUndo();
	else if (QKeySequence(k) == QKeySequence(Qt::Key_Y | Qt::CTRL))
		SignalRedo();
	else
	HandleKeyEvent(k);
}

void CTimeline::keyReleaseEvent(QKeyEvent* ev)
{
	const QPoint mousePos = mapFromGlobal(QCursor::pos());
	QMouseEvent mouseEvent(QEvent::MouseMove, mousePos, Qt::NoButton, Qt::NoButton, ev->modifiers());
	mouseMoveEvent(&mouseEvent);
}

bool CTimeline::HandleKeyEvent(int k)
{
	QKeySequence key(k);

	if (key == QKeySequence(Qt::Key_Delete))
	{
		OnMenuDelete();
		return true;
	}

	if (key == QKeySequence(Qt::Key_D))
	{
		OnMenuDuplicate();
		return true;
	}

	if (key == QKeySequence(Qt::Key_Home))
	{
		m_time = 0.0f;
		update();
		SignalScrub(false);
		return true;
	}
	if (key == QKeySequence(Qt::Key_End))
	{		
		float endTime = 0.0f;
		for (size_t i = 0; i < m_pContent->track.tracks.size(); ++i)
			if (m_pContent->track.tracks[i]->endTime > endTime)
				endTime = m_pContent->track.tracks[i]->endTime;
		m_time = endTime;
		update();
		SignalScrub(false);
		return true;
	}
	if (key == QKeySequence(Qt::Key_X) || key == QKeySequence(Qt::Key_PageUp))
	{
		OnMenuPreviousKey();
		return true;
	}
	if (key == QKeySequence(Qt::Key_C) || key == QKeySequence(Qt::Key_PageDown))
	{
		OnMenuNextKey();
		return true;
	}

	if (k == Qt::Key_Comma || k == Qt::Key_Left)
	{
		OnMenuPreviousFrame();
		return true;
	}

	if (k == Qt::Key_Period || k == Qt::Key_Right)
	{
		OnMenuNextFrame();
		return true;
	}

	if (key == QKeySequence(Qt::Key_Space))
	{
		OnMenuPlay();
		return true;
	}

	if (k >= Qt::Key_0 && k <= Qt::Key_9)
	{
		int number = k - int(Qt::Key_0);
		SignalNumberHotkey(number);
		return true;
	}

	return false;
}

void CTimeline::mousePressEvent(QMouseEvent* ev)
{
	setFocus();

	const bool bInTreeArea = m_treeVisible && (ev->x() <= m_viewState.treeWidth);

	if (ev->button() == Qt::LeftButton)
	{
		QPoint posInLayout = m_viewState.LocalToLayout(ev->pos());

		if (bInTreeArea)
		{
			const bool bOverSplitter = ev->x() >= (m_viewState.treeWidth - SPLITTER_WIDTH);

			if (bOverSplitter)
			{
				m_mouseHandler.reset(new SSplitterHandler(this));
				m_mouseHandler->mousePressEvent(ev);
				update();
			}
			else
			{
				m_mouseHandler.reset(new STreeMouseHandler(this));
				m_mouseHandler->mousePressEvent(ev);
				update();
			}
		}
		else if (ev->y() < RULER_HEIGHT)
		{
			m_mouseHandler.reset(new SScrubHandler(this));
			m_mouseHandler->mousePressEvent(ev);
			update();
		}
		else
		{
			QPoint posInLayoutSpace = m_viewState.LocalToLayout(ev->pos());

			SElementLayoutPtrs hitElements;
			bool bHit = HitTestElements(m_layout->tracks, QRect(posInLayoutSpace - QPoint(2, 2), posInLayoutSpace + QPoint(2, 2)), hitElements);

			if (ev->modifiers() & Qt::SHIFT || ev->modifiers() & Qt::CTRL)
			{
				if (bHit)
				{
					hitElements.back()->SetSelected(hitElements.back()->IsSelected());
					mouseMoveEvent(ev);
					update();
				}
				else
				{
					m_mouseHandler.reset(new SSelectionHandler(this, true));
					m_mouseHandler->mousePressEvent(ev);
				}
			}
			else
			{
				if (bHit)
				{
					bool useExistingSelection = std::any_of(hitElements.begin(), hitElements.end(), [](const SElementLayout * element) { return element->IsSelected(); });

					if (!useExistingSelection)
					{
						const TSelectedElements selectedElements = GetSelectedElements(m_pContent->track);

						ClearElementSelection(m_pContent->track);
						hitElements.back()->SetSelected(true);

						if (selectedElements != GetSelectedElements(m_pContent->track))
						{
							SignalSelectionChanged(false);
						}
					}

					bool cycleSelection = useExistingSelection;
					m_mouseHandler.reset(new SMoveHandler(this, cycleSelection));
					m_mouseHandler->mousePressEvent(ev);
					update();
				}
				else
				{
					m_mouseHandler.reset(new SSelectionHandler(this, false));
					m_mouseHandler->mousePressEvent(ev);
					update();
				}
			}
		}
	}
	else if (ev->button() == Qt::MiddleButton)
	{
		if (!bInTreeArea)
		{
			m_mouseHandler.reset(new SPanHandler(this));
			m_mouseHandler->mousePressEvent(ev);
			update();
		}
	}
	else if (ev->button() == Qt::RightButton)
	{
		if (bInTreeArea)
		{
			std::vector<STimelineTrack*> selectedTracks;
			GetSelectedTracks(m_pContent->track, selectedTracks);

			STrackLayout*	pLayout = GetTrackLayoutFromPos(ev->pos());
			if (pLayout)
			{				
				if (!stl::find(selectedTracks, pLayout->pTimelineTrack))
				{
					ClearTrackSelection(m_pContent->track);
					pLayout->pTimelineTrack->selected = true;
				}

				SignalTreeContextMenu(mapToGlobal(ev->pos()));
			}
		}
		else
		{
			QMenu menu;
			bool hasSelection = false;
			ForEachElement(m_pContent->track, [&](STimelineTrack &t, STimelineElement& e) { if (e.selected) hasSelection = true; });

			menu.addAction("Selection to Cursor", this, SLOT(OnMenuSelectionToCursor()))->setEnabled(hasSelection);
			QAction* duplicateAction = menu.addAction("Duplicate", this, SLOT(OnMenuDuplicate()), QKeySequence("D"));
			duplicateAction->setEnabled(hasSelection);
			menu.addSeparator();
			menu.addAction("Delete Event(s)", this, SLOT(OnMenuDelete()), QKeySequence("Delete"))->setEnabled(hasSelection);
			menu.addSeparator();
			menu.addAction("Play / Pause", this, SLOT(OnMenuPlay()), QKeySequence("Space"));
			menu.addAction("Previous Frame", this, SLOT(OnMenuPreviousFrame()), QKeySequence(","));
			menu.addAction("Next Frame", this, SLOT(OnMenuNextFrame()), QKeySequence("."));
			menu.addAction("Jump to Previous Event", this, SLOT(OnMenuPreviousKey()), QKeySequence("X"));
			menu.addAction("Jump to Next Event", this, SLOT(OnMenuNextKey()), QKeySequence("C"));
			menu.exec(QCursor::pos(), duplicateAction);
		}
	}
}

void CTimeline::AddKeyToTrack(STimelineTrack &track, float time)
{
	track.modified = true;
	track.elements.push_back(track.defaultElement);
	track.elements.back().added = true;
	float length = track.defaultElement.end - track.defaultElement.start;
	track.elements.back().start = time;
	track.elements.back().end = length;
	track.elements.back().selected = true;
}

void CTimeline::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		QPoint posInLayout = m_viewState.LocalToLayout(ev->pos());

		const bool bInTreeArea = m_treeVisible && (ev->x() <= m_viewState.treeWidth);

		if (bInTreeArea)
		{
			const bool bOverSplitter = ev->x() >= (m_viewState.treeWidth - SPLITTER_WIDTH);

			if (!bOverSplitter)
			{
				m_mouseHandler.reset(new STreeMouseHandler(this));
				m_mouseHandler->mouseDoubleClickEvent(ev);
				update();
			}
		}

		QPoint layoutPoint = m_viewState.LocalToLayout(ev->pos());
		STrackLayout* track = HitTestTrack(m_layout->tracks, layoutPoint);

		if (track)
		{
			SElementLayoutPtrs hitElements;
			const bool bHit = HitTestElements(m_layout->tracks, QRect(layoutPoint - QPoint(2, 2), layoutPoint + QPoint(2, 2)), hitElements);

			if (!bHit)
			{
				float time = m_viewState.LayoutToTime(layoutPoint.x());
				STimelineTrack& timelineTrack = *track->pTimelineTrack;

				if ((timelineTrack.caps & STimelineTrack::CAP_COMPOUND_TRACK) == 0)
				{
					AddKeyToTrack(timelineTrack, time);
				}
				else
				{
					const size_t numSubTracks = timelineTrack.tracks.size();
					for (size_t i = 0; i < numSubTracks; ++i)
					{
						STimelineTrack& subTrack = *timelineTrack.tracks[i];
						AddKeyToTrack(subTrack, time);
					}
				}
				
				ContentChanged(false);
				m_mouseHandler.reset();
				mouseMoveEvent(ev);
			}
		}
	}
}

void CTimeline::UpdateCursor(QMouseEvent* ev)
{
	QPoint posInLayoutSpace = m_viewState.LocalToLayout(ev->pos());

	SElementLayoutPtrs hitElements;
	HitTestElements(m_layout->tracks, QRect(posInLayoutSpace - QPoint(2, 2), posInLayoutSpace + QPoint(2, 2)), hitElements);
	const bool bOverSelected = !hitElements.empty() && hitElements.back()->elementRef.GetElement().selected;
	const bool bInTreeArea = m_treeVisible && (ev->x() <= m_viewState.treeWidth);

	bool shift = ev->modifiers().testFlag(Qt::ShiftModifier);
	bool control = ev->modifiers().testFlag(Qt::ControlModifier);

	if (m_mouseHandler)
	{
		m_mouseHandler->mouseMoveEvent(ev);
		update();
	}
	else if (m_treeVisible && (ev->x() <= m_viewState.treeWidth) && (ev->x() >= (m_viewState.treeWidth - SPLITTER_WIDTH)))
	{
		setCursor(QCursor(Qt::SplitHCursor));
	}
	else if (!bInTreeArea && bOverSelected && !(shift || control))
	{
		setCursor(QCursor(Qt::SizeHorCursor));
	}
	else
	{
		setCursor(QCursor());
	}
}

void CTimeline::mouseMoveEvent(QMouseEvent* ev)
{
	UpdateCursor(ev);
}

void CTimeline::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton || ev->button() == Qt::MiddleButton)
	{
		if (m_mouseHandler.get())
		{
			m_mouseHandler->mouseReleaseEvent(ev);
			m_mouseHandler.reset();
			update();
		}
	}
	UpdateCursor(ev);
}

void CTimeline::focusOutEvent(QFocusEvent* ev)
{
	if (m_mouseHandler.get())
	{
		m_mouseHandler->focusOutEvent(ev);
		m_mouseHandler.reset();		
	}

	update();
}

void CTimeline::wheelEvent(QWheelEvent* ev)
{
	int pixelDelta = ev->pixelDelta().manhattanLength();

	if (pixelDelta == 0)
	{
		pixelDelta = ev->delta();
	}

	const float fractionOfView = std::min(m_viewState.widthPixels != 0 ? float(pixelDelta) / m_viewState.widthPixels : 0.0f, 0.5f);

	m_viewState.visibleDistance -= m_viewState.visibleDistance * fractionOfView;
	
	const float totalDuration = m_layout->maxEndTime - m_layout->minStartTime;
	const float padding = (float(TIMELINE_PADDING) - 0.5f) / m_viewState.widthPixels * totalDuration;	
	m_viewState.visibleDistance = clamp_tpl(m_viewState.visibleDistance, 0.01f, totalDuration + 2.0f * padding);

	UpdateLayout();
	update();
}

QSize CTimeline::sizeHint() const 
{
	return QSize(m_layout->size);
}

void CTimeline::resizeEvent(QResizeEvent* ev)
{
	UpdateLayout();
}

float CTimeline::ClampAndSnapTime(float time, bool snapToFrames) const
{
	float minTime = m_layout->minStartTime;
	float maxTime = m_layout->maxEndTime;
	float unclampedTime = time;

	if (m_cycled)
	{
		while (unclampedTime < minTime)
		{
			unclampedTime += (maxTime - minTime);
		}

		unclampedTime = fmodf(unclampedTime - minTime, maxTime - minTime) + minTime;
	}

	float clampedTime = clamp_tpl(unclampedTime, minTime, maxTime);

	if (m_framesToSnap == 0 || !snapToFrames)
	{
		return clampedTime;
	}
	else
	{
		int frameIndex = int(clampedTime * m_framesToSnap + 0.5f);
		return float(frameIndex) / m_framesToSnap;
	}
}

void CTimeline::ClampAndSetTime(float time, bool scrubThrough)
{
	float newTime = ClampAndSnapTime(time, m_snapToFrames);

	if (newTime != m_time)
	{
		m_time = newTime;
		UpdateLayout();
		update();
		SignalScrub(scrubThrough);
	}
}

void CTimeline::SetTimeUnitScale(float scale)
{
	m_timeUnitScale = scale;
	update();
}

void CTimeline::SetTime(float time)
{
	m_time = time;
	update();
}

void CTimeline::SetCycled(bool cycled)
{
	m_cycled = cycled;
}

void CTimeline::SetContent(STimelineContent* pContent)
{
	m_pContent = pContent;

	UpdateLayout();
	update();
}

void CTimeline::UpdateLayout()
{
	m_layout->tracks.clear();

	m_viewState.widthPixels = width();

	if (m_treeVisible)
	{
		m_viewState.widthPixels -= m_viewState.treeWidth;
		m_viewState.widthPixels = std::max(m_viewState.widthPixels, 0);
	}

	ClampViewOrigin(&m_viewState, *m_layout);

	if (m_pContent)
	{
		CalculateLayout(m_layout.get(), *m_pContent, m_viewState, m_pFilterLineEdit, m_time, m_keyWidth, m_treeVisible);
	}

	if (m_sizeToContent)
	{
		setMaximumHeight(m_layout->size.height());
		setMinimumHeight(m_layout->size.height());
	}
	else
	{
		setMinimumHeight(RULER_HEIGHT + 1);
		setMaximumHeight(QWIDGETSIZE_MAX);
	}

	if (m_treeVisible)
	{
		if (!m_pFilterLineEdit)
		{
			m_pFilterLineEdit = new QLineEdit(this);
			connect(m_pFilterLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnFilterChanged()));
		}

		m_pFilterLineEdit->resize(m_viewState.treeWidth - SPLITTER_WIDTH, RULER_HEIGHT + VERTICAL_PADDING);
	}
	else if (!m_treeVisible && m_pFilterLineEdit)
	{
		SAFE_DELETE(m_pFilterLineEdit);
	}
}

void CTimeline::SetSizeToContent(bool sizeToContent)
{
	m_sizeToContent = sizeToContent;

	UpdateLayout();
}

void CTimeline::ContentChanged(bool continuous)
{
	SignalContentChanged(continuous);

	DeletedMarkedElements(m_pContent->track);

	ForEachElement(m_pContent->track, [](STimelineTrack& track, STimelineElement& element) 
	{
		track.modified = false;
		element.added = false;
	});
		
	UpdateLayout();
	update();
}

void CTimeline::OnMenuSelectionToCursor()
{
	TSelectedElements elements = GetSelectedElements(m_pContent->track);

	for (size_t i = 0; i < elements.size(); ++i)
	{
		STimelineTrack& track = *elements[i].first;
		STimelineElement& element = *elements[i].second;
		float length = element.end - element.start;
		element.start = m_time;
		element.end = element.start + length;
		if (element.type == element.CLIP)
			if (length > track.endTime)
				element.start = track.endTime - length;
		if (element.start < track.startTime)
			element.start = track.startTime;
	}

	ContentChanged(false);
}

void CTimeline::OnMenuDuplicate()
{
	TSelectedElements selectedElements = GetSelectedElements(m_pContent->track);
	if (selectedElements.empty())
		return;

	typedef std::vector<std::pair<STimelineTrack*, STimelineElement>> TTrackElements;

	TTrackElements elements;

	ForEachElement(m_pContent->track, [&](STimelineTrack &track, STimelineElement& element) 
	{ 
		if (element.selected) 
		{
			elements.push_back(std::make_pair(&track, element));
			element.selected = false;
		}
	});

	for (size_t i = 0; i < elements.size(); ++i)
	{
		STimelineTrack* track = elements[i].first;
		const STimelineElement& element = elements[i].second;
		track->elements.push_back(element);
		STimelineElement& e = track->elements.back();
		e.userId = 0;
		e.added = true;
		e.sideLoadChanged = true;
		e.selected = true;
	}
			 
	ContentChanged(false);
}

void CTimeline::OnMenuCopy()
{
}

void CTimeline::OnMenuPaste()
{
}

void CTimeline::OnMenuDelete()
{
	ForEachElement(m_pContent->track, [](STimelineTrack &track, STimelineElement& element)
	{
		if (element.selected)
	{
			track.modified = true;
			element.deleted = true;
	}
	});

	ContentChanged(false);
}

void CTimeline::OnMenuPlay()
{
	SignalPlay();
}

typedef std::vector<std::pair<float, STimelineContentElementRef> > TimeToId;
static void GetAllTimes(TimeToId* times, const STimelineTrack& track)
{
	for (size_t i = 0; i < track.elements.size(); ++i)
	{
		const STimelineElement& element = track.elements[i];

	}

	for (size_t i = 0; i < track.tracks.size(); ++i)
	{
		GetAllTimes(times, *track.tracks[i]);
	}
}

static void GetAllTimes(TimeToId* times, STimelineContent& content)
{
	ForEachTrack(content.track, [&](STimelineTrack &track) 
  {
		times->push_back(std::make_pair(track.startTime, STimelineContentElementRef()));
		times->push_back(std::make_pair(track.endTime, STimelineContentElementRef()));
  });

	ForEachElementWithIndex(content.track, [=](STimelineTrack& track, STimelineElement& element, size_t i) 
	{
		STimelineContentElementRef ref(&track, i);
		times->push_back(std::make_pair(element.start, ref));
		if (element.type == STimelineElement::CLIP)
			times->push_back(std::make_pair(element.end, ref));
	});

	std::sort(times->begin(), times->end());
}

static STimelineContentElementRef SelectedIdAtTime(const std::vector<STimelineContentElementRef>& selection, const STimelineContent& content, float time)
{
	for (size_t i = 0; i < selection.size(); ++i)
	{
		const STimelineContentElementRef& id = selection[i];
		const STimelineElement& element = id.GetElement();
		if (element.start == time || element.end == time)
			return id;
	}
	return STimelineContentElementRef();
}

void CTimeline::OnMenuPreviousKey()
{
	if (!m_pContent)
		return;
	TimeToId times;
	GetAllTimes(&times, *m_pContent);

	std::vector<STimelineContentElementRef> selection;
	ForEachElementWithIndex(m_pContent->track, [&](STimelineTrack &t, STimelineElement& e, size_t i) { if (e.selected) selection.push_back(STimelineContentElementRef(&t, i)); });

	STimelineContentElementRef selectedId = SelectedIdAtTime(selection, *m_pContent, m_time);

	TimeToId::iterator it = std::lower_bound(times.begin(), times.end(), std::make_pair(m_time, selectedId));
	if (it != times.end())
	{
		if (it != times.begin())
			--it;

		ClearElementSelection(m_pContent->track);
		if (it->second.IsValid())
			it->second.GetElement().selected = true;	

		m_time = it->first;

		SignalSelectionChanged(false);
		SignalScrub(false);
		update();
	}
}

void CTimeline::OnMenuNextKey()
{
	if (!m_pContent)
		return;
	TimeToId times;
	GetAllTimes(&times, *m_pContent);

	std::vector<STimelineContentElementRef> selection;
	ForEachElementWithIndex(m_pContent->track, [&](STimelineTrack &t, STimelineElement& e, size_t i) { if (e.selected) selection.push_back(STimelineContentElementRef(&t, i)); });

	STimelineContentElementRef selectedId = SelectedIdAtTime(selection, *m_pContent, m_time);

	TimeToId::iterator it = std::upper_bound(times.begin(), times.end(), std::make_pair(m_time, selectedId));
	if (it != times.end())
	{
		ClearElementSelection(m_pContent->track);
		if (it->second.IsValid())
			it->second.GetElement().selected = true;

		m_time = it->first;

		SignalSelectionChanged(false);
		SignalScrub(false);
		update();
	}
}

void CTimeline::OnMenuPreviousFrame()
{
	m_time = ClampAndSnapTime(float(int(m_time * m_framesToSnap - 0.5f)) / m_framesToSnap, true);
	SignalScrub(false);
	update();
}

void CTimeline::OnMenuNextFrame()
{
	m_time = ClampAndSnapTime(float(int(m_time * m_framesToSnap + 1.5f)) / m_framesToSnap, true);
	SignalScrub(false);
	update();
}

void CTimeline::OnFilterChanged()
{
	UpdateLayout();
	update();
}

void CTimeline::SetTreeVisible(bool visible)
{
	m_treeVisible = visible;
	m_viewState.treeWidth = visible ? DEFAULT_TREE_WIDTH : 0;

	UpdateLayout();
	update();
}

STrackLayout* CTimeline::GetTrackLayoutFromPos(const QPoint& pos) const
{
	if (pos.y() < RULER_HEIGHT)
	{
		return nullptr;
	}

	STrackLayouts& tracks = m_layout->tracks;

	auto findIter = std::upper_bound(tracks.begin(), tracks.end(), pos.y(), [&](int y, const STrackLayout& track)
	{
		return y < track.rect.bottom();
	});

	if (findIter != tracks.end() && pos.y() <= findIter->rect.bottom())
	{
		return &(*findIter);
	}

	return nullptr;
}

#include <moc_Timeline.cpp>
