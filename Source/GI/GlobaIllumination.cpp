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
	m_Coverage = Texture::create2D(giMapSize.x, giMapSize.y, ResourceFormat::RG32Float, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource);

	m_ComputeState = ComputeState::create();
	m_CommonData = ParameterBlock::create(m_SurfelCoverage->getReflector()->getParameterBlock("Data"), true);

	m_SurfelCoverageVars["GlobalState"]["globalSpawnChance"] = m_SpawnChance;
	m_SurfelCoverageVars->setParameterBlock("Data", m_CommonData);
	m_VisualizeSurfelsVars->setParameterBlock("Data", m_CommonData);

	m_MaxSurfels = 1024;// *1024;
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

		uint32_t count;
		m_SurfelCount->getVariable(0, 0, count);
		pGui->addText((std::string("Surfel Count: ") + std::to_string(count)).c_str());

		if (pGui->addFloatVar("Spawn Chance", m_SpawnChance, 0.0f, 1.0f))
		{
			m_SurfelCoverageVars["GlobalState"]["globalSpawnChance"] = m_SpawnChance;
		}

		pGui->addCheckBox("Update Time", m_UpdateTime);

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
	auto var = m_CommonData->getReflection()->getResource("Surfels.Storage");
	m_Surfels = StructuredBuffer::create(var->getName(), var->getType()->unwrapArray()->asResourceType()->inherit_shared_from_this::shared_from_this(), m_MaxSurfels);
	//// Random initialize memory
	//std::vector<Surfel> randomSurfels(m_MaxSurfels);
	//std::generate(randomSurfels.begin(), randomSurfels.end(), []() {
	//	Surfel result;
	//	result.Position = glm::linearRand(glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 2.0f));
	//	result.Normal = glm::normalize(glm::linearRand(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
	//	result.Color = glm::linearRand(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
	//	return result;
	//});
	//m_Surfels->setBlob(randomSurfels.data(), 0, randomSurfels.size() * sizeof(Surfel));

	m_CommonData->setStructuredBuffer("Surfels.Storage", m_Surfels);

	auto varCount = m_CommonData->getReflection()->getResource("Surfels.Count");
	m_SurfelCount = StructuredBuffer::create(varCount->getName(), varCount->getType()->unwrapArray()->asResourceType()->inherit_shared_from_this::shared_from_this(), 1);
	uint32_t count = 0;
	m_SurfelCount->setBlob(&count, 0, sizeof(uint32_t));
	m_CommonData->setStructuredBuffer("Surfels.Count", m_SurfelCount);
}

Texture::SharedPtr GlobalIllumination::GenerateGIMap(RenderContext* pContext, double currentTime, const Camera* pCamera, const Texture::SharedPtr& pDepthTexture, const Texture::SharedPtr& pNormalTexture)
{
	// Prepare common data
	m_CommonData->setTexture("GBuffer.Normal", pNormalTexture);
	m_CommonData->setTexture("GBuffer.Depth", pDepthTexture);
	pCamera->setIntoConstantBuffer(
		m_CommonData->getDefaultConstantBuffer().get(),
		"Camera");

	// New Surfel Placement
	// Compute Coverage
	m_SurfelCoverageVars->setTexture("gCoverage", m_Coverage);
	if (m_UpdateTime)
	{
		m_SurfelCoverageVars["GlobalState"]["globalTime"] = float(currentTime);
	}

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