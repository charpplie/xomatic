// (c) 2001-2012 Crytek GmbH
#pragma once
#include <QWidget>

#include <Cry_Math.h>
#include <Cry_Matrix34.h>
#include "EditorCommonAPI.h"
#include "QViewportEvents.h"

struct DisplayContext;
class CCamera;
struct SRenderingPassInfo;
struct SRendParams;
struct Ray;
struct IRenderer;
struct I3DEngine;
struct SSystemGlobalEnvironment;
namespace Serialization { class IArchive; }
using Serialization::IArchive;
using std::unique_ptr;

struct SKeyEvent;
struct SMouseEvent;
struct SViewportSettings;
struct SViewportState;

class QViewport;
struct SRenderContext
{
	CCamera* camera;
	QViewport* viewport;
	SRendParams* renderParams;
	SRenderingPassInfo* passInfo;
};

class QViewportConsumer;
class EDITOR_COMMON_API QViewport : public QWidget
{
	Q_OBJECT
public:
	QViewport(SSystemGlobalEnvironment* env, QWidget* parent);
	~QViewport();

	void AddConsumer(QViewportConsumer* consumer);
	void RemoveConsumer(QViewportConsumer* consumer);

	void CaptureMouse();
	void ReleaseMouse();
	void SetForegroundUpdateMode(bool foregroundUpdate);
	CCamera* Camera() const { return m_camera.get(); }
	void ResetCamera();
	void Serialize(IArchive& ar);

	void SetUseArrowsForNavigation(bool useArrowsForNavigation);
	void SetSceneDimensions(const Vec3& size) { m_sceneDimensions = size; }
	void SetSettings(const SViewportSettings& settings);
	const SViewportSettings& GetSettings() const {return *m_settings;}
	void SetState(const SViewportState& state);
	const SViewportState& GetState() const {return *m_state;}
	bool ScreenToWorldRay(Ray* ray, int x, int y);
	QPoint ProjectToScreen(const Vec3& point);

	int Width() const;
	int Height() const;

public slots:
	void Update();
signals:
	void SignalPreRender(const SRenderContext&);
	void SignalRender(const SRenderContext&);
	void SignalKey(const SKeyEvent&);
	void SignalMouse(const SMouseEvent&);
	void SignalUpdate();
	void SignalCameraMoved(const QuatT& qt);
protected:
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void wheelEvent(QWheelEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void keyPressEvent(QKeyEvent* ev) override;
	void keyReleaseEvent(QKeyEvent* ev) override;
	void resizeEvent(QResizeEvent* ev) override;
	void moveEvent(QMoveEvent* ev) override;
	void paintEvent(QPaintEvent* ev) override;
	bool event(QEvent* ev) override;
	bool winEvent(MSG * message, long * result);
private:
	void CameraMoved(const QuatT& m);
	bool CreateRenderContext();
	void DestroyRenderContext();
	void SetCurrentContext();
	void RestorePreviousContext();
	void UpdateBackgroundColor();

	void ProcessMouse();
	void ProcessKeys();
	void PreRender();
	void Render();
	void RenderInternal();
	void OnMouseEvent(const SMouseEvent& ev);
	void OnKeyEvent(const SKeyEvent& ev);
	float CalculateMoveSpeed(bool shiftPressed, bool ctrlPressed) const;

	std::auto_ptr<CCamera> m_camera;
	std::auto_ptr<CCamera> m_previousRenderCamera;
	std::auto_ptr<CCamera> m_previousSystemCamera;

	QTimer* m_timer;
	int m_width;
	int m_height;
	QPoint m_mousePressPos;
	int64 m_lastTime;
	float m_lastFrameTime;
	float m_averageFrameTime;
	bool m_useArrowsForNavigation;
	bool m_renderContextCreated;
	bool m_creatingRenderContext;
	bool m_updating;
	bool m_rotationMode;
	bool m_panMode;
	bool m_fastMode;
	bool m_slowMode;

	Vec3 m_cameraSmoothPosRate;
	float m_cameraSmoothRotRate;
	int m_mouseMovementsSinceLastFrame;
	SMouseEvent m_pendingMouseMoveEvent;

	Vec3 m_sceneDimensions;
	std::unique_ptr<SViewportSettings> m_settings;
	std::unique_ptr<SViewportState> m_state;
	std::vector<QViewportConsumer*> m_consumers;
	SSystemGlobalEnvironment* m_env;
};
