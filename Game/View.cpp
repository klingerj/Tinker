#include "View.h"
#include "AssetManager.h"
#include "Generated/ShaderDescriptors_Reflection.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Scene.h"

void View::Update()
{
  ShaderDescriptors::AllGlobals globalData = {};
  globalData.ViewProjMatrix = m_projMatrix * m_viewMatrix;

  //TODO: write data to eventual data repository and/or push to constant buffer here
}
