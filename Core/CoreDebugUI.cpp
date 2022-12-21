#include "CoreDebugUI.h"
#include "CoreDefines.h"

#ifdef VULKAN
#include "backends/imgui_impl_vulkan.h"
#else
// TODO: D3D12
#endif

#include "imgui.h"

namespace Tk
{
namespace Core
{
namespace DebugUI
{
    void Init()
    {
        #ifdef VULKAN
        ImGui_ImplVulkan_InitInfo info = {};
        // TODO: init the struct
        ImGui_ImplVulkan_Init(&info, 0 /*&imguiRenderPass TODO create the render pass object */);
        #else
        // TODO: D3D12
        #endif
    }

    void Shutdown()
    {
        #ifdef VULKAN
        ImGui_ImplVulkan_Shutdown();
        #else
        // TODO: D3D12
        #endif
    }

    void NewFrame()
    {
        #ifdef VULKAN
        ImGui_ImplVulkan_NewFrame();
        #else
        // TODO: D3D12
        #endif
    }

    void Render()
    {
        #ifdef VULKAN
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), 0/* TODO get commandbuffer somehow */);
        #else
        // TODO: D3D12
        #endif
    }

    void UI_RenderPassStats()
    {
        const char* names[3] = { "ZPrepass", "MainView", "PostGraph" };
        float timings[3] = { 0.012f, 0.53f, 1.7f };

        if (ImGui::Begin("Render Pass Timings"))
        {
            if (ImGui::BeginMenuBar())
            {
                for (uint32 i = 0; i < 3; ++i)
                {
                    ImGui::Text("%s: %.2f\n", names[i], timings[i]);
                }
            }

            ImGui::End();
        }
    }
}
}
}
