#include "ShadowMapping.h"

void ShadowMapping::Initialize()
{
	m_ShadowMapProgram = GraphicsProgram::createFromFile("Shaders/ShadowMap.slang", "", "PSmain");
	m_ShadowMapVars = GraphicsVars::create(m_ShadowMapProgram->getReflector());

	{
		Fbo::Desc desc;
		desc.setDepthStencilTarget(ResourceFormat::D32Float);

		m_ShadowMapFBO = FboHelper::create2D(m_ShadowTextureSize, m_ShadowTextureSize, desc);
	}

	m_ShadowMapCamera = Camera::create();

	Sampler::Desc desc;
	desc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
	desc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point);
	m_ShadowMapSampler = Sampler::create(desc);
	m_State = GraphicsState::create();
}

void ShadowMapping::OnGui(Gui* pGui)
{
	if (pGui->beginGroup("Shadow Map Controls", true))
	{
		pGui->addFloatVar("Shadow Radius", m_ShadowRadius, 0.1f, 100000.0f, 1.0f);
		if (pGui->addIntVar("Shadow Texture Size", m_ShadowTextureSize))
		{
			Fbo::Desc desc;
			desc.setDepthStencilTarget(ResourceFormat::D32Float);

			m_ShadowMapFBO = FboHelper::create2D(m_ShadowTextureSize, m_ShadowTextureSize, desc);
		}
		pGui->addCheckBox("Show Shadow Map", m_ShowShadowMap);
		pGui->endGroup();
	}
}

void ShadowMapping::Render(RenderContext::SharedPtr pRenderContext, SceneRenderer::SharedPtr pRenderer, DirectionalLight::SharedPtr dirLight)
{
	pRenderContext->clearFbo(m_ShadowMapFBO.get(), { 0.0f, 0.0f, 0.0f, 0.0f }, 1.0f, 0, FboAttachmentType::Depth);

	m_State->setFbo(m_ShadowMapFBO);
	m_State->setProgram(m_ShadowMapProgram);
	pRenderContext->setGraphicsState(m_State);
	pRenderContext->setGraphicsVars(m_ShadowMapVars);

	m_ShadowMapCamera->setProjectionMatrix(glm::ortho(-m_ShadowRadius, m_ShadowRadius, -m_ShadowRadius, m_ShadowRadius, 1.0f, 1.0f + 2 * m_ShadowRadius));
	m_ShadowMapCamera->setViewMatrix(glm::lookAt(-dirLight->getWorldDirection() * m_ShadowRadius, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }));

	pRenderer->renderScene(pRenderContext.get(), m_ShadowMapCamera.get());
}

void ShadowMapping::DebugDraw(RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	if (m_ShowShadowMap)
	{
		pRenderContext->blit(m_ShadowMapFBO->getDepthStencilTexture()->getSRV(), pTargetFbo->getRenderTargetView(0), glm::vec4{ 0.0f, 0.0f, m_ShadowTextureSize, m_ShadowTextureSize }, glm::vec4{ 1000, 0, 1200, 200 });
		pRenderContext->flush(true);
	}
}