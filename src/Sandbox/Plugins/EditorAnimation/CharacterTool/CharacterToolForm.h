#pragma once

#include <QMainWindow>
#include <vector>
#include "Strings.h"
#include "Pointers.h"

class QViewport;
class QMainWindow;
class QSplitter;
class QToolButton;
class QTreeView;
class QPropertyTree;
class QBoxLayout;
class QToolBar;
class QDockWidget;
class QResizeEvent;
struct SRenderContext;
struct SViewportState;

namespace Serialization
{
	class IArchive;
}

namespace CharacterTool {
using std::vector;
using std::unique_ptr;

struct ViewportPlaybackHotkeyConsumer;
struct DisplayAnimationOptions;
class AnimEventPresetPanel;
class BlendSpacePreview;
class DisplayParametersPanel;
class DockWidgetManager;
class ExplorerActionHandler;
class ExplorerPanel;
class PlaybackPanel;
class PropertiesPanel;
class SceneParametersPanel;
class TransformPanel;
class QSplitViewport;
struct IViewportMode;
struct DisplayParameters;
struct ExplorerAction;
struct ExplorerEntry;
struct ViewportOptions;
struct System;

class CharacterToolForm : public QMainWindow
{
	Q_OBJECT
public:
	CharacterToolForm(QWidget* parent = 0);
	~CharacterToolForm();

	void Serialize(Serialization::IArchive& ar);
	void ExecuteExplorerAction(const ExplorerAction& action, const vector<_smart_ptr<ExplorerEntry>>& entries);
public slots:

	void OnFileSaveAll();
	void OnFileRecent();
	void OnFileNewCharacter();
	void OnFileOpenCharacter();
	void OnFileCleanAnimations();
	void OnFileResaveAnimSettings();
	void OnLayoutReset();
	void OnLayoutSave();
	void OnLayoutSet();
	void OnLayoutRemove();

	void OnIdleUpdate();
	void OnPreRenderCompressed(const SRenderContext& context);
	void OnRenderCompressed(const SRenderContext& context);
	void OnPreRenderOriginal(const SRenderContext& context);
	void OnRenderOriginal(const SRenderContext& context);
	void OnViewportUpdate();
	void OnViewportOptionsChanged();
	void OnDisplayOptionsChanged(const DisplayParameters& displayOptions);
	void OnDisplayParametersButton();
	void OnExplorerSelectionChanged();
	void OnDockWidgetsChanged();
	void OnCharacterLoaded();

	void OnAnimEventPresetPanelPutEvent();

	bool TryToClose() { return true; }

	IViewportMode* ViewportMode() const{ return m_mode; }
	PlaybackPanel* GetPlaybackPanel() { return m_playbackPanel; }
protected:
	bool event(QEvent* ev) override;
	void closeEvent(QCloseEvent* ev);
	void resizeEvent(QResizeEvent* ev) override;
	bool eventFilter(QObject* sender, QEvent* ev) override;
private:
	void SaveState(const char* filename, bool layoutOnly);
	void LoadState(const char* filename, bool layoutOnly);
	void LoadLayout(const char* name);
	void SaveLayout(const char* name);
	void RemoveLayout(const char* name);
	void ResetLayout();
	void Initialize();
	void SplitExplorer(int explorerIndex);
	void UpdateRecentMenu();
	void UpdateLayoutMenu();
	void UpdatePanesMenu();
	void UpdateViewportMode(ExplorerEntry* newEntry);
	void InstallMode(IViewportMode* mode, ExplorerEntry* modeEntry);
	std::vector<string> FindLayoutNames();
	void CreateDefaultDockWidgets();
	void ReadViewportOptions(const ViewportOptions& options, const DisplayAnimationOptions& animationOptions);
	void UpdatePropertyToolBar();
	QString MakeUniqueExplorerName() const;
	struct SPrivate;
	System* m_system;
	QScopedPointer<SPrivate> m_private;
	QSplitViewport* m_splitViewport;
	int m_displayParametersSplitterWidths[2];
	QSplitter* m_displayParametersSplitter;
	IViewportMode* m_mode;
	QScopedPointer<IViewportMode> m_modeCharacter;
	ExplorerEntry* m_modeEntry;

	std::vector<QDockWidget*> m_dockWidgets;
	PlaybackPanel* m_playbackPanel;
	BlendSpacePreview* m_blendSpacePreview;
	SceneParametersPanel* m_sceneParametersPanel;
	DisplayParametersPanel* m_displayParametersPanel;
	AnimEventPresetPanel* m_animEventPresetPanel;
	QTreeView* m_characterTree;
	QToolBar* m_modeToolBar;
	QToolButton* m_displayParametersButton;
	TransformPanel* m_transformPanel;

	QMenu* m_menuFileRecent;
	QMenu* m_menuView;
	vector<string> m_recentCharacters;
	unique_ptr<ViewportPlaybackHotkeyConsumer> m_viewportPlaybackHotkeyConsumer;
	unique_ptr<DockWidgetManager> m_dockWidgetManager;
	unique_ptr<QPropertyTree> m_contentLayerPropertyTree;

	QAction* m_actionViewBindPose;
	QAction* m_actionViewShowOriginalAnimation;
	QAction* m_actionViewShowCompressionFlickerDiff;

	QMenu* m_menuLayout;
	QAction* m_actionLayoutReset;
	QAction* m_actionLayoutLoadState;
	QAction* m_actionLayoutSaveState;

	bool m_stateLoaded;
};

void ShowCharacterToolForm();

}
