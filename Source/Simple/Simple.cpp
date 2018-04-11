#include "Simple.h"

#include <RenderDoc/renderdoc_app.h>

RENDERDOC_API_1_1_2* renderDocAPI = nullptr;

void Simple::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
	if (pGui->beginGroup("Scene Controls", true))
	{
		pGui->addFloat4Var("Clear Color", m_ClearColor, 0.0f, 1.0f);
		pGui->endGroup();
	}
}

void Simple::onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	m_Model = Model::createFromFile("Vampire/Vampire_A_Lusth.dae");
	if (!m_Model)
	{
		pSample->shutdown();
	}
	m_Camera = Camera::create();
	m_Camera->setTarget(m_Model->getCenter());
	m_Camera->setUpVector({ 0.0f, 1.0f, 0.0f });
	m_Camera->setPosition(m_Model->getRadius() * 5.0f * vec3{ 0.0f, 0.0f, 1.0f } + m_Model->getCenter());
	m_Camera->setDepthRange(0.001f, m_Model->getRadius() * 10.0f);
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
	if (m_CaptureNextFrame)
	{
		pRenderContext->flush(true);
		renderDocAPI->StartFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());
	}
	m_CameraController.update();

	pRenderContext->clearFbo(pTargetFbo.get(), m_ClearColor, 1.0f, 0, FboAttachmentType::All);

	m_State->setFbo(pTargetFbo);
	m_State->setProgram(m_Program);
	pRenderContext->setGraphicsState(m_State);
	pRenderContext->setGraphicsVars(m_Vars);
	ModelRenderer::render(pRenderContext.get(), m_Model, m_Camera.get(), false);
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

bool Simple::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	// Ctrl + R for capture with renderdoc
	if (keyEvent.key == KeyboardEvent::Key::R
		&& keyEvent.mods.isCtrlDown)
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

int main()
{
	// Try loading RenderDoc API
	const char* renderDocDllPath = "C:\\Program Files\\RenderDoc\\renderdoc.dll";
#ifdef _DEBUG
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
#endif


	Simple::UniquePtr pRenderer = std::make_unique<Simple>();
	Falcor::SampleConfig config;
	config.windowDesc.title = "Simple Window";
	config.windowDesc.resizableWindow = false;
	config.windowDesc.width = 1280;
	config.windowDesc.height = 720;
	Falcor::Sample::run(config, pRenderer);
}