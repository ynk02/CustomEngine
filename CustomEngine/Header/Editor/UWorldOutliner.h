#pragma once

#include "../AActor.h"
#include "../../ImGui/imgui.h"
#include <vector>
#include <string>

class UWorldOutliner
{
public:

    void RenderUI(
        const std::vector<TSharedPtr<AActor>>& Actors,
        int& SelectedIndex,
        const std::function<void(const std::string&)>& OnSpawnActor)
    {
        ImGui::Begin("World Outliner");

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

        for (int i = 0; i < (int)Actors.size(); ++i)
        {
            const auto& Actor = Actors[i];
            if (!Actor) continue;

            RenderActorNode(Actors, i, SelectedIndex);
        }

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

        if (bSelected)
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.5f));

        std::string Label = GetActorIcon(Actor) + "  " + Actor->GetName();
        std::string ID    = Label + "##" + std::to_string(Index);

        ImGuiTreeNodeFlags Flags =
            ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_SpanFullWidth |
            ImGuiTreeNodeFlags_FramePadding;

        if (bSelected)
            Flags |= ImGuiTreeNodeFlags_Selected;

        bool NodeOpen = ImGui::TreeNodeEx(ID.c_str(), Flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            SelectedIndex = Index;

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

        const std::string& Name = Actor->GetName();
        if (Name.find("Floor") != std::string::npos) return "[P]";
        if (Name.find("Light") != std::string::npos) return "[L]";
        return "[A]";
    }
};
