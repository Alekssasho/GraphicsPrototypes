#pragma once
#include <Falcor.h>

using namespace Falcor;

class BaseRenderer : public Renderer
{
public:
	virtual void onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo) override final;
	virtual void onGuiRender(SampleCallbacks* pSample, Gui* pGui) override final;
	virtual void onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext) override final;
	virtual bool onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent) override final;
	virtual bool onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent) override final;

protected:
	virtual void OnGUI(SampleCallbacks* pSample, Gui* pGui) { };
	virtual void OnRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo) { };
	virtual void OnLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext) { };

private:
	void StartCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext);
	void EndCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext);
	void LoadModel();
	void LoadScene();

protected:
	glm::vec4 m_ClearColor = { 0.0f, 1.0f, 0.0f, 1.0f };
	Scene::SharedPtr m_Scene;
	SceneRenderer::SharedPtr m_Renderer;
	Camera::SharedPtr m_Camera;
	FirstPersonCameraController m_CameraController;
	bool m_CaptureNextFrame = false;
	float m_CameraSpeed = 1.0f;
};