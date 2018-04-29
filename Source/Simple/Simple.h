#pragma once

#include <Falcor.h>

using namespace Falcor;

class Simple : public Renderer
{
public:
	virtual void onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo) override;
	virtual void onGuiRender(SampleCallbacks* pSample, Gui* pGui) override;
	virtual void onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext) override;
	virtual bool onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent) override;
	virtual bool onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent) override;
private:
	void StartCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext);
	void EndCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext);
	void LoadModel();
	void LoadScene();

	glm::vec4 m_ClearColor = { 0.0f, 1.0f, 0.0f, 1.0f };
	Scene::SharedPtr m_Scene;
	SceneRenderer::SharedPtr m_Renderer;
	Camera::SharedPtr m_Camera;
	GraphicsProgram::SharedPtr m_Program;
	GraphicsState::SharedPtr m_State;
	GraphicsVars::SharedPtr m_Vars;
	FirstPersonCameraController m_CameraController;
	bool m_CaptureNextFrame = false;
	float m_CameraSpeed = 1.0f;
	glm::vec3 m_Ambient = {0.01f, 0.01f, 0.01f};

	GraphicsProgram::SharedPtr m_ShadowMapProgram;
	GraphicsVars::SharedPtr m_ShadowMapVars;
	Fbo::SharedPtr m_ShadowMapFBO;
	Camera::SharedPtr m_ShadowMapCamera;
};