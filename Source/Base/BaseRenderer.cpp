#include "BaseRenderer.h"

//#include <pix3.h>

void BaseRenderer::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
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

	OnGUI(pSample, pGui);
}

void BaseRenderer::LoadModel()
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
		m_Camera->setPosition(model->getRadius() * 5.0f * vec3{ 0.0f, 0.0f, 1.0f } +model->getCenter());

		m_CameraSpeed = model->getRadius() * 0.25f;
	}
}

void BaseRenderer::LoadScene()
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

void BaseRenderer::onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	m_Camera = Camera::create();
	m_Camera->setUpVector({ 0.0f, 1.0f, 0.0f });
	m_Camera->setDepthRange(0.01f, 10000.0f);
	m_Camera->setAspectRatio(1280.0f / 720.0f);

	m_CameraController.attachCamera(m_Camera);

	OnLoad(pSample, pRenderContext);
}

void BaseRenderer::onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
	//StartCaptureRenderDoc(pSample, pRenderContext);
	//PIXBeginEvent(pRenderContext->getLowLevelData()->getCommandList().GetInterfacePtr(), PIX_COLOR(255, 0, 0), "Frame");
	m_CameraController.setCameraSpeed(m_CameraSpeed);
	m_CameraController.update();

	pRenderContext->clearFbo(pTargetFbo.get(), m_ClearColor, 1.0f, 0, FboAttachmentType::All);

	if (m_Scene)
	{
		Program::reloadAllPrograms();

		OnRender(pSample, pRenderContext, pTargetFbo);
	}

	//PIXEndEvent(pRenderContext->getLowLevelData()->getCommandList().GetInterfacePtr());
	//EndCaptureRenderDoc(pSample, pRenderContext);
}

bool BaseRenderer::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	// Ctrl + R for capture with renderdoc
	if (keyEvent.key == KeyboardEvent::Key::R
		&& keyEvent.mods.isCtrlDown
		&& keyEvent.type == KeyboardEvent::Type::KeyReleased)
		//&& renderDocAPI) // check if we have loaded renderdoc
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

bool BaseRenderer::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
	return m_CameraController.onMouseEvent(mouseEvent);
}

void BaseRenderer::StartCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	//if (m_CaptureNextFrame)
	//{
	//	pRenderContext->flush(true);
	//	renderDocAPI->StartFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());
	//}
}

void BaseRenderer::EndCaptureRenderDoc(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
	//if (m_CaptureNextFrame)
	//{
	//	pRenderContext->flush(true);
	//	renderDocAPI->EndFrameCapture((ID3D12Device*)gpDevice->getApiHandle(), (HWND)pSample->getWindow()->getApiHandle());

	//	if (!renderDocAPI->IsTargetControlConnected())
	//	{
	//		auto numCaptures = renderDocAPI->GetNumCaptures();
	//		uint32_t captureFileLength = 0;
	//		renderDocAPI->GetCapture(numCaptures - 1, nullptr, &captureFileLength, nullptr);
	//		std::string captureFileName;
	//		captureFileName.resize(captureFileLength);
	//		renderDocAPI->GetCapture(numCaptures - 1, &captureFileName[0], &captureFileLength, nullptr);
	//		captureFileName.insert(captureFileName.begin(), '\"');
	//		captureFileName.back() = '\"'; // Change terminating null

	//		renderDocAPI->LaunchReplayUI(true, captureFileName.c_str());
	//	}

	//	m_CaptureNextFrame = false;
	//}
}