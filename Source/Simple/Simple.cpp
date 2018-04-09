#include "Simple.h"

void Simple::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
	if (pGui->beginGroup("Scene Controls", true))
	{
		pGui->addFloat4Var("Clear Color", m_ClearColor, 0.0f, 1.0f);
		pGui->endGroup();
	}
}

void Simple::onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	pRenderContext->clearFbo(pTargetFbo.get(), m_ClearColor, 1.0f, 0, FboAttachmentType::All);
}

int main()
{
	Simple::UniquePtr pRenderer = std::make_unique<Simple>();
	Falcor::SampleConfig config;
	config.windowDesc.title = "Simple Window";
	config.windowDesc.resizableWindow = false;
	config.windowDesc.width = 1280;
	config.windowDesc.height = 720;
	Falcor::Sample::run(config, pRenderer);
}