#include "GlobaIllumination.h"

#include "Data/HostDeviceSurfelsData.h"

// TODO: not needed
#include "glm/gtc/random.hpp"

void GlobalIllumination::Initilize(const uvec2& giMapSize)
{
	Fbo::Desc fboDesc;
	fboDesc.setColorTarget(0, Falcor::ResourceFormat::RGBA8Unorm);
	m_GIFbo = FboHelper::create2D(giMapSize.x, giMapSize.y, fboDesc);

	m_VisualizeSurfelsProgram = FullScreenPass::create("VisualizeSurfels.slang");
	m_VisualizeSurfelsVars = GraphicsVars::create(m_VisualizeSurfelsProgram->getProgram()->getReflector());

	m_MaxSurfels = 5;// 1024 * 1024;
	ResetGI();
}

void GlobalIllumination::RenderUI(Gui* pGui)
{
	if(pGui->beginGroup("GI"))
	{
		// Statistics
		auto totalSurfelsSize = sizeof(Surfel) * m_MaxSurfels;
		std::string totalSurfelsSizeInMB = "Surfel Data: " + std::to_string(float(totalSurfelsSize) / (1024 * 1024)) + " MB";
		pGui->addText(totalSurfelsSizeInMB.c_str());

		if (pGui->addIntVar("Max Surfels", m_MaxSurfels, 0))
		{
			ResetGI();
		}

		pGui->addCheckBox("Visualize Surfels", m_VisualizeSurfels);

		m_Surfels->renderUI(pGui, "Surfels Data");

		if (pGui->addButton("Reset GI"))
		{
			ResetGI();
		}

		pGui->endGroup();
	}
}

void GlobalIllumination::ResetGI()
{
	m_Surfels = StructuredBuffer::create(m_VisualizeSurfelsProgram->getProgram(), "gSurfels", m_MaxSurfels);
	// Random initialize memory
	std::vector<Surfel> randomSurfels(m_MaxSurfels);
	std::generate(randomSurfels.begin(), randomSurfels.end(), []() {
		Surfel result;
		result.Position = glm::linearRand(glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 2.0f));
		result.Normal = glm::normalize(glm::linearRand(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
		result.Color = glm::linearRand(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		return result;
	});
	m_Surfels->setBlob(randomSurfels.data(), 0, randomSurfels.size() * sizeof(Surfel));
}

Texture::SharedPtr GlobalIllumination::GenerateGIMap(RenderContext* pContext, const Camera* pCamera, const Texture::SharedPtr& pDepthTexture, const Texture::SharedPtr& pNormalTexture)
{
	if (m_VisualizeSurfels)
	{
		m_VisualizeSurfelsVars->setTexture("gBuffNormal", pNormalTexture);
		m_VisualizeSurfelsVars->setTexture("gBuffDepth", pDepthTexture);
		pCamera->setIntoConstantBuffer(
			m_VisualizeSurfelsVars["GlobalData"].get(),
			"globalCamera");

		m_VisualizeSurfelsVars->setStructuredBuffer("gSurfels", m_Surfels);

		pContext->getGraphicsState()->setFbo(m_GIFbo);
		pContext->pushGraphicsVars(m_VisualizeSurfelsVars);
		m_VisualizeSurfelsProgram->execute(pContext);
		pContext->popGraphicsVars();
	}
	else
	{
		// TODO: Implement
		pContext->clearFbo(m_GIFbo.get(), vec4(0.0f), 0.0f, 0, FboAttachmentType::Color);
	}

	return m_GIFbo->getColorTexture(0);
}