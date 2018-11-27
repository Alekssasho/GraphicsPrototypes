#include "GlobaIllumination.h"

#include "Data/HostDeviceSurfelsData.h"

void GlobalIllumination::Initilize(const uvec2& giMapSize)
{
	Fbo::Desc fboDesc;
	fboDesc.setColorTarget(0, Falcor::ResourceFormat::RGBA8Unorm);
	m_GIFbo = FboHelper::create2D(giMapSize.x, giMapSize.y, fboDesc);

	m_VisualizeSurfelsProgram = FullScreenPass::create("VisualizeSurfels.slang");
	m_VisualizeSurfelsVars = GraphicsVars::create(m_VisualizeSurfelsProgram->getProgram()->getReflector());

	m_SurfelCoverage = ComputeProgram::createFromFile("ComputeCoverage.slang", "main");
	m_SurfelCoverageVars = ComputeVars::create(m_SurfelCoverage->getReflector());

	m_SpawnSurfel = ComputeProgram::createFromFile("SpawnSurfels.slang", "main");
	m_SpawnSurfelVars = ComputeVars::create(m_SpawnSurfel->getReflector());

	m_UpdateWorldStructure = ComputeProgram::createFromFile("UpdateWorldStructure.slang", "main");
	m_UpdateWorldStructureVars = ComputeVars::create(m_UpdateWorldStructure->getReflector());

	m_Coverage = Texture::create2D(giMapSize.x, giMapSize.y, ResourceFormat::RG32Float, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource);

	uint32_t initialData[3] = { 0, 1, 1 };
	m_NewSurfelCountBuffer = Buffer::create(sizeof(uint32_t) * 3, Resource::BindFlags::UnorderedAccess, Buffer::CpuAccess::None, &initialData);

	m_SurfelSpawnCoords = StructuredBuffer::create(m_SurfelCoverage, "gSurfelSpawnCoords", (giMapSize.x / 16) * (giMapSize.y / 16));

	m_NewSurfelCounts = StructuredBuffer::create(m_SurfelCoverage, "gNewSurfelsCount", WORLD_STRUCTURE_TOTAL_SIZE);

	m_WorldStructure = StructuredBuffer::create(m_UpdateWorldStructure, "gWorldStructure", WORLD_STRUCTURE_TOTAL_SIZE);

	m_ComputeState = ComputeState::create();
	m_CommonData = ParameterBlock::create(m_SurfelCoverage->getReflector()->getParameterBlock("Data"), true);
	m_CommonData->setStructuredBuffer("Surfels.WorldStructure", m_WorldStructure);

	m_SurfelCoverageVars["GlobalState"]["globalSpawnChance"] = m_SpawnChance;
	m_SurfelCoverageVars->setStructuredBuffer("gSurfelSpawnCoords", m_SurfelSpawnCoords);
	m_SurfelCoverageVars->setStructuredBuffer("gNewSurfelsCount", m_NewSurfelCounts);

	m_SpawnSurfelVars->setStructuredBuffer("gNewSurfelsCount", m_NewSurfelCounts);
	m_SpawnSurfelVars->setStructuredBuffer("gSurfelSpawnCoords", m_SurfelSpawnCoords);
	m_SpawnSurfelVars->setRawBuffer("gSurfelCount", m_SurfelSpawnCoords->getUAVCounter());

	m_SurfelCoverageVars->setParameterBlock("Data", m_CommonData);
	m_SpawnSurfelVars->setParameterBlock("Data", m_CommonData);
	m_VisualizeSurfelsVars->setParameterBlock("Data", m_CommonData);

	m_ScannedNewSurfelCounts = StructuredBuffer::create(m_SurfelCoverage, "gNewSurfelsCount", WORLD_STRUCTURE_TOTAL_SIZE);

	m_UpdateWorldStructureVars->setStructuredBuffer("gWorldStructure", m_WorldStructure);
	m_UpdateWorldStructureVars->setStructuredBuffer("gNewSurfelsCount", m_NewSurfelCounts);
	m_UpdateWorldStructureVars->setStructuredBuffer("gScannedNewSurfelsCount", m_ScannedNewSurfelCounts);

	// Exclusive Scan
	m_ScanBlocks = ComputeProgram::createFromFile("ExclusiveScan.slang", "ExclusiveScanInBlocks");
	m_ScanSumOfBlocks = ComputeProgram::createFromFile("ExclusiveScan.slang", "ExclusiveScanSumsOfBlocks");
	m_AddSumToBlocks = ComputeProgram::createFromFile("ExclusiveScan.slang", "AddSumsToBlocks");
	m_ScanVars = ComputeVars::create(m_AddSumToBlocks->getReflector());
	assert(WORLD_STRUCTURE_TOTAL_SIZE <= (1024 * 1024));
	m_ScanAuxiliaryBuffer[0] = StructuredBuffer::create(m_SurfelCoverage, "gNewSurfelsCount", 1024);
	m_ScanAuxiliaryBuffer[1] = StructuredBuffer::create(m_SurfelCoverage, "gNewSurfelsCount", 1024);

	m_MaxSurfels = 1024 *1024;
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

	m_CommonData->setStructuredBuffer("Surfels.Storage", m_Surfels);

	auto varCount = m_CommonData->getReflection()->getResource("Surfels.Count");
	m_SurfelCount = StructuredBuffer::create(varCount->getName(), varCount->getType()->unwrapArray()->asResourceType()->inherit_shared_from_this::shared_from_this(), 1);
	uint32_t count = 0;
	m_SurfelCount->setBlob(&count, 0, sizeof(uint32_t));
	m_CommonData->setStructuredBuffer("Surfels.Count", m_SurfelCount);

	m_SurfelIndices[0] = StructuredBuffer::create(m_UpdateWorldStructure, "gNewSurfelIndices", m_MaxSurfels);
	m_SurfelIndices[1] = StructuredBuffer::create(m_UpdateWorldStructure, "gNewSurfelIndices", m_MaxSurfels);

	std::vector<WorldStructureChunk> tempData(WORLD_STRUCTURE_TOTAL_SIZE);
	memset(tempData.data(), 0, tempData.size() * sizeof(WorldStructureChunk));
	m_WorldStructure->updateData(tempData.data(), 0, tempData.size() * sizeof(WorldStructureChunk));
}

Texture::SharedPtr GlobalIllumination::GenerateGIMap(RenderContext* pContext, double currentTime, const Camera* pCamera, const Texture::SharedPtr& pDepthTexture, const Texture::SharedPtr& pNormalTexture)
{
	// Prepare common data
	m_CommonData->setTexture("GBuffer.Normal", pNormalTexture);
	m_CommonData->setTexture("GBuffer.Depth", pDepthTexture);
	pCamera->setIntoConstantBuffer(
		m_CommonData->getDefaultConstantBuffer().get(),
		"Camera");

	m_CommonData->setStructuredBuffer("Surfels.Indices", m_SurfelIndices[m_CurrentSurfelIndicesBuffer]);

	// Reset counter
	uint32_t zero = 0;
	m_SurfelSpawnCoords->getUAVCounter()->updateData(&zero, 0, sizeof(zero));

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

	pContext->copyBufferRegion(m_NewSurfelCountBuffer.get(), 0, m_SurfelSpawnCoords->getUAVCounter().get(), 0, sizeof(uint32_t));

	ExclusiveScan(pContext);

	m_UpdateWorldStructureVars->setStructuredBuffer("gOldSurfelIndices", m_SurfelIndices[m_CurrentSurfelIndicesBuffer]);
	m_UpdateWorldStructureVars->setStructuredBuffer("gNewSurfelIndices", m_SurfelIndices[(m_CurrentSurfelIndicesBuffer + 1) % 2]);
	m_CurrentSurfelIndicesBuffer = (m_CurrentSurfelIndicesBuffer + 1) % 2;
	m_ComputeState->setProgram(m_UpdateWorldStructure);
	pContext->pushComputeVars(m_UpdateWorldStructureVars);

	pContext->dispatch(WORLD_STRUCTURE_DIMENSION, WORLD_STRUCTURE_DIMENSION, WORLD_STRUCTURE_DIMENSION);

	pContext->popComputeVars();

	m_CommonData->setStructuredBuffer("Surfels.Indices", m_SurfelIndices[m_CurrentSurfelIndicesBuffer]);

	m_SpawnSurfelVars->setStructuredBuffer("gIndices", m_SurfelIndices[m_CurrentSurfelIndicesBuffer]);
	m_ComputeState->setProgram(m_SpawnSurfel);
	pContext->pushComputeVars(m_SpawnSurfelVars);

	pContext->dispatchIndirect(m_NewSurfelCountBuffer.get(), 0);

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

void GlobalIllumination::ExclusiveScan(RenderContext* pContext)
{
	pContext->pushComputeState(m_ComputeState);
	pContext->pushComputeVars(m_ScanVars);

	{
		m_ComputeState->setProgram(m_ScanBlocks);
		m_ScanVars->setStructuredBuffer("Input", m_NewSurfelCounts);
		m_ScanVars->setStructuredBuffer("Result", m_ScannedNewSurfelCounts);
		m_ScanVars->setStructuredBuffer("BlockSums", m_ScanAuxiliaryBuffer[0]);
		pContext->dispatch(WORLD_STRUCTURE_TOTAL_SIZE / 1024, 1, 1);
	}
	{
		m_ComputeState->setProgram(m_ScanSumOfBlocks);
		m_ScanVars->setStructuredBuffer("Input", m_ScanAuxiliaryBuffer[0]);
		m_ScanVars->setStructuredBuffer("Result", m_ScanAuxiliaryBuffer[1]);
		pContext->dispatch(1, 1, 1);
	}
	{
		m_ComputeState->setProgram(m_AddSumToBlocks);
		m_ScanVars->setStructuredBuffer("Input", m_ScanAuxiliaryBuffer[1]);
		m_ScanVars->setStructuredBuffer("Result", m_ScannedNewSurfelCounts);
		pContext->dispatch(WORLD_STRUCTURE_TOTAL_SIZE / 1024, 1, 1);
	}

	pContext->popComputeVars();
	pContext->popComputeState();
}