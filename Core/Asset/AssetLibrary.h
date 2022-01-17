#include "CoreDefines.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "DataStructures/HashMap.h"

namespace Tk
{
namespace Core
{
namespace Asset
{

struct AssetLibrary
{
	Tk::Core::HashMap<uint32, Tk::Core::Graphics::StaticMeshData*, Hash32> m_MeshLib;
	//TODO: add texture lib

	enum
	{
		eMeshLibSize = 64,
		eTextureLibSize = 128,
	};

	void Init()
	{
		m_MeshLib.Reserve(eMeshLibSize);
	}

	Tk::Core::Graphics::StaticMeshData* FindMesh(uint32 MeshID)
	{
		uint32 index = m_MeshLib.FindIndex(MeshID);
		if (index == m_MeshLib.eInvalidIndex)
			return nullptr;
		else
			return m_MeshLib.DataAtIndex(index);
	}
};

}
}
}
