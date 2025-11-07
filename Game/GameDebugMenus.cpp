#include "GameDebugMenus.h"
#include "DataStructures/HashMap.h"
#include "DataStructures/Vector.h"
#include "Graphics/Common/GPUTimestamps.h"
#include "Hashing.h"
#include "Sorting.h"
#include "StringTypes.h"
#include "ThirdParty/imgui-docking/imgui.h"
#include "Utility/MemTracker.h"
#include <stdlib.h>

using namespace Tk;
using namespace Graphics;

static bool g_enable = false;
static bool mainMenu_SelectedOverview = false;
static bool mainMenu_SelectedRPTimings = false;
static bool mainMenu_SelectedMemTracker = false;

void ToggleEnable()
{
  g_enable = !g_enable;
}

void Menu_MainMenu()
{
  if (!g_enable)
  {
    mainMenu_SelectedOverview = false;
    mainMenu_SelectedRPTimings = false;
    mainMenu_SelectedMemTracker = false;
    return;
  }

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Performance"))
    {
      if (ImGui::MenuItem("Overview", NULL, &mainMenu_SelectedOverview))
      {
      }
      if (ImGui::MenuItem("GPU Render Pass Timings", NULL, &mainMenu_SelectedRPTimings))
      {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Memory"))
    {
      if (ImGui::MenuItem("Alloc Tracker", NULL, &mainMenu_SelectedMemTracker))
      {
      }
      ImGui::EndMenu();
    }
  }
  ImGui::EndMainMenuBar();
}

void Menu_PerformanceOverview()
{
  if (mainMenu_SelectedOverview)
  {
    if (ImGui::Begin("Performance Overview"))
    {
      GPUTimestamps::TimestampData timestampData = GPUTimestamps::GetTimestampData();
      ImGui::Text("%s: %.2f\n", "Total Frame Time", timestampData.totalFrameTimeInUS);
    }
    ImGui::End();
  }
}

void Menu_RenderPassStats()
{
  struct RunningTimestampEntry
  {
    uint32 numSamples = 0;
    float runningTermQ = 0.0f;
    float runningAvg = 0.0f;
    float runningMax = 0.0f;
  };

  struct DisplayTimestampEntry
  {
    enum : uint8
    {
      TimeCurr,
      TimeAvg,
      StdDev,
      TimeMax,
      DisplayCount,
    };

    const char* name = NULL;
    float timeData[DisplayCount] = {};
  };

  // Track timestamp name hash to running statistics data
  static const uint32 ReserveEles = 256;
  static Core::HashMap<uint64, RunningTimestampEntry, MapHashFn64> runningStatsMap;
  runningStatsMap.Reserve(ReserveEles);

  // Final list of entries to display for sorting
  static Core::Vector<DisplayTimestampEntry> entryDisplayList;
  entryDisplayList.Reserve(ReserveEles);
  entryDisplayList.Clear();

  if (mainMenu_SelectedRPTimings)
  {
    if (ImGui::Begin("GPU Render Pass Timings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      static bool shouldCopyToClip = false;
      static int displayFactorBtnIdx = 0;
      static float displayConversionFactor = 1.0f;

      if (ImGui::SmallButton("Clear"))
      {
        runningStatsMap.Clear();
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Copy"))
      {
        shouldCopyToClip = true;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("US", &displayFactorBtnIdx, 0))
      {
        displayConversionFactor = 1.0f;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("MS", &displayFactorBtnIdx, 1))
      {
        displayConversionFactor = 0.001f;
      }

      ImGuiTableFlags_ tableFlags = (ImGuiTableFlags_)(ImGuiTableFlags_RowBg
                                                       | ImGuiTableFlags_SizingFixedSame
                                                       | ImGuiTableFlags_PadOuterX
                                                       | ImGuiTableFlags_Resizable
                                                       | ImGuiTableFlags_Sortable
                                                       | ImGuiTableFlags_SortTristate);

      const uint32 numCols = 5;
      if (ImGui::BeginTable("GPU Render Pass Timings Table", numCols, tableFlags))
      {
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.4f, 0.3f, 0.0f, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.4f, 0.3f, 0.0f, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 0.5f));

        const char* headerStrings[numCols] = {
          "Pass name", "Curr time", "Avg time", "Std dev", "Max time",
        };

        // Column headers
        ImGui::TableSetupColumn(headerStrings[0],
                                ImGuiTableColumnFlags_PreferSortAscending);
        ImGui::TableSetupColumn(headerStrings[1],
                                ImGuiTableColumnFlags_PreferSortDescending);
        ImGui::TableSetupColumn(headerStrings[2],
                                ImGuiTableColumnFlags_PreferSortDescending);
        ImGui::TableSetupColumn(headerStrings[3],
                                ImGuiTableColumnFlags_PreferSortDescending);
        ImGui::TableSetupColumn(headerStrings[4],
                                ImGuiTableColumnFlags_PreferSortDescending);
        ImGui::TableHeadersRow();

        // Timestamp data rows
        GPUTimestamps::TimestampData timestampData = GPUTimestamps::GetTimestampData();
        for (uint32 i = 0; i < timestampData.numTimestamps; ++i)
        {
          const GPUTimestamps::Timestamp& currTimestamp = timestampData.timestamps[i];

          if (!currTimestamp.name)
          {
            continue;
          }

          const Core::Hash timestampNameHash =
            HASH_64_RUNTIME(currTimestamp.name, (uint32)strlen(currTimestamp.name));

          RunningTimestampEntry* entry = NULL;
          uint32 index = runningStatsMap.FindIndex(timestampNameHash.m_val);

          if (index == Core::HashMapBase::eInvalidIndex)
          {
            // First time add to map
            index = runningStatsMap.Insert(timestampNameHash.m_val, {});
          }
          entry = &(runningStatsMap.DataAtIndex(index));

          // Update stats in entry
          const float currentSample = currTimestamp.timeInst;
          const float prevRunningAvg = entry->runningAvg;
          entry->numSamples++;
          entry->runningMax = Max(entry->runningMax, currentSample);
          // https://en.wikipedia.org/wiki/Standard_deviation#Rapid_calculation_methods
          entry->runningAvg =
            prevRunningAvg + ((currentSample - prevRunningAvg) / entry->numSamples);
          entry->runningTermQ =
            entry->runningTermQ
            + (currentSample - prevRunningAvg) * (currentSample - entry->runningAvg);
          float currStdDev = 0.0f;
          if (entry->numSamples > 1)
          {
            currStdDev = sqrtf(entry->runningTermQ / (entry->numSamples - 1));
          }

          DisplayTimestampEntry displayEntry = {};
          displayEntry.name = currTimestamp.name;
          displayEntry.timeData[DisplayTimestampEntry::TimeCurr] = currentSample;
          displayEntry.timeData[DisplayTimestampEntry::TimeAvg] = entry->runningAvg;
          displayEntry.timeData[DisplayTimestampEntry::StdDev] = currStdDev;
          displayEntry.timeData[DisplayTimestampEntry::TimeMax] = entry->runningMax;
          entryDisplayList.PushBackRaw(displayEntry);
        }

        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
        if (sortSpecs->SpecsCount)
        {
          const ImGuiTableColumnSortSpecs* tableSortSpecs = sortSpecs->Specs;

          Core::MergeSort(
            (DisplayTimestampEntry*)entryDisplayList.Data(), entryDisplayList.Size(),
            [=](const void* A, const void* B)
            {
              const DisplayTimestampEntry* entryA = (DisplayTimestampEntry*)A;
              const DisplayTimestampEntry* entryB = (DisplayTimestampEntry*)B;

              bool compareResult = 0;
              switch (tableSortSpecs->ColumnIndex)
              {
                case 0:
                {
                  compareResult = strcmp(entryA->name, entryB->name) < 0 ? 1 : 0;
                  break;
                }
                case 1:
                case 2:
                case 3:
                case 4:
                {
                  compareResult = entryA->timeData[tableSortSpecs->ColumnIndex - 1]
                                  < entryB->timeData[tableSortSpecs->ColumnIndex - 1];
                  break;
                }

                default:
                {
                  break;
                }
              }

              if (tableSortSpecs->SortDirection == ImGuiSortDirection_Descending)
              {
                compareResult = !compareResult;
              }

              return compareResult;
            });
        }

        if (shouldCopyToClip)
        {
          static Core::StrFixedBuffer<1'048'576> csvOutput;
          csvOutput.Clear();

          // Headers
          const char* delimiter = ",";
          for (uint32 uiValue = 0; uiValue < numCols; ++uiValue)
          {
            csvOutput.Append(headerStrings[uiValue]);
            csvOutput.Append(delimiter);
          }
          csvOutput.Append("\n");

          // Data
          for (uint32 i = 0; i < entryDisplayList.Size(); ++i)
          {
            const DisplayTimestampEntry& displayEntry = entryDisplayList[i];

            csvOutput.Append(displayEntry.name);
            csvOutput.Append(delimiter);
            for (uint32 uiValue = 0; uiValue < DisplayTimestampEntry::DisplayCount;
                 ++uiValue)
            {
              int result = sprintf_s(csvOutput.EndOfStrPtr(), csvOutput.LenRemaining(),
                                     "%.2f", displayEntry.timeData[uiValue]);
              TINKER_ASSERT(result != -1);
              csvOutput.m_len += result;
              csvOutput.Append(delimiter);
            }
            csvOutput.Append("\n");
          }
          csvOutput.NullTerminate();
          ImGui::SetClipboardText(csvOutput.m_data);

          shouldCopyToClip = false;
        }

        for (uint32 i = 0; i < entryDisplayList.Size(); ++i)
        {
          const DisplayTimestampEntry& displayEntry = entryDisplayList[i];

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%s", displayEntry.name);
          ImGui::TableNextColumn();
          ImGui::Text("%.3f", displayEntry.timeData[DisplayTimestampEntry::TimeCurr]
                                * displayConversionFactor);
          ImGui::TableNextColumn();
          ImGui::Text("%.3f", displayEntry.timeData[DisplayTimestampEntry::TimeAvg]
                                * displayConversionFactor);
          ImGui::TableNextColumn();
          ImGui::Text((const char*)u8"± %.2f",
                      displayEntry.timeData[DisplayTimestampEntry::StdDev]
                        * displayConversionFactor);
          ImGui::TableNextColumn();
          ImGui::Text("%.3f", displayEntry.timeData[DisplayTimestampEntry::TimeMax]
                                * displayConversionFactor);
        }

        ImGui::EndTable();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
      }
    }
    ImGui::End();
  }
}

void Menu_MemoryAllocationTracker()
{
  if (mainMenu_SelectedMemTracker)
  {
    if (ImGui::Begin("Memory Allocation Overview"))
    {
      if (ImGui::BeginTabBar("MemAllocViewsTabBar"))
      {
        if (ImGui::BeginTabItem("Per-frame"))
        {
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("All"))
        {
          const Core::Utility::AllocRecordMap& allMemAllocRecords =
            Core::Utility::GetAllAllocRecords();
          uint64 allocBytesTotal = 0;
          uint64 freedBytesTotal = 0;

          // Collect summary stats
          for (size_t i = 0; i < allMemAllocRecords.Capacity(); ++i)
          {
            const uint64 key = allMemAllocRecords.KeyAtIndex(static_cast<uint32>(i));
            if (key == allMemAllocRecords.GetInvalidKey())
            {
              continue;
            }
            const Core::Utility::MemRecord& record =
              allMemAllocRecords.DataAtIndex(static_cast<uint32>(i));

            allocBytesTotal += record.sizeInBytes;
            if (record.bWasDeallocated)
            {
              freedBytesTotal += record.sizeInBytes;
            }
          }
          ImGui::Text("Summary");
          ImGui::Text("Total bytes requested (malloc only): ");
          {
            char buffer[256];
            memset(buffer, 0, ARRAYCOUNT(buffer));
            _ui64toa_s(allocBytesTotal, buffer, ARRAYCOUNT(buffer), 10);
            ImGui::Text(buffer);
            ImGui::SameLine();
            ImGui::Text("bytes\n");
          }
          ImGui::Text("Total bytes freed (malloc only): ");
          {
            char buffer[256];
            memset(buffer, 0, ARRAYCOUNT(buffer));
            _ui64toa_s(freedBytesTotal, buffer, ARRAYCOUNT(buffer), 10);
            ImGui::Text(buffer);
            ImGui::SameLine();
            ImGui::Text("bytes\n");
          }

          // Print entries
          for (size_t i = 0; i < allMemAllocRecords.Capacity(); ++i)
          {
            const uint64 key = allMemAllocRecords.KeyAtIndex(static_cast<uint32>(i));
            if (key == allMemAllocRecords.GetInvalidKey())
            {
              continue;
            }
            const Core::Utility::MemRecord& record =
              allMemAllocRecords.DataAtIndex(static_cast<uint32>(i));

            char buffer[256];
            memset(buffer, 0, ARRAYCOUNT(buffer));
            _ui64toa_s(record.sizeInBytes, buffer, ARRAYCOUNT(buffer), 10);
            ImGui::Text(buffer);
            ImGui::SameLine();
            ImGui::Text("bytes\n");
          }

          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
    }
    ImGui::End();
  }
}

void UpdateAllDebugMenus()
{
  Menu_MainMenu();
  Menu_PerformanceOverview();
  Menu_RenderPassStats();
  Menu_MemoryAllocationTracker();
}
