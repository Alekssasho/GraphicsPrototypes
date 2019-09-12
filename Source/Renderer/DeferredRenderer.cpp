#include "DeferredRenderer.h"

#include "pix3.h"

void BeginMarker(RenderContext* renderContext, const char* name)
{
	PIXBeginEvent(renderContext->getLowLevelData()->getCommandList().GetInterfacePtr(), PIX_COLOR(255, 255, 255), name);
}

void EndMarker(RenderContext* renderContext)
{
	PIXEndEvent(renderContext->getLowLevelData()->getCommandList().GetInterfacePtr());
}

struct MarkerScope
{
	MarkerScope(RenderContext* context, const char* name)
		: m_Context(context)
	{
		BeginMarker(context, name);
	}

	~MarkerScope()
	{
		EndMarker(m_Context);
	}

	MarkerScope(const MarkerScope&) = delete;
	MarkerScope(MarkerScope&&) = delete;
	MarkerScope& operator=(const MarkerScope&) = delete;
	MarkerScope& operator=(MarkerScope&&) = delete;

	RenderContext* m_Context;
};

//const std::string DeferredRenderer::skDefaultScene = "Arcade/Arcade.fscene";
const std::string DeferredRenderer::skDefaultScene = "G:/Development/Models/MorganMcGuire/CornellBox/falcor_scene.fscene";

void DeferredRenderer::initDepthPass()
{
	mDepthPass.pProgram = GraphicsProgram::createFromFile("DepthPass.ps.slang", "", "main");
	mDepthPass.pVars = GraphicsVars::create(mDepthPass.pProgram->getReflector());
}

void DeferredRenderer::initLightingPass()
{
	Program::DefineList defines;
	defines.add("_LIGHT_COUNT", std::to_string(mpSceneRenderer->getScene()->getLightCount()));

	mLightingPass.pLightingFullscreenPass = FullScreenPass::create("LightingPass.ps.slang", defines);
	mLightingPass.pVars = GraphicsVars::create(mLightingPass.pLightingFullscreenPass->getProgram()->getReflector());
	mLightingPass.pGBufferBlock = ParameterBlock::create(mLightingPass.pLightingFullscreenPass->getProgram()->getReflector()->getParameterBlock("GB"), false);
	mLightingPass.pVars->setParameterBlock("GB", mLightingPass.pGBufferBlock);

	mLightingPass.LightArrayOffset = mLightingPass.pVars["PerFrame"]->getVariableOffset("Lights[0]");
}

void DeferredRenderer::initGBufferPass()
{
	mGBufferPass.pProgram = GraphicsProgram::createFromFile("GBufferPass.slang", "vs", "ps");
	initControls();
	mGBufferPass.pVars = GraphicsVars::create(mGBufferPass.pProgram->getReflector());
    
	DepthStencilState::Desc dsDesc;
	dsDesc.setDepthTest(true).setStencilTest(false)./*setDepthWriteMask(false).*/setDepthFunc(DepthStencilState::Func::LessEqual);
	mGBufferPass.pDsState = DepthStencilState::create(dsDesc);

	RasterizerState::Desc rsDesc;
	rsDesc.setCullMode(RasterizerState::CullMode::None);
	mGBufferPass.pNoCullRS = RasterizerState::create(rsDesc);

	BlendState::Desc bsDesc;
	bsDesc.setRtBlend(0, true).setRtParams(0, BlendState::BlendOp::Add, BlendState::BlendOp::Add, BlendState::BlendFunc::SrcAlpha, BlendState::BlendFunc::OneMinusSrcAlpha, BlendState::BlendFunc::One, BlendState::BlendFunc::Zero);
	mGBufferPass.pAlphaBlendBS = BlendState::create(bsDesc);
}

void DeferredRenderer::initShadowPass(uint32_t windowWidth, uint32_t windowHeight)
{
	mShadowPass.pCsm = CascadedShadowMaps::create(mpSceneRenderer->getScene()->getLight(0), 2048, 2048, windowWidth, windowHeight, mpSceneRenderer->getScene()->shared_from_this());
	mShadowPass.pCsm->setFilterMode(CsmFilterHwPcf);
	//mShadowPass.pCsm->setVsmLightBleedReduction(0.3f);
	//mShadowPass.pCsm->setVsmMaxAnisotropy(4);
	//mShadowPass.pCsm->setEvsmBlur(7, 3);
}

void DeferredRenderer::initSSAO()
{
	mSSAO.pSSAO = SSAO::create(uvec2(1024));
	mSSAO.pApplySSAOPass = FullScreenPass::create("ApplyAOGI.slang");
	mSSAO.pVars = GraphicsVars::create(mSSAO.pApplySSAOPass->getProgram()->getReflector());

	Sampler::Desc desc;
	desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
	mSSAO.pVars->setSampler("gSampler", Sampler::create(desc));
	ambientValue = 1.0f;
}

void DeferredRenderer::setSceneSampler(uint32_t maxAniso)
{
	Scene* pScene = const_cast<Scene*>(mpSceneRenderer->getScene().get());
	Sampler::Desc samplerDesc;
	samplerDesc.setAddressingMode(Sampler::AddressMode::Wrap, Sampler::AddressMode::Wrap, Sampler::AddressMode::Wrap).setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(maxAniso);
	mpSceneSampler = Sampler::create(samplerDesc);
	pScene->bindSampler(mpSceneSampler);
}

void DeferredRenderer::applyCustomSceneVars(const Scene* pScene, const std::string& filename)
{
	std::string folder = getDirectoryFromFile(filename);

	if (pScene->getEnvironmentMap()) initSkyBox(pScene->getEnvironmentMap());

	auto var = pScene->getUserVariable("opacity_scale");
	if (var.type == Scene::UserVariable::Type::Double) mOpacityScale = (float)var.d64;
}

void DeferredRenderer::initScene(SampleCallbacks* pSample, RtScene::SharedPtr pScene)
{
	if (pScene->getCameraCount() == 0)
	{
		// Place the camera above the center, looking slightly downwards
		const Model* pModel = pScene->getModel(0).get();
		Camera::SharedPtr pCamera = Camera::create();

		vec3 position = pModel->getCenter();
		float radius = pModel->getRadius();
		position.y += 0.3f * radius;
		position.z += 2 * radius;
		pScene->setCameraSpeed(radius * 0.3f);

		pCamera->setPosition(position);
		pCamera->setTarget(position + vec3(0, -0.3f, -radius));
		pCamera->setDepthRange(0.1f, radius * 10);

		pScene->addCamera(pCamera);
	}

	if (pScene->getLightCount() == 0)
	{
		// Create a directional light
		DirectionalLight::SharedPtr pDirLight = DirectionalLight::create();
		pDirLight->setWorldDirection(vec3(-0.189f, -0.861f, -0.471f));
		pDirLight->setIntensity(vec3(1, 1, 0.985f) * 10.0f);
		pDirLight->setName("DirLight");
		pScene->addLight(pDirLight);
	}

	if (pScene->getLightProbeCount() > 0)
	{
		const LightProbe::SharedPtr& pProbe = pScene->getLightProbe(0);
		pProbe->setRadius(pScene->getRadius());
		pProbe->setPosW(pScene->getCenter());
		pProbe->setSampler(mpSceneSampler);
	}

	mpSceneRenderer = DeferredRendererSceneRenderer::create(pScene);
	mpSceneRenderer->setCameraControllerType(SceneRenderer::CameraControllerType::FirstPerson);
	mpSceneRenderer->toggleStaticMaterialCompilation(mPerMaterialShader);
	setSceneSampler(mpSceneSampler ? mpSceneSampler->getMaxAnisotropy() : 4);
	setActiveCameraAspectRatio(pSample->getCurrentFbo()->getWidth(), pSample->getCurrentFbo()->getHeight());
	initDepthPass();
	initLightingPass();
	initGBufferPass();
	auto pTargetFbo = pSample->getCurrentFbo();
	initShadowPass(pTargetFbo->getWidth(), pTargetFbo->getHeight());
	initSSAO();
	initAA(pSample);

	mControls[EnableReflections].enabled = pScene->getLightProbeCount() > 0;
	applyLightingProgramControl(ControlID::EnableReflections);
    
	pSample->setCurrentTime(0);
	mpSceneRenderer->getScene()->getActiveCamera()->setDepthRange(0.1f, 100.0f);

	mGI.Initilize(uvec2(pTargetFbo->getWidth(), pTargetFbo->getHeight()));

	mpRtSceneRenderer = RtSceneRenderer::create(pScene);
}

void DeferredRenderer::resetScene()
{
	mpSceneRenderer = nullptr;
	mSkyBox.pEffect = nullptr;
}

void DeferredRenderer::loadModel(SampleCallbacks* pSample, const std::string& filename, bool showProgressBar)
{
	Mesh::resetGlobalIdCounter();
	resetScene();

	ProgressBar::SharedPtr pBar;
	if (showProgressBar)
	{
		pBar = ProgressBar::create("Loading Model");
	}

	RtModel::SharedPtr pModel = RtModel::createFromFile(filename.c_str());
	if (!pModel) return;
	RtScene::SharedPtr pScene = RtScene::createFromModel(pModel);

	initScene(pSample, pScene);
}

void DeferredRenderer::loadScene(SampleCallbacks* pSample, const std::string& filename, bool showProgressBar)
{
	Mesh::resetGlobalIdCounter();
	resetScene();

	ProgressBar::SharedPtr pBar;
	if (showProgressBar)
	{
		pBar = ProgressBar::create("Loading Scene", 100);
	}

	RtScene::SharedPtr pScene = RtScene::loadFromFile(filename);

	if (pScene != nullptr)
	{
		initScene(pSample, pScene);
		applyCustomSceneVars(pScene.get(), filename);
		applyCsSkinningMode();
	}
}

void DeferredRenderer::initSkyBox(const Texture::SharedPtr& texture)
{
	Sampler::Desc samplerDesc;
	samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
	mSkyBox.pSampler = Sampler::create(samplerDesc);
	mSkyBox.pEffect = SkyBox::create(texture, mSkyBox.pSampler);
	DepthStencilState::Desc dsDesc;
	dsDesc.setDepthFunc(DepthStencilState::Func::Always);
	mSkyBox.pDS = DepthStencilState::create(dsDesc);
}

void DeferredRenderer::updateLightProbe(const LightProbe::SharedPtr& pLight)
{
	Scene::SharedPtr pScene = mpSceneRenderer->getScene();

	// Remove existing light probes
	while (pScene->getLightProbeCount() > 0)
	{
		pScene->deleteLightProbe(0);
	}

	pLight->setRadius(pScene->getRadius());
	pLight->setPosW(pScene->getCenter());
	pLight->setSampler(mpSceneSampler);
	pScene->addLightProbe(pLight);

	mControls[EnableReflections].enabled = true;
	applyLightingProgramControl(ControlID::EnableReflections);
}

void DeferredRenderer::initAA(SampleCallbacks* pSample)
{
	mTAA.pTAA = TemporalAA::create();
	mpFXAA = FXAA::create();
	applyAaMode(pSample);
}

void DeferredRenderer::initPostProcess()
{
	mpToneMapper = ToneMapping::create(ToneMapping::Operator::Clamp);
}

void DeferredRenderer::onLoad(SampleCallbacks* pSample, RenderContext* pRenderContext)
{
	mpState = GraphicsState::create();
	initPostProcess();
	loadScene(pSample, skDefaultScene, true);
}

void DeferredRenderer::renderSkyBox(RenderContext* pContext)
{
	if (mSkyBox.pEffect)
	{
		MarkerScope scope(pContext, "Sky box");
		PROFILE("skyBox");
		mpState->setDepthStencilState(mSkyBox.pDS);
		mSkyBox.pEffect->render(pContext, mpSceneRenderer->getScene()->getActiveCamera().get());
		mpState->setDepthStencilState(nullptr);
	}
}

void DeferredRenderer::beginFrame(RenderContext* pContext, Fbo* pTargetFbo, uint64_t frameId)
{
	pContext->pushGraphicsState(mpState);
	pContext->clearFbo(mpMainFbo.get(), glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), 1, 0, FboAttachmentType::All);
	pContext->clearFbo(mpGBufferFbo.get(), glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), 1, 0, FboAttachmentType::All);
	pContext->clearFbo(mpPostProcessFbo.get(), glm::vec4(), 1, 0, FboAttachmentType::Color);

	if (mAAMode == AAMode::TAA)
	{
		glm::vec2 targetResolution = glm::vec2(pTargetFbo->getWidth(), pTargetFbo->getHeight());
		pContext->clearRtv(mpGBufferFbo->getColorTexture(3)->getRTV().get(), vec4(0));
	}
}

void DeferredRenderer::endFrame(RenderContext* pContext)
{
	pContext->popGraphicsState();
}

void DeferredRenderer::postProcess(RenderContext* pContext, Fbo::SharedPtr pTargetFbo)
{
	PROFILE("postProcess");
	MarkerScope scope(pContext, "Post Process");
	mpToneMapper->execute(pContext, mpMainFbo->getColorTexture(0), pTargetFbo);
}

void DeferredRenderer::depthPass(RenderContext* pContext)
{
	MarkerScope scope(pContext, "Depth Pass");
	PROFILE("depthPass");
	if (mEnableDepthPass == false) 
	{
		return;
	}

	mpState->setFbo(mpDepthPassFbo);
	mpState->setProgram(mDepthPass.pProgram);
	pContext->setGraphicsVars(mDepthPass.pVars);
    
	auto renderMode = mControls[EnableTransparency].enabled ? DeferredRendererSceneRenderer::Mode::Opaque : DeferredRendererSceneRenderer::Mode::All;
	mpSceneRenderer->setRenderMode(renderMode);
	mpSceneRenderer->renderScene(pContext);
}

void DeferredRenderer::lightingPass(RenderContext* pContext, Fbo* pTargetFbo)
{
	MarkerScope scope(pContext, "Ligthing pass");
	PROFILE("lightingPass");

	mLightingPass.pGBufferBlock->setTexture("Texture0", mpGBufferFbo->getColorTexture(0));
	mLightingPass.pGBufferBlock->setTexture("Texture1", mpGBufferFbo->getColorTexture(1));
	mLightingPass.pGBufferBlock->setTexture("Texture2", mpGBufferFbo->getColorTexture(2));
	mLightingPass.pGBufferBlock->setTexture("Depth", mpGBufferFbo->getDepthStencilTexture());

	if (mGBufferDebugMode != GBufferDebugMode::None)
	{
		mLightingPass.pVars["Globals"]["DebugMode"] = (uint32_t)mGBufferDebugMode;
	}
	if (mControls[ControlID::EnableShadows].enabled)
	{
		mLightingPass.pVars->setTexture("gVisibilityBuffer", mShadowPass.pVisibilityBuffer);
	}

	// Set ligth information
	auto perFrameCB = mLightingPass.pVars["PerFrame"].get();
	if (mLightingPass.LightArrayOffset != ConstantBuffer::kInvalidOffset)
	{
		for (auto i = 0u; i < mpSceneRenderer->getScene()->getLightCount(); i++)
		{
			mpSceneRenderer->getScene()->getLight(i)->setIntoProgramVars(mLightingPass.pVars.get(), perFrameCB, mLightingPass.LightArrayOffset + (i * Light::getShaderStructSize()));
		}
	}

	// Set camera information
	mpSceneRenderer->getScene()->getActiveCamera()->setIntoConstantBuffer(perFrameCB, "Camera");

	pContext->setGraphicsVars(mLightingPass.pVars);

	mLightingPass.pLightingFullscreenPass->execute(pContext);
}

void DeferredRenderer::gBufferPass(RenderContext* pContext, Fbo* pTargetFbo)
{
	MarkerScope scope(pContext, "G-Buffer pass");
	PROFILE("gBufferPass");
	mpState->setProgram(mGBufferPass.pProgram);
	mpState->setDepthStencilState(mEnableDepthPass ? mGBufferPass.pDsState : nullptr);
	pContext->setGraphicsVars(mGBufferPass.pVars);
	ConstantBuffer::SharedPtr pCB = mGBufferPass.pVars->getConstantBuffer("PerFrameCB");
	pCB["gOpacityScale"] = mOpacityScale;

	if (mControls[ControlID::EnableShadows].enabled)
	{
		pCB["camVpAtLastCsmUpdate"] = mShadowPass.camVpAtLastCsmUpdate;
		mGBufferPass.pVars->setTexture("gVisibilityBuffer", mShadowPass.pVisibilityBuffer);
	}

	if (mAAMode == AAMode::TAA)
	{
		pContext->clearFbo(mTAA.getActiveFbo().get(), vec4(0.0, 0.0, 0.0, 0.0), 1, 0, FboAttachmentType::Color);
		pCB["gRenderTargetDim"] = glm::vec2(pTargetFbo->getWidth(), pTargetFbo->getHeight());
	}

	if(mControls[EnableTransparency].enabled)
	{
		renderOpaqueObjects(pContext);
		renderTransparentObjects(pContext);
	}
	else
	{
		mpSceneRenderer->setRenderMode(DeferredRendererSceneRenderer::Mode::All);
		mpSceneRenderer->renderScene(pContext);
	}
	pContext->flush();
	mpState->setDepthStencilState(nullptr);
}

void DeferredRenderer::renderOpaqueObjects(RenderContext* pContext)
{
	mpSceneRenderer->setRenderMode(DeferredRendererSceneRenderer::Mode::Opaque);
	mpSceneRenderer->renderScene(pContext);
}

void DeferredRenderer::renderTransparentObjects(RenderContext* pContext)
{
	mpSceneRenderer->setRenderMode(DeferredRendererSceneRenderer::Mode::Transparent);
	mpState->setBlendState(mGBufferPass.pAlphaBlendBS);
	mpState->setRasterizerState(mGBufferPass.pNoCullRS);
	mpSceneRenderer->renderScene(pContext);
	mpState->setBlendState(nullptr);
	mpState->setRasterizerState(nullptr);
}

void DeferredRenderer::shadowPass(RenderContext* pContext)
{
	MarkerScope scope(pContext, "Shadow Map Pass");
	PROFILE("shadowPass");
	if (mControls[EnableShadows].enabled && mShadowPass.updateShadowMap)
	{
		mShadowPass.camVpAtLastCsmUpdate = mpSceneRenderer->getScene()->getActiveCamera()->getViewProjMatrix();
		Texture::SharedPtr pDepth = mpDepthPassFbo->getDepthStencilTexture();
		mShadowPass.pVisibilityBuffer = mShadowPass.pCsm->generateVisibilityBuffer(pContext, mpSceneRenderer->getScene()->getActiveCamera().get(), mEnableDepthPass ? pDepth : nullptr);
		pContext->flush();
	}
}

void DeferredRenderer::runTAA(RenderContext* pContext, Fbo::SharedPtr pColorFbo)
{
	if(mAAMode == AAMode::TAA)
	{
		MarkerScope scope(pContext, "TAA");
		PROFILE("runTAA");
		//  Get the Current Color and Motion Vectors
		const Texture::SharedPtr pCurColor = pColorFbo->getColorTexture(0);
		const Texture::SharedPtr pMotionVec = mpGBufferFbo->getColorTexture(3);

		//  Get the Previous Color
		const Texture::SharedPtr pPrevColor = mTAA.getInactiveFbo()->getColorTexture(0);

		//  Execute the Temporal Anti-Aliasing
		pContext->getGraphicsState()->pushFbo(mTAA.getActiveFbo());
		mTAA.pTAA->execute(pContext, pCurColor, pPrevColor, pMotionVec);
		pContext->getGraphicsState()->popFbo();

		//  Copy over the Anti-Aliased Color Texture
		pContext->blit(mTAA.getActiveFbo()->getColorTexture(0)->getSRV(0, 1), pColorFbo->getColorTexture(0)->getRTV());

		//  Swap the Fbos
		mTAA.switchFbos();
	}
}

void DeferredRenderer::runGI(RenderContext* pContext, double currentTime)
{
	PROFILE("gi");
	MarkerScope scope(pContext, "GI");
	mSSAO.pVars->setTexture("gGIMap",
		mGI.GenerateGIMap(
			pContext,
			mpRtSceneRenderer.get(),
			currentTime,
			mpSceneRenderer->getScene()->getActiveCamera().get(),
			mpGBufferFbo->getDepthStencilTexture(),
			mpGBufferFbo->getColorTexture(2),
			mpGBufferFbo->getColorTexture(0))
	);
}

void DeferredRenderer::applyAOGI(RenderContext* pContext, Fbo::SharedPtr pTargetFbo)
{
	mSSAO.pVars->setTexture("gColor", mpPostProcessFbo->getColorTexture(0));
	mSSAO.pVars["ApplyAOGIData"]["ambientValue"] = ambientValue;

	pContext->getGraphicsState()->setFbo(pTargetFbo);
	pContext->setGraphicsVars(mSSAO.pVars);

	MarkerScope scope(pContext, "Apply AO & GI");
	mSSAO.pApplySSAOPass->execute(pContext);
}

void DeferredRenderer::ambientOcclusion(RenderContext* pContext)
{
	if (mControls[EnableSSAO].enabled)
	{
		PROFILE("ssao");
		MarkerScope scope(pContext, "SSAO");
		mSSAO.pVars->setTexture("gAOMap",
			mSSAO.pSSAO->generateAOMap(
				pContext,
				mpSceneRenderer->getScene()->getActiveCamera().get(),
				mpGBufferFbo->getDepthStencilTexture(),
				mpGBufferFbo->getColorTexture(2))
		);
	}
}

void DeferredRenderer::executeFXAA(RenderContext* pContext, Fbo::SharedPtr pTargetFbo)
{
	PROFILE("fxaa");
	if(mAAMode == AAMode::FXAA)
	{
		MarkerScope scope(pContext, "FXAA");
		pContext->blit(pTargetFbo->getColorTexture(0)->getSRV(), mpMainFbo->getRenderTargetView(0));
		mpFXAA->execute(pContext, mpMainFbo->getColorTexture(0), pTargetFbo);
	}
}

void DeferredRenderer::onFrameRender(SampleCallbacks* pSample, RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
	if (mCaptureNextFrame)
	{
		StartRenderDocCapture(pSample, pRenderContext);
	}

	if (mpSceneRenderer)
	{
		Program::reloadAllPrograms();

		MarkerScope scope(pRenderContext, "Frame");
		beginFrame(pRenderContext, pTargetFbo.get(), pSample->getFrameID());
		{
			PROFILE("updateScene");
			mpSceneRenderer->update(pSample->getCurrentTime());
		}

		depthPass(pRenderContext);
		shadowPass(pRenderContext);
		mpState->setFbo(mpGBufferFbo);
		gBufferPass(pRenderContext, pTargetFbo.get());

		mpState->setFbo(mpMainFbo);
		lightingPass(pRenderContext, pTargetFbo.get());
		renderSkyBox(pRenderContext);

		if (mGBufferDebugMode != GBufferDebugMode::None)
		{
			pRenderContext->blit(mpMainFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));

			// Do not do any post process/ao/aa
			return;
		}

		runGI(pRenderContext, pSample->getCurrentTime());

		if (mControls[ControlID::VisualizeGI].enabled)
		{
			pRenderContext->blit(mSSAO.pVars->getTexture("gGIMap")->getSRV(), pTargetFbo->getRenderTargetView(0));

			// Do not do any post process/ao/aa
			return;
		}

		if (mControls[ControlID::VisualizeSurfelCoverage].enabled)
		{
			pRenderContext->blit(mGI.GetSurfelCoverageTexture()->getSRV(), pTargetFbo->getRenderTargetView(0));

			// Do not do any post process/ao/aa
			return;
		}

		if (mControls[ControlID::VisualizeIrradiance].enabled)
		{
			pRenderContext->blit(mGI.GetIrradianceTexture()->getSRV(), pTargetFbo->getRenderTargetView(0));

			// Do not do any post process/ao/aa
			return;
		}

		if (mControls[ControlID::VisualizeGIDebug].enabled)
		{
			pRenderContext->blit(mGI.GetDebugTexture()->getSRV(), pTargetFbo->getRenderTargetView(0));

			// Do not do any post process/ao/aa
			return;
		}

		Fbo::SharedPtr pPostProcessDst = mControls[EnableSSAO].enabled ? mpPostProcessFbo : pTargetFbo;
		postProcess(pRenderContext, pPostProcessDst);
		runTAA(pRenderContext, pPostProcessDst); // This will only run if we are in TAA mode
		ambientOcclusion(pRenderContext);
		applyAOGI(pRenderContext, pTargetFbo);
		executeFXAA(pRenderContext, pTargetFbo);

		endFrame(pRenderContext);
	}
	else
	{
		pRenderContext->clearFbo(pTargetFbo.get(), vec4(0.2f, 0.4f, 0.5f, 1), 1, 0);
	}

	if (mCaptureNextFrame)
	{
		mCaptureNextFrame = false;
		EndRenderDocCapture(pSample, pRenderContext);
	}
}

void DeferredRenderer::applyCameraPathState()
{
	const Scene* pScene = mpSceneRenderer->getScene().get();
	if(pScene->getPathCount())
	{
		mUseCameraPath = mUseCameraPath;
		if (mUseCameraPath)
		{
			pScene->getPath(0)->attachObject(pScene->getActiveCamera());
		}
		else
		{
			pScene->getPath(0)->detachObject(pScene->getActiveCamera());
		}
	}
}

bool DeferredRenderer::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	if (mpSceneRenderer && keyEvent.type == KeyboardEvent::Type::KeyPressed)
	{
		switch (keyEvent.key)
		{
		case KeyboardEvent::Key::Minus:
			mUseCameraPath = !mUseCameraPath;
			applyCameraPathState();
			return true;
		case KeyboardEvent::Key::O:
			mPerMaterialShader = !mPerMaterialShader;
			mpSceneRenderer->toggleStaticMaterialCompilation(mPerMaterialShader);
			return true;
		case KeyboardEvent::Key::R:
			mCaptureNextFrame = true && mpRenderDocAPI; // Set it to true only if we have loaded RenderDoc
			return true;
		}
	}

	return mpSceneRenderer ? mpSceneRenderer->onKeyEvent(keyEvent) : false;
}

void DeferredRenderer::onDroppedFile(SampleCallbacks* pSample, const std::string& filename)
{
	if (hasSuffix(filename, ".fscene", false) == false)
	{
		msgBox("You can only drop a scene file into the window");
		return;
	}
	loadScene(pSample, filename, true);
}

bool DeferredRenderer::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
	return mpSceneRenderer ? mpSceneRenderer->onMouseEvent(mouseEvent) : true;
}

void DeferredRenderer::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
	// Create the post-process FBO and AA resolve Fbo
	Fbo::Desc fboDesc;
	fboDesc.setColorTarget(0, ResourceFormat::RGBA8UnormSrgb);
	mpPostProcessFbo = FboHelper::create2D(width, height, fboDesc);

	applyAaMode(pSample);
	mShadowPass.pCsm->onResize(width, height);

	if(mpSceneRenderer)
	{
		setActiveCameraAspectRatio(width, height);
	}
}

void DeferredRenderer::applyCsSkinningMode()
{
	if(mpSceneRenderer)
	{
		SkinningCache::SharedPtr pCache = mUseCsSkinning ? SkinningCache::create() : nullptr;
		mpSceneRenderer->getScene()->attachSkinningCacheToModels(pCache);
	}    
}

void DeferredRenderer::setActiveCameraAspectRatio(uint32_t w, uint32_t h)
{
	mpSceneRenderer->getScene()->getActiveCamera()->setAspectRatio((float)w / (float)h);
}

void DeferredRenderer::StartRenderDocCapture(SampleCallbacks* pSample, RenderContext* pRenderContext)
{
	pRenderContext->flush(true);
	mpRenderDocAPI->StartFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());
}

void DeferredRenderer::EndRenderDocCapture(SampleCallbacks* pSample, RenderContext* pRenderContext)
{
	pRenderContext->flush(true);
	mpRenderDocAPI->EndFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());

	if (!mpRenderDocAPI->IsTargetControlConnected())
	{
		auto numCaptures = mpRenderDocAPI->GetNumCaptures();

		uint32_t captureFileLength = 0;
		mpRenderDocAPI->GetCapture(numCaptures - 1, nullptr, &captureFileLength, nullptr);

		std::string captureFileName;
		captureFileName.resize(captureFileLength);
		mpRenderDocAPI->GetCapture(numCaptures - 1, &captureFileName[0], &captureFileLength, nullptr);

		captureFileName.insert(captureFileName.begin(), '\"');
		captureFileName.back() = '\"'; // Change terminating null

		mpRenderDocAPI->LaunchReplayUI(true, captureFileName.c_str());
	}
}

void DeferredRenderer::LoadRenderDoc()
{
	const char* renderDocDllPath = "C:\\Program Files\\RenderDoc\\renderdoc.dll";

	auto module = ::LoadLibraryA(renderDocDllPath);
	if (!module)
	{
		return;
	}
	pRENDERDOC_GetAPI RENDERDOC_GetAPIfunc = (pRENDERDOC_GetAPI)::GetProcAddress(module, "RENDERDOC_GetAPI");
	auto ret = RENDERDOC_GetAPIfunc(eRENDERDOC_API_Version_1_1_2, (void**)&mpRenderDocAPI);
	assert(ret == 1);
	mpRenderDocAPI->SetFocusToggleKeys(nullptr, 0);
	mpRenderDocAPI->SetCaptureKeys(nullptr, 0);
	mpRenderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
	mpRenderDocAPI->UnloadCrashHandler();
}

DeferredRenderer::DeferredRenderer(bool loadRenderDoc)
{
	if (loadRenderDoc)
	{
		LoadRenderDoc();
	}
}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
#else
int main(int argc, char** argv)
#endif
{
	Falcor::addDataDirectory("../../External/Falcor/Media");
	Falcor::addDataDirectory("../../Source/Renderer/Data");
	Falcor::addDataDirectory("../../Source/GI/Data");

	Falcor::ArgList args;
	args.parseCommandLine(GetCommandLineA());
	DeferredRenderer::UniquePtr pRenderer = std::make_unique<DeferredRenderer>(args.argExists("renderdoc"));

	SampleConfig config;
	config.windowDesc.title = "Falcor Deferred Renderer";
	config.windowDesc.resizableWindow = false;
	config.windowDesc.width = 1280;
	config.windowDesc.height = 720;
#ifdef _WIN32
	Sample::run(config, pRenderer);
#else
	config.argc = (uint32_t)argc;
	config.argv = argv;
	Sample::run(config, pRenderer);
#endif
	return 0;
}
