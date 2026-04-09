#pragma once

#include "AActor.h"
#include "imgui.h"
#include <vector>
#include <string>

// ============================================================
//  UWorldOutliner
//  언리얼의 World Outliner 패널 역할.
//  씬의 AActor 계층 구조를 ImGui TreeNode로 표시하고
//  클릭으로 선택 인덱스를 변경.
// ============================================================
class UWorldOutliner
{
public:
    /**
     * 매 프레임 RenderUI() 를 호출해 패널을 그림.
     * @param Actors           월드의 전체 Actor 배열
     * @param SelectedIndex    현재 선택된 인덱스 (in/out)
     * @param OnSpawnActor     [+ Add] 버튼 드롭다운 선택 콜백
     */
    void RenderUI(
        const std::vector<TSharedPtr<AActor>>& Actors,
        int& SelectedIndex,
        const std::function<void(const std::string&)>& OnSpawnActor)
    {
        ImGui::Begin("World Outliner");

        // ---- 상단 툴바 ----
        if (ImGui::Button("  + Add  "))
        {
            ImGui::OpenPopup("AddActorPopup");
        }
        
        if (ImGui::BeginPopup("AddActorPopup"))
        {
            ImGui::TextDisabled("Basic");
            ImGui::Separator();
            if (ImGui::Selectable("Empty Actor")) { if (OnSpawnActor) OnSpawnActor("Empty"); }
            
            ImGui::Spacing();
            ImGui::TextDisabled("Shapes");
            ImGui::Separator();
            if (ImGui::Selectable("Cube")) { if (OnSpawnActor) OnSpawnActor("Cube"); }
            if (ImGui::Selectable("Floor")) { if (OnSpawnActor) OnSpawnActor("Floor"); }
            
            ImGui::EndPopup();
        }
        ImGui::Separator();

        // ---- Actor 트리 ----
        // 루트(부모 없는) Actor만 최상위로 표시
        for (int i = 0; i < (int)Actors.size(); ++i)
        {
            const auto& Actor = Actors[i];
            if (!Actor) continue;

            RenderActorNode(Actors, i, SelectedIndex);
        }

        // ---- 빈 공간 클릭 → 선택 해제 ----
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !ImGui::IsAnyItemHovered())
        {
            SelectedIndex = -1;
        }

        ImGui::End();
    }

private:
    void RenderActorNode(
        const std::vector<TSharedPtr<AActor>>& Actors,
        int Index,
        int& SelectedIndex)
    {
        const auto& Actor = Actors[Index];
        if (!Actor) return;

        bool bSelected = (SelectedIndex == Index);

        // 선택 하이라이트 색상
        if (bSelected)
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.5f));

        // Actor 아이콘 + 이름
        std::string Label = GetActorIcon(Actor) + "  " + Actor->GetName();
        std::string ID    = Label + "##" + std::to_string(Index);

        ImGuiTreeNodeFlags Flags =
            ImGuiTreeNodeFlags_Leaf |            // 자식 없음 (현재는 flat 구조)
            ImGuiTreeNodeFlags_SpanFullWidth |
            ImGuiTreeNodeFlags_FramePadding;

        if (bSelected)
            Flags |= ImGuiTreeNodeFlags_Selected;

        bool NodeOpen = ImGui::TreeNodeEx(ID.c_str(), Flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            SelectedIndex = Index;

        // ── 우클릭 컨텍스트 메뉴 ──
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::TextDisabled("  %s", Actor->GetName().c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Select"))          SelectedIndex = Index;
            if (ImGui::MenuItem("Deselect"))        SelectedIndex = -1;
            ImGui::EndPopup();
        }

        if (NodeOpen)
            ImGui::TreePop();

        if (bSelected)
            ImGui::PopStyleColor();
    }

    std::string GetActorIcon(const TSharedPtr<AActor>& Actor)
    {
        // 간단한 이름 기반 아이콘 (유니코드 사용 없이 ASCII)
        const std::string& Name = Actor->GetName();
        if (Name.find("Floor") != std::string::npos) return "[P]"; // Plane
        if (Name.find("Light") != std::string::npos) return "[L]"; // Light
        return "[A]";                                               // Actor
    }
};
