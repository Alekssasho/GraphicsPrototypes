#include "Simple.h"

void Simple::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
	if (pGui->beginGroup("Scene Controls", true))
	{
		pGui->addFloat4Var("Clear Color", m_ClearColor, 0.0f, 1.0f);
		pGui->endGroup();
	}
}

void Simple::onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	m_Model = Model::createFromFile("Vampire/Vampire_A_Lusth.dae");
	if (!m_Model)
	{
		pSample->shutdown();
	}
	m_Camera = Camera::create();
	m_Camera->setTarget(m_Model->getCenter());
	m_Camera->setUpVector({ 0.0f, 1.0f, 0.0f });
	m_Camera->setPosition(m_Model->getRadius() * 5.0f * vec3{ 0.0f, 0.0f, 1.0f } + m_Model->getCenter());
	m_Camera->setDepthRange(0.001f, 100.0f);

	m_Program = GraphicsProgram::createFromFile("Shaders/SimpleModel.slang", "", "PSmain");
	m_State = GraphicsState::create();
	m_Vars = GraphicsVars::create(m_Program->getReflector());

	m_Vars["PerFrameCB"]["gAmbient"] = vec3{ 0.2f, 0.2f, 0.2f };

	m_DirLight = DirectionalLight::create();
	m_DirLight->setWorldDirection(glm::vec3(0.13f, 0.27f, -0.9f));
	ConstantBuffer* CB = m_Vars["PerFrameCB"].get();
	m_DirLight->setIntoProgramVars(m_Vars.get(), CB, "gDirLight");

	m_CameraController.attachCamera(m_Camera);
}

void Simple::onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	m_CameraController.update();

	pRenderContext->clearFbo(pTargetFbo.get(), m_ClearColor, 1.0f, 0, FboAttachmentType::All);

	m_State->setFbo(pTargetFbo);
	m_State->setProgram(m_Program);
	pRenderContext->setGraphicsState(m_State);
	pRenderContext->setGraphicsVars(m_Vars);
	ModelRenderer::render(pRenderContext.get(), m_Model, m_Camera.get());
}

bool Simple::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	return m_CameraController.onKeyEvent(keyEvent);
}

bool Simple::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
	return m_CameraController.onMouseEvent(mouseEvent);
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