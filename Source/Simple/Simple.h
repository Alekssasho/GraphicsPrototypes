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
	glm::vec4 m_ClearColor = { 0.0f, 1.0f, 0.0f, 1.0f };
	Model::SharedPtr m_Model;
	Camera::SharedPtr m_Camera;
	GraphicsProgram::SharedPtr m_Program;
	GraphicsState::SharedPtr m_State;
	GraphicsVars::SharedPtr m_Vars;
	DirectionalLight::SharedPtr m_DirLight;
	FirstPersonCameraController m_CameraController;
};