#pragma once

#include <Falcor.h>

using namespace Falcor;

class ShadowMapping
{
public:
	void Initialize();
	void OnGui(Gui* pGui);
	void Render(RenderContext::SharedPtr pRenderContext, SceneRenderer::SharedPtr pRenderer, DirectionalLight::SharedPtr dirLight);
	Texture::SharedPtr GetShadowMapTexture() { return m_ShadowMapFBO->getDepthStencilTexture(); }
	Sampler::SharedPtr GetShadowMapSampler() { return m_ShadowMapSampler; }
	const mat4& GetLightProjectionMatrix() { return m_ShadowMapCamera->getViewProjMatrix(); }
	void DebugDraw(RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo);
private:
	GraphicsState::SharedPtr m_State;
	GraphicsProgram::SharedPtr m_ShadowMapProgram;
	GraphicsVars::SharedPtr m_ShadowMapVars;
	Fbo::SharedPtr m_ShadowMapFBO;
	Camera::SharedPtr m_ShadowMapCamera;
	Sampler::SharedPtr m_ShadowMapSampler;

	int m_ShadowTextureSize = 2048;
	float m_ShadowRadius = 75.0f;
	bool m_ShowShadowMap = false;
};