#include "DeferredRendererSceneRenderer.h"

static bool isMaterialTransparent(const Material* pMaterial)
{
	return pMaterial->getBaseColor().a < 1.0f;
}

DeferredRendererSceneRenderer::DeferredRendererSceneRenderer(const Scene::SharedPtr& pScene) : SceneRenderer(pScene)
{
	for (uint32_t model = 0; model < mpScene->getModelCount(); model++)
	{
		const auto& pModel = mpScene->getModel(model);
		for (uint32_t mesh = 0; mesh < pModel->getMeshCount(); mesh++)
		{
			const auto& pMesh = pModel->getMesh(mesh);
			uint32_t id = pMesh->getId();
			if (mTransparentMeshes.size() <= id) mTransparentMeshes.resize(id + 1);
			bool transparent = isMaterialTransparent(pMesh->getMaterial().get());
			mHasOpaqueObjects = mHasOpaqueObjects || (transparent == false);
			mHasTransparentObject = mHasTransparentObject || transparent;
			mTransparentMeshes[id] = transparent;
		}
	}

	RasterizerState::Desc rsDesc;
	mpDefaultRS = RasterizerState::create(rsDesc);
	rsDesc.setCullMode(RasterizerState::CullMode::None);
	mpNoCullRS = RasterizerState::create(rsDesc);
}

DeferredRendererSceneRenderer::SharedPtr DeferredRendererSceneRenderer::create(const Scene::SharedPtr& pScene)
{
	return SharedPtr(new DeferredRendererSceneRenderer(pScene));
}

bool DeferredRendererSceneRenderer::setPerMeshData(const CurrentWorkingData& currentData, const Mesh* pMesh)
{
	switch (mRenderMode)
	{
	case Mode::All:
		return true;
	case Mode::Opaque:
		return mTransparentMeshes[pMesh->getId()] == false;
	case Mode::Transparent:
		return mTransparentMeshes[pMesh->getId()];
	default:
		should_not_get_here();
		return false;
	}
}

void DeferredRendererSceneRenderer::renderScene(RenderContext* pContext)
{
	switch (mRenderMode)
	{
	case Mode::Opaque:
		if (mHasOpaqueObjects == false) return;
		break;
	case Mode::Transparent:
		if (mHasTransparentObject == false) return;
	}
	SceneRenderer::renderScene(pContext);
}

RasterizerState::SharedPtr DeferredRendererSceneRenderer::getRasterizerState(const Material* pMaterial)
{
	if (pMaterial->getAlphaMode() == AlphaModeMask)
	{
		return mpNoCullRS;
	}
	else
	{
		return mpDefaultRS;
	}
}

bool DeferredRendererSceneRenderer::setPerMaterialData(const CurrentWorkingData& currentData, const Material* pMaterial)
{
	const auto& pRsState = getRasterizerState(currentData.pMaterial);
	if (pRsState != mpLastSetRs)
	{
		currentData.pContext->getGraphicsState()->setRasterizerState(pRsState);
		mpLastSetRs = pRsState;
	}

	return SceneRenderer::setPerMaterialData(currentData, pMaterial);
}