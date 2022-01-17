#include "CoreDefines.h"

namespace Tk
{
namespace Core
{

namespace Graphics
{
struct GraphicsCommandStream;
}

namespace Asset
{

TINKER_API void LoadAllAssets(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream);

struct AssetLibrary;
TINKER_API void AddStreamedAssetsToAssetLibrary(AssetLibrary* AssetLib, uint32* NumAssetsStreamed);

}
}
}