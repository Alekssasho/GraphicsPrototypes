#pragma once

#include <Falcor.h>
#include <FalcorExperimental.h>

using namespace Falcor;

class GlobalIllumination
{
public:
	void Initilize(const uvec2& giMapSize);
	void RenderUI(Gui* pGui);

	Texture::SharedPtr GenerateGIMap(RenderContext* pContext,
		RtSceneRenderer* pSceneRenderer,
		double currentTime,
		const Camera* pCamera,
		const Texture::SharedPtr& pDepthTexture,
		const Texture::SharedPtr& pNormalTexture,
		const Texture::SharedPtr& pAlbedoTexture);

	Texture::SharedPtr GetSurfelCoverageTexture() { return m_Coverage; }
	Texture::SharedPtr GetIrradianceTexture() { return m_Irradiance; }
	Texture::SharedPtr GetDebugTexture() { return m_DebugTexture; }
private:
	void ResetGI();

	void ExclusiveScan(RenderContext* pContext);

	// Data Structures
	StructuredBuffer::SharedPtr m_Surfels;
	StructuredBuffer::SharedPtr m_SurfelCount;
	int32_t m_MaxSurfels;

	// Outputs
	Texture::SharedPtr m_GIMap;

	// Debug Visualization
	ComputeProgram::SharedPtr m_SurfelRendering;
	ComputeVars::SharedPtr m_SurfelRenderingVars;
	bool m_VisualizeSurfels = false;

	// Utility objects
	ComputeState::SharedPtr m_ComputeState;
	ParameterBlock::SharedPtr m_CommonData;

	// Surfels Placement
	ComputeProgram::SharedPtr m_SurfelCoverage;
	ComputeVars::SharedPtr m_SurfelCoverageVars;
	ComputeProgram::SharedPtr m_UpdateWorldStructure;
	ComputeVars::SharedPtr m_UpdateWorldStructureVars;
	ComputeProgram::SharedPtr m_SpawnSurfel;
	ComputeVars::SharedPtr m_SpawnSurfelVars;
	float m_SpawnChance = 1.0f;
	bool m_UpdateTime = true;

	// Resources
	Texture::SharedPtr m_Coverage;
	StructuredBuffer::SharedPtr m_SurfelSpawnCoords;
	StructuredBuffer::SharedPtr m_NewSurfelCounts;
	Buffer::SharedPtr m_NewSurfelCountBuffer;
	StructuredBuffer::SharedPtr m_WorldStructure;
	StructuredBuffer::SharedPtr m_SurfelIndices[2];
	uint32_t m_CurrentSurfelIndicesBuffer = 0;
	Texture::SharedPtr m_Irradiance;

	// Exclusive Scan implementation
	StructuredBuffer::SharedPtr m_ScannedNewSurfelCounts;
	StructuredBuffer::SharedPtr m_ScanAuxiliaryBuffer[2];
	ComputeProgram::SharedPtr m_ScanBlocks;
	ComputeProgram::SharedPtr m_ScanSumOfBlocks;
	ComputeProgram::SharedPtr m_AddSumToBlocks;
	ComputeVars::SharedPtr m_ScanVars;

	// RayTracing
	RtProgram::SharedPtr m_SurfelAccumulateProgram;
	RtProgramVars::SharedPtr m_SurfelAccumulateVars;
	RtState::SharedPtr m_RTState;
	RtScene::SharedPtr m_CachedScene;
	int32_t m_SurfelAccumulateRayBudget;

	// Rendering stuff
	bool m_UseWeightFunctions = true;
	bool m_ApplyGI = true;

	Texture::SharedPtr m_DebugTexture;
	GaussianBlur::UniquePtr m_Blur;
	bool m_ApplyBlur = false;
	Fbo::SharedPtr m_BlurFbo;
};