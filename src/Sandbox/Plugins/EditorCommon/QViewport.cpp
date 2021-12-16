// (c) 2001-2012 Crytek GmbH
#include "StdAfx.h"

#include <Cry_Camera.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <ITimer.h>
#include <I3DEngine.h>
#include <IPhysicsDebugRenderer.h>
#include <IEditor.h>

#include <QMouseEvent>
#include <QTimer>

#include "QViewport.h"
#include "QViewportEvents.h"
#include "QViewportConsumer.h"
#include "QViewportSettings.h"
#include "Serialization.h"
#include <QApplication>

#pragma warning(disable: 4355) // 'this' : used in base member initializer list)

static void DrawGridLine(IRenderAuxGeom& aux, ColorB col, const float alpha, const float alphaFalloff, const float slide, const float halfSlide, const float maxSlide, const Vec3& stepDir, const Vec3& orthoDir, const SViewportState& state, const SViewportGridSettings& gridSettings)
{
	ColorB colEnd = col;

	float weight = 1.0f - (slide / halfSlide);
	if(slide > halfSlide)
		weight = (slide-halfSlide) / halfSlide;

	float orthoWeight = 1.0f;

	if (gridSettings.circular)
	{
		float invWeight = 1.0f - weight;
		orthoWeight = sqrtf((invWeight*2) - (invWeight*invWeight));
	}
	else
		orthoWeight = 1.0f;

	col.a = (1.0f - (weight * (1.0f-alphaFalloff))) * alpha;
	colEnd.a = alphaFalloff * alpha;

	Vec3 orthoStep = state.gridOrigin.q * (orthoDir*halfSlide*orthoWeight);

	Vec3 point = state.gridOrigin * (-(stepDir*halfSlide) + (stepDir*slide));
	Vec3 points[3] = {
		point,
		point - orthoStep,
		point + orthoStep
	};

	aux.DrawLine(points[0], col, points[1], colEnd);
	aux.DrawLine(points[0], col, points[2], colEnd);
}

static void DrawGridLines(IRenderAuxGeom& aux, const uint count, const uint interStepCount, const Vec3& stepDir, const float stepSize, const Vec3& orthoDir, const float offset, const SViewportState& state, const SViewportGridSettings& gridSettings)
{	
	const uint countHalf = count / 2;
	Vec3 step = stepDir * stepSize;
	Vec3 orthoStep = orthoDir*countHalf;
	Vec3 maxStep = step*countHalf;// + stepDir*fabs(offset);
	const float maxStepLen = count*stepSize;
	const float halfStepLen = countHalf*stepSize;
	
	float interStepSize = interStepCount > 0 ? (stepSize / interStepCount) : stepSize;
	const float alphaMulMain = (float)gridSettings.mainColor.a;
	const float alphaMulInter = (float)gridSettings.middleColor.a;
	const float alphaFalloff = 1.0f - (gridSettings.alphaFalloff / 100.0f);
	float orthoWeight = 1.0f;
	
	for (int i=0; i<count+2; i++)
	{
		float pointSlide = i*stepSize + offset;
		if (pointSlide > 0.0f && pointSlide < maxStepLen)
			DrawGridLine(aux, gridSettings.mainColor, alphaMulMain, alphaFalloff, pointSlide, halfStepLen, maxStepLen, stepDir, orthoDir, state, gridSettings);

		for (int d=1; d<interStepCount; d++)
		{
			float interSlide = ((i-1)*stepSize) + offset + (d*interStepSize);
			if (interSlide > 0.0f && interSlide < maxStepLen )
				DrawGridLine(aux, gridSettings.middleColor, alphaMulInter, alphaFalloff, interSlide, halfStepLen, maxStepLen, stepDir, orthoDir, state, gridSettings);
		}
	}
}

static void DrawGrid(IRenderAuxGeom& aux, const SViewportState& state, const SViewportGridSettings& gridSettings)
{
	const uint count = gridSettings.count * 2;
	const float gridSize = gridSettings.spacing * gridSettings.count * 2.0f;
	const float halfGridSize = gridSettings.spacing * gridSettings.count;
	
	const float stepSize = gridSize/count;
	DrawGridLines(aux, count, gridSettings.interCount, Vec3(1.0f, 0.0f, 0.0f), stepSize, Vec3(0.0f, 1.0f, 0.0f), state.gridCellOffset.x, state, gridSettings);
	DrawGridLines(aux, count, gridSettings.interCount, Vec3(0.0f, 1.0f, 0.0f), stepSize, Vec3(1.0f, 0.0f, 0.0f), state.gridCellOffset.y, state, gridSettings);
}

static void DrawOrigin(IRenderAuxGeom& aux, const ColorB& col)
{
	const float scale = 0.3f;
	const float lineWidth = 4.0f;
	aux.DrawLine(Vec3(-scale,0,0), col, Vec3(scale,0,0), col, lineWidth);
	aux.DrawLine(Vec3(0,-scale,0), col, Vec3(0,scale,0), col, lineWidth);
	aux.DrawLine(Vec3(0,0,-scale), col, Vec3(0,0,scale), col, lineWidth);
}

static void DrawOrigin(IRenderAuxGeom& aux, const int left, const int top, const float scale, const Matrix34 cameraTM)
{
	Vec3 originPos = Vec3(left,top,0);
	Quat originRot = Quat(0.707107f, 0.707107f, 0, 0) * Quat(cameraTM).GetInverted();
	Vec3 x = originPos + originRot * Vec3(1,0,0) * scale;
	Vec3 y = originPos + originRot * Vec3(0,1,0) * scale;
	Vec3 z = originPos + originRot * Vec3(0,0,1) * scale;
	ColorF xCol(1,0,0);
	ColorF yCol(0,1,0);
	ColorF zCol(0,0,1);
	const float lineWidth = 2.0f;

	aux.DrawLine(originPos, xCol, x, xCol, lineWidth);
	aux.DrawLine(originPos, yCol, y, yCol, lineWidth);
	aux.DrawLine(originPos, zCol, z, zCol, lineWidth);
}

QViewport::QViewport(SSystemGlobalEnvironment* env, QWidget* parent)
: QWidget(parent)
, m_renderContextCreated(false)
, m_updating(false)
, m_width(0)
, m_height(0)
, m_rotationMode(false)
, m_panMode(false)
, m_fastMode(false)
, m_slowMode(false)
, m_lastTime(0)
, m_lastFrameTime(0.0f)
, m_averageFrameTime(0.0f)
, m_sceneDimensions(1.0f, 1.0f, 1.0f)
, m_creatingRenderContext(false)
, m_timer(0)
, m_env(env)
, m_cameraSmoothPosRate(0)
, m_cameraSmoothRotRate(0)
, m_settings(new SViewportSettings())
, m_state(new SViewportState())
, m_useArrowsForNavigation(true)
, m_previousSystemCamera(new CCamera)
, m_previousRenderCamera(new CCamera)
, m_mouseMovementsSinceLastFrame(0)
{
	if (!gEnv)
		gEnv = m_env; // Shhh!

	CreateRenderContext();

	m_camera.reset(new CCamera());
	ResetCamera();

	m_mousePressPos = QCursor::pos();

	UpdateBackgroundColor();

	m_timer = new QTimer(this);
	m_timer->start(5);

	setUpdatesEnabled(false);
	setAttribute(Qt::WA_PaintOnScreen);
	setMouseTracking(true);
}

QViewport::~QViewport()
{

}

void QViewport::UpdateBackgroundColor()
{
	QPalette pal(palette());
	pal.setColor(QPalette::Background, QColor(m_settings->background.topColor.r, 
																						m_settings->background.topColor.g, 
																						m_settings->background.topColor.b, 
																						m_settings->background.topColor.a));
	setPalette(pal);
	setAutoFillBackground(true);
}

bool QViewport::ScreenToWorldRay(Ray* ray, int x, int y)
{
	if (!m_env->pRenderer)
		return false;

	SetCurrentContext();

	Vec3 pos0, pos1;
	float wx, wy, wz;
	if (!m_env->pRenderer->UnProjectFromScreen(float(x), float(m_height - y), 0, &wx, &wy, &wz))
		return false;
	pos0(wx, wy,wz);
	if (!m_env->pRenderer->UnProjectFromScreen(float(x), float(m_height - y), 1, &wx, &wy, &wz))
		return false;
	pos1(wx, wy, wz);

	RestorePreviousContext();

	Vec3 v = (pos1-pos0);
	v = v.GetNormalized();

	ray->origin = pos0;
	ray->direction = v;
	return true;
}

QPoint QViewport::ProjectToScreen(const Vec3& wp)
{
	float x,y,z;

	SetCurrentContext();
	m_env->pRenderer->ProjectToScreen( wp.x,wp.y,wp.z,&x,&y,&z );
	if (_finite(x) || _finite(y))
	{
		return QPoint(int((x / 100.0) * Width()), int((y / 100.0) * Height()));
	}
	RestorePreviousContext();

	return QPoint(0, 0);
}

int QViewport::Width() const
{
	return rect().width();
}

int QViewport::Height() const
{
	return rect().height();
}

bool QViewport::CreateRenderContext()
{
	if (m_creatingRenderContext)
		return false;
	m_creatingRenderContext = true;
	DestroyRenderContext();
	HWND window = (HWND)QWidget::winId();
	if (window && m_env->pRenderer && !m_renderContextCreated)
	{
		m_renderContextCreated = true;
		m_env->pRenderer->CreateContext(window);
		m_env->pRenderer->SetCurrentContext(window);
		m_creatingRenderContext = false;
		return true;
	}
	m_creatingRenderContext = false;
	return false;
}

void QViewport::DestroyRenderContext()
{
	if (m_env->pRenderer && m_renderContextCreated)
	{
		HWND window = (HWND)QWidget::winId();
		if (window != m_env->pRenderer->GetHWND())
			m_env->pRenderer->DeleteContext(window);
		m_renderContextCreated = false;
	}
}

void QViewport::SetCurrentContext()
{
	if(m_camera.get() == 0)
		return;
	HWND window = (HWND)QWidget::winId();
	m_env->pRenderer->SetCurrentContext( window );
	m_env->pRenderer->ChangeViewport(0, 0, m_width, m_height);
	*m_previousRenderCamera = m_env->pRenderer->GetCamera();
	m_env->pRenderer->SetCamera(*m_camera);
	*m_previousSystemCamera = m_env->pSystem->GetViewCamera();
	m_env->pSystem->SetViewCamera(*m_camera);
}

void QViewport::RestorePreviousContext()
{
	if (!m_camera.get())
		return;
	m_env->pRenderer->SetCamera(*m_previousRenderCamera);
	m_env->pSystem->SetViewCamera(*m_previousSystemCamera);
}

void QViewport::Serialize(IArchive& ar)
{
	if (!ar.IsEdit())
	{
		ar(m_state->cameraTarget, "cameraTarget", "Camera Target");
	}
}

struct AutoBool
{
	AutoBool(bool* value)
	: m_value(value)
	{
		*m_value = true;
	}

	~AutoBool()
	{
		*m_value = false;
	}

	bool* m_value;
};

void QViewport::Update()
{
	int64 time = m_env->pSystem->GetITimer()->GetAsyncTime().GetMilliSecondsAsInt64();
	if (m_lastTime == 0)
		m_lastTime = time;
	m_lastFrameTime = (time - m_lastTime) * 0.001f;
	m_lastTime = time;
	if (m_averageFrameTime == 0.0f)
		m_averageFrameTime = m_lastFrameTime;
	else
		m_averageFrameTime = 0.01f * m_lastFrameTime + 0.99f * m_averageFrameTime;

	if (m_env->pRenderer == 0 ||
			m_env->p3DEngine == 0)
		return;

	if (!isVisible())
		return;

	if (!m_renderContextCreated)
		return;

	if (m_updating)
		return;
	
	AutoBool updating(&m_updating);

	if (hasFocus())
	{
		ProcessMouse();
		ProcessKeys();
	}

	RenderInternal();
}

void QViewport::CaptureMouse()
{
	grabMouse();
}

void QViewport::ReleaseMouse()
{
	releaseMouse();
}

void QViewport::SetForegroundUpdateMode(bool foregroundUpdate)
{
	//m_timer->setInterval(foregroundUpdate ? 2 : 50);
}





void QViewport::ProcessMouse()
{
	QPoint point = mapFromGlobal(QCursor::pos());

	if (point == m_mousePressPos)
	{		
		return;
	}

	float speedScale = CalculateMoveSpeed(m_fastMode, m_slowMode);

	if ((m_rotationMode && m_panMode)/* || m_bInZoomMode*/)
	{
		// Zoom.
		QuatT qt = m_state->cameraTarget;
		Vec3 xdir(0,0,0);

		Vec3 ydir = qt.GetColumn1().GetNormalized();
		Vec3 pos = qt.t;
		pos = pos - 0.2f*ydir*(m_mousePressPos.y()-point.y())*speedScale;
		qt.t = pos;
		CameraMoved(qt);

		QCursor::setPos(mapToGlobal(m_mousePressPos));
	}
	else if (m_rotationMode)
	{
		Ang3 angles( -point.y()+m_mousePressPos.y(),0,-point.x()+m_mousePressPos.x() );
		angles = angles * 0.001f * m_settings->camera.rotationSpeed;

		QuatT qt = m_state->cameraTarget;
		Ang3 ypr = CCamera::CreateAnglesYPR( Matrix33(qt.q) );
		ypr.x += angles.z;
		ypr.y += angles.x;
		ypr.y = clamp_tpl(ypr.y,-1.5f,1.5f);

		qt.q = Quat(CCamera::CreateOrientationYPR(ypr));
		CameraMoved(qt);

		QCursor::setPos(mapToGlobal(m_mousePressPos));
	}
	else if (m_panMode)
	{
		// Slide.
		QuatT qt = m_state->cameraTarget;
		Vec3 xdir = qt.GetColumn0().GetNormalized();
		Vec3 zdir = qt.GetColumn2().GetNormalized();

		Vec3 pos = qt.t;
		pos += 0.0025f*xdir*(point.x()-m_mousePressPos.x())*speedScale + 0.0025f*zdir*(m_mousePressPos.y()-point.y())*speedScale;
		qt.t = pos;
		CameraMoved(qt);

		QCursor::setPos(mapToGlobal(m_mousePressPos));
	}
	/*
	else if (m_orbitMode)
	{
		Ang3 angles( -point.y+m_mousePos.y,0,-point.x+m_mousePos.x );
		angles = angles * 0.002f * GetCameraRotateSpeed();

		Ang3 ypr = CCamera::CreateAnglesYPR( Matrix33(GetViewTM()) );
		ypr.x += angles.z;
		ypr.y = CLAMP(ypr.y,-1.5f,1.5f);		// to keep rotation in reasonable range
		ypr.y += angles.x;

		Matrix33 rotateTM = CCamera::CreateOrientationYPR(ypr);
		Matrix34 camTM = GetViewTM();

		Vec3 src = GetViewTM().GetTranslation();
		Vec3 trg = m_orbitTarget;
		float fCameraRadius = (trg-src).GetLength();
		
		// Calc new source.
		src = trg - rotateTM * Vec3(0,1,0)* fCameraRadius;
		camTM = rotateTM;
		camTM.SetTranslation( src );

		CameraMoved(camTM);

		QCursor::setPos(mapToGlobal(m_mousePos));
	}
	*/
}

void QViewport::ProcessKeys()
{
	if (!m_renderContextCreated)
		return;

	float deltaTime = m_lastFrameTime;

	if (deltaTime > 0.1f)
		deltaTime = 0.1f;

	QuatT qt = m_state->cameraTarget;
	Vec3 ydir = qt.GetColumn1().GetNormalized();
	Vec3 xdir = qt.GetColumn0().GetNormalized();
	Vec3 pos = qt.t;

	float moveSpeed = CalculateMoveSpeed(m_fastMode, m_slowMode);
	bool hasPressedKey = false;

	if ((m_useArrowsForNavigation && CheckVirtualKey(VK_UP)) || CheckVirtualKey('W'))
	{
		hasPressedKey = true;
		qt.t = qt.t + deltaTime * moveSpeed * ydir;
		CameraMoved(qt);
	}

	if ((m_useArrowsForNavigation && CheckVirtualKey(VK_DOWN)) || CheckVirtualKey('S'))
	{
		hasPressedKey = true;
		qt.t = qt.t - deltaTime * moveSpeed * ydir;
		CameraMoved(qt);
	}

	if ((m_useArrowsForNavigation && CheckVirtualKey(VK_LEFT)) || CheckVirtualKey('A'))
	{
		hasPressedKey = true;
		qt.t = qt.t - deltaTime * moveSpeed * xdir;
		CameraMoved(qt);
	}

	if ((m_useArrowsForNavigation && CheckVirtualKey(VK_RIGHT)) || CheckVirtualKey('D'))
	{
		hasPressedKey = true;
		qt.t = qt.t + deltaTime * moveSpeed * xdir;
		CameraMoved(qt);
	}

	if (CheckVirtualKey(VK_RBUTTON) | CheckVirtualKey(VK_MBUTTON))
	{
		hasPressedKey = true;
	}
}

void QViewport::CameraMoved(const QuatT& qt)
{
	m_state->cameraTarget = qt;
	SignalCameraMoved(qt);
}

void QViewport::OnKeyEvent(const SKeyEvent& ev)
{
	for (size_t i = 0; i < m_consumers.size(); ++i)
		m_consumers[i]->OnViewportKey(ev);
	SignalKey(ev);
}

void QViewport::OnMouseEvent(const SMouseEvent& ev)
{
	if (ev.type == SMouseEvent::MOVE)
	{
		// Make sure we don't process more than one mouse event per frame, so we don't 
		// end up consuming all the "idle" time
		++m_mouseMovementsSinceLastFrame;

		if (m_mouseMovementsSinceLastFrame > 1)
		{
			// we can't discard all movement events, the last one should be delivered.
			m_pendingMouseMoveEvent = ev;
			return;
		}
	}

	for (size_t i = 0; i < m_consumers.size(); ++i)
		m_consumers[i]->OnViewportMouse(ev);
	SignalMouse(ev);	
}

void QViewport::PreRender()
{
	SRenderContext rc;
	rc.camera = m_camera.get();
	rc.viewport = this;
	
	SignalPreRender(rc);


	const float fov = DEG2RAD(m_settings->camera.fov);
	const float fTime = m_env->pTimer->GetFrameTime();
	float lastRotWeight = 0.0f;
	
	QuatT targetTM = m_state->cameraTarget;
	QuatT currentTM = m_state->lastCameraTarget;
	
	if ((targetTM.t-currentTM.t).len() > 0.0001f)
		SmoothCD(currentTM.t, m_cameraSmoothPosRate, fTime, targetTM.t, m_settings->camera.smoothPos);
	else
		m_cameraSmoothPosRate = Vec3(0);

	SmoothCD(lastRotWeight, m_cameraSmoothRotRate, fTime, 1.0f, m_settings->camera.smoothRot);

	if (lastRotWeight >= 1.0f)
		m_cameraSmoothRotRate = 0.0f;

	currentTM = QuatT(Quat::CreateNlerp(currentTM.q, targetTM.q, lastRotWeight), currentTM.t);

	m_state->lastCameraParentFrame = m_state->cameraParentFrame;
	m_state->lastCameraTarget = currentTM;

	m_camera->SetFrustum(m_width, m_height, fov, m_settings->camera.nearClip, m_env->p3DEngine->GetMaxViewDistance());
	m_camera->SetMatrix(Matrix34(m_state->cameraParentFrame * currentTM));
}

void QViewport::Render()
{
	ColorF viewportBackgroundColor(m_settings->background.topColor.r / 255.0f, m_settings->background.topColor.g / 255.0f, m_settings->background.topColor.b / 255.0f);
	m_env->pRenderer->ClearBuffer(FRT_CLEAR, &viewportBackgroundColor);
	m_env->pRenderer->ResetToDefault();
	m_env->pRenderer->SetCamera(*m_camera);


	IRenderAuxGeom* aux = m_env->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags oldFlags = aux->GetRenderFlags();

	if (m_settings->background.useGradient)
	{	
		const float z = 1.0f; 
		aux->SetRenderFlags(e_Mode3D|e_AlphaNone|e_FillModeSolid|e_CullModeNone|e_DepthWriteOff|e_DepthTestOn);
		m_env->pRenderer->Set2DMode(true, m_width, m_height, 0.0f, z);
		ColorB topColor = m_settings->background.topColor;
		ColorB bottomColor = m_settings->background.bottomColor;
		aux->DrawTriangle(Vec3(0, 0, z), topColor, Vec3(m_width, 0, z), topColor, Vec3(m_width, m_height, z), bottomColor);
		aux->DrawTriangle(Vec3(m_width, m_height, z), bottomColor, Vec3(0, m_height, z), bottomColor, Vec3(0, 0, z), topColor);
		m_env->pRenderer->Set2DMode(false, m_width, m_height);
		aux->Flush();
	}
	
	// wireframe mode
	CScopedWireFrameMode scopedWireFrame(m_env->pRenderer, m_settings->debug.wireframe ? R_WIREFRAME_MODE : R_SOLID_MODE);

	SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(*m_camera, SRenderingPassInfo::DEFAULT_FLAGS, true);
	m_env->pRenderer->BeginSpawningGeneratingRendItemJobs(passInfo.ThreadID());
	m_env->pRenderer->BeginSpawningShadowGeneratingRendItemJobs(passInfo.ThreadID());		
	m_env->pRenderer->EF_ClearSkinningDataPool();
	m_env->pRenderer->EF_StartEf(passInfo);
	
	SRendParams rp;
	rp.AmbientColor.r = m_settings->lighting.ambientColor.r / 255.0f * m_settings->lighting.brightness;
	rp.AmbientColor.g = m_settings->lighting.ambientColor.g / 255.0f * m_settings->lighting.brightness;
	rp.AmbientColor.b = m_settings->lighting.ambientColor.b / 255.0f * m_settings->lighting.brightness;

	Matrix34 tm(IDENTITY);
	rp.pMatrix = &tm;
	rp.pPrevMatrix = &tm;

	rp.nDLightMask = 7;
	rp.dwFObjFlags  = 0;
	rp.dwFObjFlags |= FOB_TRANS_MASK;

	SRenderContext rc;
	rc.camera = m_camera.get();
	rc.viewport = this;
	rc.passInfo = &passInfo;
	rc.renderParams = &rp;


	for (size_t i = 0; i < m_consumers.size(); ++i)
		m_consumers[i]->OnViewportRender(rc);
	SignalRender(rc);

	m_env->pSystem->GetIPhysicsDebugRenderer()->Flush(m_lastFrameTime);
	m_env->pRenderer->EF_EndEf3D(SHDF_STREAM_SYNC, -1, -1, passInfo);
	
	
	if (m_settings->grid.showGrid)
	{
		aux->SetRenderFlags(e_Mode3D|e_AlphaBlended|e_FillModeSolid|e_CullModeNone|e_DepthWriteOff|e_DepthTestOn);
		DrawGrid(*aux, *m_state, m_settings->grid);
	}

	if (m_settings->debug.origin)
	{
		aux->SetRenderFlags(e_Mode3D|e_AlphaBlended|e_FillModeSolid|e_CullModeNone|e_DepthWriteOff|e_DepthTestOn);
		DrawOrigin(*aux, m_settings->debug.originColor);
	}

	if (m_settings->camera.showViewportOrientation)
	{
		aux->SetRenderFlags(e_Mode3D|e_AlphaBlended|e_FillModeSolid|e_CullModeNone|e_DepthWriteOn|e_DepthTestOn);
		m_env->pRenderer->Set2DMode(true, m_width, m_height);
		DrawOrigin(*aux, 50, m_height-50, 20.0f, m_camera->GetMatrix());
		m_env->pRenderer->Set2DMode(false, m_width, m_height);
	}

	aux->Flush();
	aux->SetRenderFlags(oldFlags);
	
	float col[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	if ((m_settings->debug.fps == true) && (m_averageFrameTime != 0.0f))
		m_env->pRenderer->Draw2dLabel(12.0f, 12.0f, 1.25f, col, false, "FPS: %.2f", 1.0f / m_averageFrameTime);

	if (m_mouseMovementsSinceLastFrame > 0)
	{
		m_mouseMovementsSinceLastFrame = 0;

		// Make sure we deliver at least last mouse movement event
		OnMouseEvent(m_pendingMouseMoveEvent);
	}
}

void QViewport::RenderInternal()
{
	CCamera previousCamera = m_env->pSystem->GetViewCamera();

	m_env->pSystem->RenderBegin();

	PreRender();
	SetCurrentContext();
	Render();

	bool renderStats = false;
	m_env->pSystem->RenderEnd(renderStats);

	RestorePreviousContext();
}

void QViewport::ResetCamera()
{
	*m_state = SViewportState();
	m_camera->SetMatrix(Matrix34(m_state->cameraTarget));
}

void QViewport::SetSettings(const SViewportSettings& settings)
{
	*m_settings = settings;
}

void QViewport::SetState(const SViewportState& state)
{
	*m_state = state;
}

float QViewport::CalculateMoveSpeed(bool shiftPressed, bool ctrlPressed) const
{
	float maxDimension = max(0.1f, max(m_sceneDimensions.x, max(m_sceneDimensions.y, m_sceneDimensions.z)));
	float moveSpeed = max(0.01f, m_settings->camera.moveSpeed) * maxDimension;

	if (shiftPressed)
		moveSpeed *= m_settings->camera.fastMoveMultiplier;
	if (ctrlPressed)
		moveSpeed *= m_settings->camera.slowMoveMultiplier;

	return moveSpeed;
}

void QViewport::mousePressEvent(QMouseEvent* ev)
{
	SMouseEvent me;
	me.type = SMouseEvent::PRESS;
	me.button = SMouseEvent::EButton(ev->button());
	me.x = ev->x();
	me.y = ev->y();
	me.viewport = this;
	me.shift = (ev->modifiers() & Qt::SHIFT) != 0;
	me.control = (ev->modifiers() & Qt::CTRL) != 0;
	OnMouseEvent(me);
	
	QWidget::mousePressEvent(ev);
	setFocus();

	m_mousePressPos = ev->pos();

	if (ev->button() == Qt::MiddleButton)
	{
		m_panMode = true;
		QApplication::setOverrideCursor(Qt::BlankCursor);
	}
	if (ev->button() == Qt::RightButton)
	{
		m_rotationMode = true;
		QApplication::setOverrideCursor(Qt::BlankCursor);
	}
}

void QViewport::mouseReleaseEvent(QMouseEvent* ev)
{
	SMouseEvent me;
	me.type = SMouseEvent::RELEASE;
	me.button = SMouseEvent::EButton(ev->button());
	me.x = ev->x();
	me.y = ev->y();
	me.viewport = this;
	OnMouseEvent(me);

	QWidget::mouseReleaseEvent(ev);

	if (ev->button() == Qt::MiddleButton)
	{
		if (m_panMode)
		{
			m_panMode = false;
		}
	}
	if (ev->button() == Qt::RightButton)
	{
		m_rotationMode = false;
	}
	QApplication::restoreOverrideCursor();
}

void QViewport::wheelEvent(QWheelEvent* ev)
{
	QuatT qt = m_state->cameraTarget;
	Vec3 ydir = qt.GetColumn1().GetNormalized();
	Vec3 pos = qt.t;
	const float wheelSpeed = m_settings->camera.zoomSpeed * (m_fastMode ? m_settings->camera.fastMoveMultiplier : 1.0f) * (m_slowMode ? m_settings->camera.slowMoveMultiplier : 1.0f);
	pos += 0.01f * ydir * ev->delta() * wheelSpeed;
	qt.t = pos;
	CameraMoved(qt);
}

void QViewport::mouseMoveEvent(QMouseEvent* ev)
{
	SMouseEvent me;
	me.type = SMouseEvent::MOVE;
	me.button = SMouseEvent::EButton(ev->button());
	me.x = ev->x();
	me.y = ev->y();
	me.viewport = this;
	m_fastMode = (ev->modifiers() & Qt::SHIFT) != 0; 
	m_slowMode = (ev->modifiers() & Qt::CTRL) != 0;
	OnMouseEvent(me);

	QWidget::mouseMoveEvent(ev);
}

void QViewport::keyPressEvent(QKeyEvent* ev)
{
	SKeyEvent event;
	event.type = SKeyEvent::PRESS;
	event.key = ev->key() | ev->modifiers();
	m_fastMode = (ev->modifiers() & Qt::SHIFT) != 0; 
	m_slowMode = (ev->modifiers() & Qt::CTRL) != 0;
	OnKeyEvent(event);

	QWidget::keyPressEvent(ev);
}

void QViewport::keyReleaseEvent(QKeyEvent* ev)
{
	SKeyEvent event;
	event.type = SKeyEvent::RELEASE;
	event.key = ev->key() | ev->modifiers();
	m_fastMode = (ev->modifiers() & Qt::SHIFT) != 0;
	m_slowMode = (ev->modifiers() & Qt::CTRL) != 0;
	OnKeyEvent(event);
	QWidget::keyReleaseEvent(ev);
}

void QViewport::resizeEvent(QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);

	int cx = ev->size().width();
	int cy = ev->size().height();
	if (cx == 0 || cy == 0)
		return;

	m_width = cx;
	m_height = cy;

	m_env->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, cx, cy);
	SignalUpdate();
}

void QViewport::moveEvent(QMoveEvent* ev)
{
	QWidget::moveEvent(ev);

	m_env->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, ev->pos().x(), ev->pos().y());
}

bool QViewport::event(QEvent* ev)
{
	bool result = QWidget::event(ev);

	if (ev->type() == QEvent::WinIdChange)
	{
		CreateRenderContext();
	}

	return result;
}

void QViewport::paintEvent(QPaintEvent* ev)
{
	QWidget::paintEvent(ev);
}

bool QViewport::winEvent(MSG* message, long* result)
{
	return QWidget::nativeEvent("windows_generic_MSG", message, result);
}

void QViewport::AddConsumer(QViewportConsumer* consumer)
{
	RemoveConsumer(consumer);
	m_consumers.push_back(consumer);
}

void QViewport::RemoveConsumer(QViewportConsumer* consumer)
{
	m_consumers.erase(std::remove(m_consumers.begin(), m_consumers.end(), consumer), m_consumers.end());
}

void QViewport::SetUseArrowsForNavigation(bool useArrowsForNavigation)
{
	m_useArrowsForNavigation = useArrowsForNavigation;
}

