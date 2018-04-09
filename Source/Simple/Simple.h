#pragma once

#include <Falcor.h>

using namespace Falcor;

class Simple : public Renderer
{
public:
	virtual void onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo) override;
	virtual void onGuiRender(SampleCallbacks* pSample, Gui* pGui) override;
private:
	glm::vec4 m_ClearColor = { 0.0f, 1.0f, 0.0f, 1.0f };
};