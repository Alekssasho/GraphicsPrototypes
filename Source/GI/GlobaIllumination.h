#pragma once

#include <Falcor.h>

using namespace Falcor;

class GlobalIllumination
{
public:
	void Initilize(const uvec2& giMapSize);
	void RenderUI(Gui* pGui);

	Texture::SharedPtr GenerateGIMap(RenderContext* pContext, const Camera* pCamera, const Texture::SharedPtr& pDepthTexture, const Texture::SharedPtr& pNormalTexture);
private:
	void ResetGI();

	// Data Structures
	StructuredBuffer::SharedPtr m_Surfels;
	int32_t m_MaxSurfels;

	// Outputs
	Fbo::SharedPtr m_GIFbo;

	// Debug Visualization
	FullScreenPass::UniquePtr m_VisualizeSurfelsProgram;
	GraphicsVars::SharedPtr m_VisualizeSurfelsVars;
	bool m_VisualizeSurfels = false;
};