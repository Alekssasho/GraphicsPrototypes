#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "DeferredRendererSceneRenderer.h"

#include "GI/GlobaIllumination.h"

#include <RenderDoc/renderdoc_app.h>

using namespace Falcor;

class DeferredRenderer : public Renderer
{
public:
	DeferredRenderer(bool loadRenderDoc);

	void onLoad(SampleCallbacks* pSample, RenderContext* pRenderContext) override;
	void onFrameRender(SampleCallbacks* pSample, RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo) override;
	void onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height) override;
	bool onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent) override;
	bool onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent) override;
	void onGuiRender(SampleCallbacks* pSample, Gui* pGui) override;
	void onDroppedFile(SampleCallbacks* pSample, const std::string& filename) override;

private:
	Fbo::SharedPtr mpGBufferFbo;
	Fbo::SharedPtr mpMainFbo;
	Fbo::SharedPtr mpDepthPassFbo;
	Fbo::SharedPtr mpPostProcessFbo;

	struct ShadowPass
	{
		bool updateShadowMap = true;
		CascadedShadowMaps::SharedPtr pCsm;
		Texture::SharedPtr pVisibilityBuffer;
		glm::mat4 camVpAtLastCsmUpdate = glm::mat4();
	};
	ShadowPass mShadowPass;

	//  SkyBox Pass.
	struct
	{
		SkyBox::SharedPtr pEffect;
		DepthStencilState::SharedPtr pDS;
		Sampler::SharedPtr pSampler;
	} mSkyBox;

	//  GBufer Pass.
	struct
	{
		GraphicsVars::SharedPtr pVars;
		GraphicsProgram::SharedPtr pProgram;
		DepthStencilState::SharedPtr pDsState;
		RasterizerState::SharedPtr pNoCullRS;
		BlendState::SharedPtr pAlphaBlendBS;
	} mGBufferPass;

	//  Lighting Pass.
	struct
	{
		GraphicsVars::SharedPtr pVars;
		FullScreenPass::UniquePtr pLightingFullscreenPass;
		size_t LightArrayOffset;
		ParameterBlock::SharedPtr pGBufferBlock;
	} mLightingPass;

	struct
	{
		GraphicsVars::SharedPtr pVars;
		GraphicsProgram::SharedPtr pProgram;
	} mDepthPass;


	//  The Temporal Anti-Aliasing Pass.
	class
	{
	public:
		TemporalAA::SharedPtr pTAA;
		Fbo::SharedPtr getActiveFbo() { return pTAAFbos[activeFboIndex]; }
		Fbo::SharedPtr getInactiveFbo()  { return pTAAFbos[1 - activeFboIndex]; }
		void createFbos(uint32_t width, uint32_t height, const Fbo::Desc & fboDesc)
		{
			pTAAFbos[0] = FboHelper::create2D(width, height, fboDesc);
			pTAAFbos[1] = FboHelper::create2D(width, height, fboDesc);
		}

		void switchFbos() { activeFboIndex = 1 - activeFboIndex; }
		void resetFbos()
		{
			activeFboIndex = 0;
			pTAAFbos[0] = nullptr;
			pTAAFbos[1] = nullptr;
		}

		void resetFboActiveIndex() { activeFboIndex = 0;}

	private:
		Fbo::SharedPtr pTAAFbos[2];
		uint32_t activeFboIndex = 0;
	} mTAA;


	ToneMapping::SharedPtr mpToneMapper;

	struct
	{
		SSAO::SharedPtr pSSAO;
		FullScreenPass::UniquePtr pApplySSAOPass;
		GraphicsVars::SharedPtr pVars;
	} mSSAO;

	FXAA::SharedPtr mpFXAA;

	void beginFrame(RenderContext* pContext, Fbo* pTargetFbo, uint64_t frameId);
	void endFrame(RenderContext* pContext);
	void depthPass(RenderContext* pContext);
	void shadowPass(RenderContext* pContext);
	void renderSkyBox(RenderContext* pContext);
	void gBufferPass(RenderContext* pContext, Fbo* pTargetFbo);
	void lightingPass(RenderContext* pContext, Fbo* pTargetFbo);
	void executeFXAA(RenderContext* pContext, Fbo::SharedPtr pTargetFbo);
	void runTAA(RenderContext* pContext, Fbo::SharedPtr pColorFbo);
	void postProcess(RenderContext* pContext, Fbo::SharedPtr pTargetFbo);
	void ambientOcclusion(RenderContext* pContext);
	void runGI(RenderContext* pContext, double currentTime);
	void applyAOGI(RenderContext* pContext, Fbo::SharedPtr pTargetFbo);

	void renderOpaqueObjects(RenderContext* pContext);
	void renderTransparentObjects(RenderContext* pContext);

	void initSkyBox(const Texture::SharedPtr& texture);
	void initPostProcess();
	void initGBufferPass();
	void initLightingPass();
	void initDepthPass();
	void initShadowPass(uint32_t windowWidth, uint32_t windowHeight);
	void initSSAO();
	void updateLightProbe(const LightProbe::SharedPtr& pLight);
	void initAA(SampleCallbacks* pSample);

	void initControls();

	GraphicsState::SharedPtr mpState;
	DeferredRendererSceneRenderer::SharedPtr mpSceneRenderer;
	RtSceneRenderer::SharedPtr mpRtSceneRenderer;
	void loadModel(SampleCallbacks* pSample, const std::string& filename, bool showProgressBar);
	void loadScene(SampleCallbacks* pSample, const std::string& filename, bool showProgressBar);
	void initScene(SampleCallbacks* pSample, RtScene::SharedPtr pScene);
	void applyCustomSceneVars(const Scene* pScene, const std::string& filename);
	void resetScene();

	void setActiveCameraAspectRatio(uint32_t w, uint32_t h);
	void setSceneSampler(uint32_t maxAniso);

	void createTaaPatternGenerator(uint32_t fboWidth, uint32_t fboHeight);

	Sampler::SharedPtr mpSceneSampler;

	struct ProgramControl
	{
		bool enabled;
		bool unsetOnEnabled;
		std::string define;
		std::string value;
	};

	enum ControlID
	{
		SuperSampling,
		EnableShadows,
		EnableReflections,
		EnableSSAO,
		EnableHashedAlpha,
		EnableTransparency,
		VisualizeCascades,
		VisualizeAO,
		VisualizeSurfelCoverage,
		VisualizeGI,
		Count
	};

	enum class SamplePattern : uint32_t
	{
		Halton,
		DX11
	};

	enum class AAMode
	{
		None,
		TAA,
		FXAA
	};

	float mOpacityScale = 0.5f;
	AAMode mAAMode = AAMode::None;
	SamplePattern mTAASamplePattern = SamplePattern::Halton;
	void applyAaMode(SampleCallbacks* pSample);
	std::vector<ProgramControl> mControls;
	void applyLightingProgramControl(ControlID controlID);

	bool mUseCameraPath = true;
	void applyCameraPathState();
	bool mPerMaterialShader = false;
	bool mEnableDepthPass = true;
	bool mUseCsSkinning = false;
	void applyCsSkinningMode();
	static const std::string skDefaultScene;

	enum class GBufferDebugMode
	{
		None,
		Diffuse,
		Specular,
		Normal,
		Roughness,
		Depth,
	};

	GBufferDebugMode mGBufferDebugMode = GBufferDebugMode::None;

	// RenderDoc integration
	void LoadRenderDoc();
	void StartRenderDocCapture(SampleCallbacks* pSample, RenderContext* pRenderContext);
	void EndRenderDocCapture(SampleCallbacks* pSample, RenderContext* pRenderContext);

	RENDERDOC_API_1_1_2* mpRenderDocAPI = nullptr;
	bool mCaptureNextFrame = false;

	// GI
	GlobalIllumination mGI;
};
