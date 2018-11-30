#include "DeferredRenderer.h"

const Gui::DropdownList aaModeList =
{
	{ 0, "None"},
	{ 1, "TAA" },
	{ 2, "FXAA" }
};

const Gui::DropdownList gBufferDebugModeList =
{
	{ 0, "None"},
	{ 1, "Diffuse" },
	{ 2, "Specular" },
	{ 3, "Normal" },
	{ 4, "Roughness" },
	{ 5, "Depth" },
};

void DeferredRenderer::initControls()
{
	mControls.resize(ControlID::Count);
	mControls[ControlID::SuperSampling] = { false, false, "INTERPOLATION_MODE", "sample" };
	mControls[ControlID::EnableShadows] = { true, false, "_ENABLE_SHADOWS" };
	mControls[ControlID::EnableReflections] = { false, false, "_ENABLE_REFLECTIONS" };
	mControls[ControlID::EnableHashedAlpha] = { true, true, "_DEFAULT_ALPHA_TEST" };
	mControls[ControlID::EnableTransparency] = { false, false, "_ENABLE_TRANSPARENCY" };
	mControls[ControlID::EnableSSAO] = { true, false, "" };
	mControls[ControlID::VisualizeCascades] = { false, false, "_VISUALIZE_CASCADES" };
	mControls[ControlID::VisualizeAO] = { false, false, "VISUALIZE_AO_MAP" };
	mControls[ControlID::VisualizeSurfelCoverage] = { false, false, "" };

	for (uint32_t i = 0; i < ControlID::Count; i++)
	{
		applyLightingProgramControl((ControlID)i);
	}
}

void DeferredRenderer::applyLightingProgramControl(ControlID controlId)
{
	const ProgramControl control = mControls[controlId];
	if (control.define.size())
	{
		bool add = control.unsetOnEnabled ? !control.enabled : control.enabled;
		if (add)
		{
			mGBufferPass.pProgram->addDefine(control.define, control.value);
			mLightingPass.pLightingFullscreenPass->getProgram()->addDefine(control.define, control.value);
			if (controlId == ControlID::EnableHashedAlpha) mDepthPass.pProgram->addDefine(control.define, control.value);
		}
		else
		{
			mGBufferPass.pProgram->removeDefine(control.define);
			mLightingPass.pLightingFullscreenPass->getProgram()->removeDefine(control.define);
			if (controlId == ControlID::EnableHashedAlpha) mDepthPass.pProgram->removeDefine(control.define);
		}
	}
}

void DeferredRenderer::createTaaPatternGenerator(uint32_t fboWidth, uint32_t fboHeight)
{
	PatternGenerator::SharedPtr pGenerator;
	switch (mTAASamplePattern)
	{
	case SamplePattern::Halton:
		pGenerator = HaltonSamplePattern::create();
		break;
	case SamplePattern::DX11:
		pGenerator = DxSamplePattern::create();
		break;
	default:
		should_not_get_here();
		pGenerator = nullptr;
	}

	mpSceneRenderer->getScene()->getActiveCamera()->setPatternGenerator(pGenerator, 1.0f / vec2(fboWidth, fboHeight));
}

void DeferredRenderer::applyAaMode(SampleCallbacks* pSample)
{
	if (mGBufferPass.pProgram == nullptr) return;

	uint32_t w = pSample->getCurrentFbo()->getWidth();
	uint32_t h = pSample->getCurrentFbo()->getHeight();

	// Common FBO desc (2 color outputs - color and normal)
	Fbo::Desc fboDesc;
	fboDesc.setColorTarget(0, ResourceFormat::RGBA8Unorm);
	fboDesc.setColorTarget(1, ResourceFormat::RGBA8Unorm);
	fboDesc.setColorTarget(2, ResourceFormat::RGBA8Unorm);
	fboDesc.setDepthStencilTarget(ResourceFormat::D32Float);

	// Release the TAA FBOs
	mTAA.resetFbos();

	if (mAAMode == AAMode::TAA)
	{
		mGBufferPass.pProgram->removeDefine("INTERPOLATION_MODE");
		mGBufferPass.pProgram->addDefine("_OUTPUT_MOTION_VECTORS");
		fboDesc.setColorTarget(3, ResourceFormat::RG16Float);

		Fbo::Desc taaFboDesc;
		taaFboDesc.setColorTarget(0, ResourceFormat::RGBA8UnormSrgb);
		mTAA.createFbos(w, h, taaFboDesc);
		createTaaPatternGenerator(w, h);
	}
	else
	{
		mGBufferPass.pProgram->removeDefine("_OUTPUT_MOTION_VECTORS");
		applyLightingProgramControl(SuperSampling);
		fboDesc.setSampleCount(1);
		// Disable jitter
		mpSceneRenderer->getScene()->getActiveCamera()->setPatternGenerator(nullptr);
	}

	mpGBufferFbo = FboHelper::create2D(w, h, fboDesc);
	mpDepthPassFbo = Fbo::create();
	mpDepthPassFbo->attachDepthStencilTarget(mpGBufferFbo->getDepthStencilTexture());

	{
		Fbo::Desc mainDesc;
		mainDesc.setColorTarget(0, ResourceFormat::RGBA32Float);
		mpMainFbo = FboHelper::create2D(w, h, mainDesc);
		mpMainFbo->attachDepthStencilTarget(mpGBufferFbo->getDepthStencilTexture());
	}
}

void DeferredRenderer::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
	static const FileDialogFilterVec kImageFilesFilter = { {"bmp"}, {"jpg"}, {"dds"}, {"png"}, {"tiff"}, {"tif"}, {"tga"} };
	if (pGui->addButton("Load Model"))
	{
		std::string filename;
		if (openFileDialog(Model::kFileExtensionFilters, filename))
		{
			loadModel(pSample, filename, true);
		}
	}

	if (pGui->addButton("Load Scene"))
	{
		std::string filename;
		if (openFileDialog(Scene::kFileExtensionFilters, filename))
		{
			loadScene(pSample, filename, true);
		}
	}

	if (mpSceneRenderer)
	{
		if (pGui->addButton("Load SkyBox Texture"))
		{
			std::string filename;
			if (openFileDialog(kImageFilesFilter, filename))
			{
				initSkyBox(createTextureFromFile(filename, false, true));
			}
		}

		if (pGui->beginGroup("Scene Settings"))
		{
			Scene* pScene = mpSceneRenderer->getScene().get();
			float camSpeed = pScene->getCameraSpeed();
			if (pGui->addFloatVar("Camera Speed", camSpeed))
			{
				pScene->setCameraSpeed(camSpeed);
			}

			vec2 depthRange(pScene->getActiveCamera()->getNearPlane(), pScene->getActiveCamera()->getFarPlane());
			if (pGui->addFloat2Var("Depth Range", depthRange, 0, FLT_MAX))
			{
				pScene->getActiveCamera()->setDepthRange(depthRange.x, depthRange.y);
			}

			if (pScene->getPathCount() > 0)
			{
				if (pGui->addCheckBox("Camera Path", mUseCameraPath))
				{
					applyCameraPathState();
				}
			}

			if (pScene->getLightCount() && pGui->beginGroup("Light Sources"))
			{
				for (uint32_t i = 0; i < pScene->getLightCount(); i++)
				{
					Light* pLight = pScene->getLight(i).get();
					pLight->renderUI(pGui, pLight->getName().c_str());
				}
				pGui->endGroup();
			}

			if (pGui->addCheckBox("Use CS for Skinning", mUseCsSkinning))
			{
				applyCsSkinningMode();
			}
			pGui->endGroup();
		}

		mGI.RenderUI(pGui);
		if (pGui->beginGroup("Debug GI"))
		{
			pGui->addCheckBox("Visualize Surfel Coverage", mControls[ControlID::VisualizeSurfelCoverage].enabled);
			pGui->endGroup();
		}

		if (pGui->beginGroup("Renderer Settings"))
		{
			pGui->addCheckBox("Depth Pass", mEnableDepthPass);
			pGui->addTooltip("Run a depth-pass at the beginning of the frame");

			if (pGui->addCheckBox("Specialize Material Shaders", mPerMaterialShader))
			{
				mpSceneRenderer->toggleStaticMaterialCompilation(mPerMaterialShader);
			}
			pGui->addTooltip("Create a specialized version of the lighting program for each material in the scene");

			uint32_t maxAniso = mpSceneSampler->getMaxAnisotropy();
			if (pGui->addIntVar("Max Anisotropy", (int&)maxAniso, 1, 16))
			{
				setSceneSampler(maxAniso);
			}

			pGui->endGroup();
		}

		if (pGui->beginGroup("G-Buffer"))
		{
			if (pGui->addDropdown("Debug Mode", gBufferDebugModeList, (uint32_t&)mGBufferDebugMode))
			{
				if (mGBufferDebugMode == GBufferDebugMode::None)
				{
					mLightingPass.pLightingFullscreenPass->getProgram()->removeDefine("DEBUG_MODE");
				}
				else
				{
					mLightingPass.pLightingFullscreenPass->getProgram()->addDefine("DEBUG_MODE");
				}
			}
			pGui->endGroup();
		}

		//  Anti-Aliasing Controls.
		if (pGui->beginGroup("Anti-Aliasing"))
		{
			bool reapply = false;
			reapply = reapply || pGui->addDropdown("AA Mode", aaModeList, (uint32_t&)mAAMode);
            
			//  Temporal Anti-Aliasing.
			if (mAAMode == AAMode::TAA)
			{
				if (pGui->beginGroup("TAA"))
				{
					//  Render the TAA UI.
					mTAA.pTAA->renderUI(pGui);

					//  Choose the Sample Pattern for TAA.
					Gui::DropdownList samplePatternList;
					samplePatternList.push_back({ (uint32_t)SamplePattern::Halton, "Halton" });
					samplePatternList.push_back({ (uint32_t)SamplePattern::DX11, "DX11" });
					pGui->addDropdown("Sample Pattern", samplePatternList, (uint32_t&)mTAASamplePattern);

					// Disable super-sampling
					pGui->endGroup();
				}
			}

			if (mAAMode == AAMode::FXAA)
			{
				mpFXAA->renderUI(pGui, "FXAA");
			}

			if (reapply) applyAaMode(pSample);

			pGui->endGroup();
		}

		if (pGui->beginGroup("Light Probes"))
		{
			if (pGui->addButton("Add/Change Light Probe"))
			{
				std::string filename;
				if (openFileDialog(kImageFilesFilter, filename))
				{
					updateLightProbe(LightProbe::create(pSample->getRenderContext(), filename, true, ResourceFormat::RGBA16Float));
				}
			}

			Scene::SharedPtr pScene = mpSceneRenderer->getScene();
			if (pScene->getLightProbeCount() > 0)
			{
				if (pGui->addCheckBox("Enable", mControls[ControlID::EnableReflections].enabled))
				{
					applyLightingProgramControl(ControlID::EnableReflections);
				}
				if (mControls[ControlID::EnableReflections].enabled)
				{
					pGui->addSeparator();
					pScene->getLightProbe(0)->renderUI(pGui);
				}
			}

			pGui->endGroup();
		}

		mpToneMapper->renderUI(pGui, "Tone-Mapping");

		if (pGui->beginGroup("Shadows"))
		{
			if (pGui->addCheckBox("Enable Shadows", mControls[ControlID::EnableShadows].enabled))
			{
				applyLightingProgramControl(ControlID::EnableShadows);
			}
			if (mControls[ControlID::EnableShadows].enabled)
			{
				pGui->addCheckBox("Update Map", mShadowPass.updateShadowMap);
				mShadowPass.pCsm->renderUI(pGui);
				if (pGui->addCheckBox("Visualize Cascades", mControls[ControlID::VisualizeCascades].enabled))
				{
					applyLightingProgramControl(ControlID::VisualizeCascades);
					mShadowPass.pCsm->toggleCascadeVisualization(mControls[ControlID::VisualizeCascades].enabled);
				}
			}
			pGui->endGroup();
		}

		if (pGui->beginGroup("SSAO"))
		{
			if (pGui->addCheckBox("Enable SSAO", mControls[ControlID::EnableSSAO].enabled))
			{
				applyLightingProgramControl(ControlID::EnableSSAO);
			}

			if (mControls[ControlID::EnableSSAO].enabled)
			{
				if (pGui->addCheckBox("Visualize AO", mControls[ControlID::VisualizeAO].enabled))
				{
					applyLightingProgramControl(ControlID::VisualizeAO);
					if (mControls[ControlID::VisualizeAO].enabled)
					{
						mSSAO.pApplySSAOPass->getProgram()->addDefine(mControls[ControlID::VisualizeAO].define, mControls[ControlID::VisualizeAO].value);
					}
					else
					{
						mSSAO.pApplySSAOPass->getProgram()->removeDefine(mControls[ControlID::VisualizeAO].define);
					}
				}
				mSSAO.pSSAO->renderUI(pGui);
			}
			pGui->endGroup();
		}

		if (pGui->beginGroup("Transparency"))
		{
			if (pGui->addCheckBox("Enable Transparency", mControls[ControlID::EnableTransparency].enabled))
			{
				applyLightingProgramControl(ControlID::EnableTransparency);
			}
			pGui->addFloatVar("Opacity Scale", mOpacityScale, 0, 1);
			pGui->endGroup();
		}

		if (pGui->addCheckBox("Hashed-Alpha Test", mControls[ControlID::EnableHashedAlpha].enabled))
		{
			applyLightingProgramControl(ControlID::EnableHashedAlpha);
		}
	}
}
