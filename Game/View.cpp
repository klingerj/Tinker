#include "View.h"
#include "AssetManager.h"
#include "DataRepository.h"
#include "Generated/ShaderDescriptors_Reflection.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Scene.h"

void View::Update()
{
  alignas(16) m4f viewProj = m_projMatrix * m_viewMatrix;
  DataRepo::g_theDataRepository.ShaderConstants_Globals.ViewProjMatrix = viewProj;
}
