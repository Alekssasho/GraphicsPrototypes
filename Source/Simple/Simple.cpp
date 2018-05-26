#include "Simple.h"

#include <RenderDoc/renderdoc_app.h>
#include <pix3.h>

RENDERDOC_API_1_1_2* renderDocAPI = nullptr;

void Simple::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
	if (pGui->beginGroup("Load Data", true))
	{
		if (pGui->addButton("Load Model"))
		{
			LoadModel();
		}

		if (pGui->addButton("Load Scene", true))
		{
			LoadScene();
		}

		pGui->endGroup();
	}
	if (pGui->beginGroup("App Controls", true))
	{
		pGui->addFloat4Var("Clear Color", m_ClearColor, 0.0f, 1.0f);
		pGui->addFloatVar("Camera Speed", m_CameraSpeed, 0.0f, 100000.0f, 0.1f);
		pGui->endGroup();
	}

	if (pGui->beginGroup("Scene Controls", true))
	{
		pGui->addFloatVar("Ambient", m_Ambient, 0.0f, 1.0f);
		if (pGui->addFloat3Var("Dir Light", dirLight, -1.0f, 1.0f))
		{
			dirLight = glm::normalize(dirLight);
		}
		m_DirLightShadowMap.OnGui(pGui);
		pGui->endGroup();
	}
}

void Simple::LoadModel()
{
	std::string filename;
	if (openFileDialog(Model::kSupportedFileFormatsStr, filename))
	{
		auto model = Model::createFromFile(filename.c_str());
		if (!model)
		{
			return;
		}
		m_Scene = Scene::create();
		m_Scene->addModelInstance(model, "");
		m_Scene->addCamera(m_Camera);
		m_Scene->setActiveCamera(0);

		m_Renderer = SceneRenderer::create(m_Scene);
		m_Renderer->toggleMeshCulling(false);

		m_Camera->setTarget(model->getCenter());
		m_Camera->setPosition(model->getRadius() * 5.0f * vec3{ 0.0f, 0.0f, 1.0f } + model->getCenter());

		m_CameraSpeed = model->getRadius() * 0.25f;
	}
}

void Simple::LoadScene()
{
	std::string filename;
	if (openFileDialog(Scene::kFileFormatString, filename))
	{
		auto scene = Scene::loadFromFile(filename);
		if (!scene)
		{
			return;
		}

		m_Scene = scene;
		auto cameraInd = m_Scene->addCamera(m_Camera);
		m_Scene->setActiveCamera(cameraInd);

		m_Renderer = SceneRenderer::create(m_Scene);
		m_Renderer->toggleMeshCulling(false);

		m_Camera->setPosition({ 0.0f, 0.0f, 0.0f });
		m_Camera->setTarget({ 0.0f, 0.0f, 1.0f });
	}
}

void Simple::onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	m_Camera = Camera::create();
	m_Camera->setUpVector({ 0.0f, 1.0f, 0.0f });
	m_Camera->setDepthRange(0.01f, 10000.0f);
	m_Camera->setAspectRatio(1280.0f / 720.0f);

	m_CameraController.attachCamera(m_Camera);

	m_State = GraphicsState::create();
	m_Program = GraphicsProgram::createFromFile("Shaders/SimpleModel.slang", "", "PSmain");
	m_Vars = GraphicsVars::create(m_Program->getReflector());

	m_DirLightShadowMap.Initialize();
}

void Simple::onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	StartCaptureRenderDoc(pSample, pRenderContext);
	PIXBeginEvent(pRenderContext->getLowLevelData()->getCommandList().GetInterfacePtr(), PIX_COLOR(255, 0, 0), "Frame");
	m_CameraController.setCameraSpeed(m_CameraSpeed);
	m_CameraController.update();

	pRenderContext->clearFbo(pTargetFbo.get(), m_ClearColor, 1.0f, 0, FboAttachmentType::All);

	if (m_Scene)
	{
		Program::reloadAllPrograms();

		// Check if we have dir light and create shadow map for it
		for (const auto& light : m_Scene->getLights())
		{
			if (light->getType() == LightDirectional)
			{
				auto dirl = std::static_pointer_cast<DirectionalLight>(light);
				dirl->setWorldDirection(dirLight);
				m_DirLightShadowMap.Render(pRenderContext, m_Renderer, dirl);
			}
		}

		m_State->setFbo(pTargetFbo);
		m_State->setProgram(m_Program);
		m_Vars["PerFrameCB"]["gAmbient"] = glm::vec3{ m_Ambient, m_Ambient, m_Ambient };
		m_Vars["PerFrameCB"]["gLightViewMatrix"] = m_DirLightShadowMap.GetLightProjectionMatrix();
		m_Vars->setTexture("gShadowMap", m_DirLightShadowMap.GetShadowMapTexture());
		m_Vars->setSampler("gShadowMapSampler", m_DirLightShadowMap.GetShadowMapSampler());
		pRenderContext->setGraphicsState(m_State);
		pRenderContext->setGraphicsVars(m_Vars);

		m_Renderer->renderScene(pRenderContext.get());

		m_DirLightShadowMap.DebugDraw(pRenderContext, pTargetFbo);
	}
	PIXEndEvent(pRenderContext->getLowLevelData()->getCommandList().GetInterfacePtr());
	EndCaptureRenderDoc(pSample, pRenderContext);
}

bool Simple::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	// Ctrl + R for capture with renderdoc
	if (keyEvent.key == KeyboardEvent::Key::R
		&& keyEvent.mods.isCtrlDown
		&& keyEvent.type == KeyboardEvent::Type::KeyReleased
		&& renderDocAPI) // check if we have loaded renderdoc
	{
		m_CaptureNextFrame = true;
		return true;
	}
	else
	{
		return m_CameraController.onKeyEvent(keyEvent);
	}
	return false;
}

bool Simple::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
	return m_CameraController.onMouseEvent(mouseEvent);
}

void Simple::StartCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	if (m_CaptureNextFrame)
	{
		pRenderContext->flush(true);
		renderDocAPI->StartFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());
	}
}

void Simple::EndCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	if (m_CaptureNextFrame)
	{
		pRenderContext->flush(true);
		renderDocAPI->EndFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());

		if (!renderDocAPI->IsTargetControlConnected())
		{
			auto numCaptures = renderDocAPI->GetNumCaptures();
			uint32_t captureFileLength = 0;
			renderDocAPI->GetCapture(numCaptures - 1, nullptr, &captureFileLength, nullptr);
			std::string captureFileName;
			captureFileName.resize(captureFileLength);
			renderDocAPI->GetCapture(numCaptures - 1, &captureFileName[0], &captureFileLength, nullptr);
			captureFileName.insert(captureFileName.begin(), '\"');
			captureFileName.back() = '\"'; // Change terminating null

			renderDocAPI->LaunchReplayUI(true, captureFileName.c_str());
		}

		m_CaptureNextFrame = false;
	}
}

int main(int argc, char** argv)
{
	bool loadRenderDoc = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--renderdoc") == 0)
		{
			loadRenderDoc = true;
		}
	}

	if (loadRenderDoc)
	{
		// Try loading RenderDoc API
		const char* renderDocDllPath = "C:\\Program Files\\RenderDoc\\renderdoc.dll";
		auto module = ::LoadLibraryA(renderDocDllPath);
		if (!module)
		{
			return -1;
		}
		pRENDERDOC_GetAPI RENDERDOC_GetAPIfunc = (pRENDERDOC_GetAPI)::GetProcAddress(module, "RENDERDOC_GetAPI");
		auto ret = RENDERDOC_GetAPIfunc(eRENDERDOC_API_Version_1_1_2, (void**)&renderDocAPI);
		assert(ret == 1);

		renderDocAPI->SetFocusToggleKeys(nullptr, 0);
		renderDocAPI->SetCaptureKeys(nullptr, 0);
		renderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
		renderDocAPI->UnloadCrashHandler();
	}


	Simple::UniquePtr pRenderer = std::make_unique<Simple>();
	Falcor::SampleConfig config;
	config.windowDesc.title = "Simple Window";
	config.windowDesc.resizableWindow = false;
	config.windowDesc.width = 1280;
	config.windowDesc.height = 720;
	config.argc = argc;
	config.argv = argv;
	Falcor::Sample::run(config, pRenderer);
}