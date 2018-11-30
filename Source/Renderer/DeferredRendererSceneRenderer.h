#pragma once
#include "Falcor.h"

using namespace Falcor;

class DeferredRendererSceneRenderer : public SceneRenderer
{
public:
	using SharedPtr = std::shared_ptr<DeferredRendererSceneRenderer>;
	~DeferredRendererSceneRenderer() = default;
	enum class Mode
	{
		All,
		Opaque,
		Transparent
	};

	static SharedPtr create(const Scene::SharedPtr& pScene);
	void setRenderMode(Mode renderMode) { mRenderMode = renderMode; }
	void renderScene(RenderContext* pContext) override;
private:
	bool setPerMeshData(const CurrentWorkingData& currentData, const Mesh* pMesh) override;
	bool setPerMaterialData(const CurrentWorkingData& currentData, const Material* pMaterial) override;
	RasterizerState::SharedPtr getRasterizerState(const Material* pMaterial);
	DeferredRendererSceneRenderer(const Scene::SharedPtr& pScene);
	std::vector<bool> mTransparentMeshes;
	Mode mRenderMode = Mode::All;
	bool mHasOpaqueObjects = false;
	bool mHasTransparentObject = false;

	RasterizerState::SharedPtr mpDefaultRS;
	RasterizerState::SharedPtr mpNoCullRS;
	RasterizerState::SharedPtr mpLastSetRs;
};
