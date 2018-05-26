#pragma once

#include <Falcor.h>

#include "../Base/BaseRenderer.h"
#include "../Techniques/ShadowMapping.h"

using namespace Falcor;

class ShadowMapPrototype : public BaseRenderer
{
public:
	virtual void OnGUI(SampleCallbacks* pSample, Gui* pGui) override;
	virtual void OnLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext) override;
	virtual void OnRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo) override;

private:
	GraphicsProgram::SharedPtr m_Program;
	GraphicsState::SharedPtr m_State;
	GraphicsVars::SharedPtr m_Vars;
	float m_Ambient = 0.01f;

	glm::vec3 dirLight = { -0.819, -0.560, -0.124 };
	ShadowMapping m_DirLightShadowMap;
};