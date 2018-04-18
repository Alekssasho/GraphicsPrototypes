#include "Simple.h"

#include <RenderDoc/renderdoc_app.h>

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
	if (pGui->beginGroup("Scene Controls", true))
	{
		pGui->addFloat4Var("Clear Color", m_ClearColor, 0.0f, 1.0f);
		pGui->addFloatVar("Camera Speed", m_CameraSpeed, 0.0f, 100000.0f, 1.0f);
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
	m_Camera->setDepthRange(0.01f, 1000.0f);
	m_Camera->setAspectRatio(1280.0f / 720.0f);

	m_Program = GraphicsProgram::createFromFile("Shaders/SimpleModel.slang", "", "PSmain");
	m_State = GraphicsState::create();
	m_Vars = GraphicsVars::create(m_Program->getReflector());

	m_Vars["PerFrameCB"]["gAmbient"] = vec3{ 0.2f, 0.2f, 0.2f };

	m_DirLight = DirectionalLight::create();
	m_DirLight->setWorldDirection(glm::vec3(0.13f, 0.27f, -0.9f));
	ConstantBuffer* CB = m_Vars["PerFrameCB"].get();
	m_DirLight->setIntoProgramVars(m_Vars.get(), CB, "gDirLight");

	m_CameraController.attachCamera(m_Camera);
}

void Simple::onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	StartCaptureRenderDoc(pSample, pRenderContext);
	m_CameraController.setCameraSpeed(m_CameraSpeed);
	m_CameraController.update();

	pRenderContext->clearFbo(pTargetFbo.get(), m_ClearColor, 1.0f, 0, FboAttachmentType::All);

	if (m_Scene)
	{
		m_State->setFbo(pTargetFbo);
		m_State->setProgram(m_Program);
		pRenderContext->setGraphicsState(m_State);
		pRenderContext->setGraphicsVars(m_Vars);

		m_Renderer->renderScene(pRenderContext.get());
	}
	EndCaptureRenderDoc(pSample, pRenderContext);
}

bool Simple::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	// Ctrl + R for capture with renderdoc
	if (keyEvent.key == KeyboardEvent::Key::R
		&& keyEvent.mods.isCtrlDown
		&& keyEvent.type == KeyboardEvent::Type::KeyReleased)
	{
		m_CaptureNextFrame = true;
		return true;
	}
	else
	{
		return m_CameraController.onKeyEvent(keyEvent);
	}
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

int main()
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

	Simple::UniquePtr pRenderer = std::make_unique<Simple>();
	Falcor::SampleConfig config;
	config.windowDesc.title = "Simple Window";
	config.windowDesc.resizableWindow = false;
	config.windowDesc.width = 1280;
	config.windowDesc.height = 720;
	Falcor::Sample::run(config, pRenderer);
}