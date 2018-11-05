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

	m_SurfelCoverage = ComputeProgram::createFromFile("ComputeCoverage.slang", "main");
	m_SurfelCoverageVars = ComputeVars::create(m_SurfelCoverage->getReflector());
	m_Coverage = Texture::create2D(giMapSize.x, giMapSize.y, ResourceFormat::R32Float, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource);

	m_ComputeState = ComputeState::create();
	m_CommonData = ParameterBlock::create(m_SurfelCoverage->getReflector()->getParameterBlock("Data"), true);

	m_SurfelCoverageVars->setParameterBlock("Data", m_CommonData);
	m_VisualizeSurfelsVars->setParameterBlock("Data", m_CommonData);

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
	auto var = m_CommonData->getReflection()->getResource("surfels.SurfelStorage");
	m_Surfels = StructuredBuffer::create(var->getName(), var->getType()->unwrapArray()->asResourceType()->inherit_shared_from_this::shared_from_this(), m_MaxSurfels);
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

	m_CommonData->setStructuredBuffer("surfels.SurfelStorage", m_Surfels);
}

Texture::SharedPtr GlobalIllumination::GenerateGIMap(RenderContext* pContext, const Camera* pCamera, const Texture::SharedPtr& pDepthTexture, const Texture::SharedPtr& pNormalTexture)
{
	// Prepare common data
	m_CommonData->setTexture("gBuff.Normal", pNormalTexture);
	m_CommonData->setTexture("gBuff.Depth", pDepthTexture);
	pCamera->setIntoConstantBuffer(
		m_CommonData->getDefaultConstantBuffer().get(),
		"camera");

	// New Surfel Placement
	// Compute Coverage
	m_SurfelCoverageVars->setTexture("gCoverage", m_Coverage);

	m_ComputeState->setProgram(m_SurfelCoverage);
	pContext->pushComputeState(m_ComputeState);
	pContext->pushComputeVars(m_SurfelCoverageVars);

	pContext->dispatch(m_Coverage->getWidth() / 16, m_Coverage->getHeight() / 16, 1);

	pContext->popComputeVars();
	pContext->popComputeState();

	if (m_VisualizeSurfels)
	{
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