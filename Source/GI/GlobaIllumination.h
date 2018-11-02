#pragma once

#include <Falcor.h>

using namespace Falcor;

class GlobalIllumination
{
public:
	void Initilize();
	void RenderUI(Gui* pGui);
private:
	void ResetGI();

	StructuredBuffer::SharedPtr m_Surfels;
	int32_t m_MaxSurfels;
};