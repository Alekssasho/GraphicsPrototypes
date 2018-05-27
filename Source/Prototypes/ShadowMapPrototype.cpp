#include "ShadowMapPrototype.h"

void ShadowMapPrototype::OnGUI(SampleCallbacks* pSample, Gui* pGui)
{
	if (pGui->beginGroup("Scene Controls", true))
	{
		pGui->addFloatVar("Ambient", m_Ambient, 0.0f, 1.0f);
		if (pGui->addFloat3Var("Dir Light", dirLight, -1.0f, 1.0f))
		{
			dirLight = glm::normalize(dirLight);
		}
		m_DirLightShadowMap.OnGui(pGui);
		pGui->endGroup();
	}
}

void ShadowMapPrototype::OnLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	m_State = GraphicsState::create();
	m_Program = GraphicsProgram::createFromFile("Shaders/SimpleModel.slang", "", "PSmain");
	m_Vars = GraphicsVars::create(m_Program->getReflector());

	m_DirLightShadowMap.Initialize();
}

void ShadowMapPrototype::OnRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	// Check if we have dir light and create shadow map for it
	for (const auto& light : m_Scene->getLights())
	{
		if (light->getType() == LightDirectional)
		{
			auto dirl = std::static_pointer_cast<DirectionalLight>(light);
			dirl->setWorldDirection(dirLight);
			m_DirLightShadowMap.Render(pRenderContext, m_Renderer, dirl);
		}
	}

	m_State->setFbo(pTargetFbo);
	m_State->setProgram(m_Program);
	m_Vars["PerFrameCB"]["gAmbient"] = glm::vec3{ m_Ambient, m_Ambient, m_Ambient };
	m_Vars["PerFrameCB"]["gLightViewMatrix"] = m_DirLightShadowMap.GetLightProjectionMatrix();
	m_Vars->setTexture("gShadowMap", m_DirLightShadowMap.GetShadowMapTexture());
	m_Vars->setSampler("gShadowMapSampler", m_DirLightShadowMap.GetShadowMapSampler());
	pRenderContext->setGraphicsState(m_State);
	pRenderContext->setGraphicsVars(m_Vars);

	m_Renderer->renderScene(pRenderContext.get());

	m_DirLightShadowMap.DebugDraw(pRenderContext, pTargetFbo);
}

int main(int argc, char** argv)
{
	ShadowMapPrototype::UniquePtr pRenderer = std::make_unique<ShadowMapPrototype>();
	Falcor::SampleConfig config;
	config.windowDesc.title = "Shadow Map Prototype Window";
	config.windowDesc.resizableWindow = false;
	config.windowDesc.width = 1280;
	config.windowDesc.height = 720;
	config.argc = argc;
	config.argv = argv;
	Falcor::Sample::run(config, pRenderer);
}