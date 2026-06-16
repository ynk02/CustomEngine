#pragma once

#include "../AActor.h"
#include "../URigidBodyComponent.h"
#include "../../ImGui/imgui.h"
#include <vector>
#include <functional>

class UDetailsPanel
{
public:

    void RenderUI(
        const std::vector<TSharedPtr<AActor>>& Actors,
        int SelectedIndex,
        const std::function<void(TSharedPtr<AActor>, bool)>& OnTogglePhysics)
    {
        ImGui::Begin("Details");

        if (SelectedIndex < 0 || SelectedIndex >= (int)Actors.size())
        {
            ImGui::TextDisabled("No actor selected.");
            ImGui::End();
            return;
        }

        auto& Actor = Actors[SelectedIndex];
        if (!Actor) { ImGui::End(); return; }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.0f));
        ImGui::Text("[A]  %s", Actor->GetName().c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        RenderTransformSection(Actor);

        TSharedPtr<URigidBodyComponent> RigidBody = FindRigidBodyComponent(Actor);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.8f, 0.4f));
        bool physicsOpen = ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopStyleColor();

        if (physicsOpen)
        {
            bool bHasPhysics = (RigidBody != nullptr);
            if (ImGui::Checkbox("Has Physics Component", &bHasPhysics))
            {
                if (OnTogglePhysics)
                {
                    OnTogglePhysics(Actor, bHasPhysics);

                    RigidBody = FindRigidBodyComponent(Actor);
                }
            }

            if (RigidBody)
            {
                ImGui::Spacing();
                RenderPhysicsSection(RigidBody);
            }
        }

        ImGui::End();
    }

private:

    void RenderTransformSection(const TSharedPtr<AActor>& Actor)
    {
        bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);
        if (!open) return;

        FTransform T      = Actor->GetTransform();
        bool       Changed = false;

        ImGui::PushItemWidth(-1.0f);

        ImGui::Text("Location");
        Changed |= ImGui::DragFloat3("##Loc", &T.Location.x, 0.05f, -FLT_MAX, FLT_MAX, "%.3f");

        ImGui::Text("Rotation");
        Changed |= ImGui::DragFloat3("##Rot", &T.Rotation.x, 1.0f, -360.0f, 360.0f, "%.1f deg");

        ImGui::Text("Scale");
        Changed |= ImGui::DragFloat3("##Scl", &T.Scale3D.x, 0.01f, 0.001f, 100.0f, "%.3f");

        ImGui::PopItemWidth();

        if (Changed)
        {

            TSharedPtr<URigidBodyComponent> RB = FindRigidBodyComponent(Actor);
            if (RB)
                RB->SyncTransformToPhysics(T);
            else
                Actor->SetTransform(T);
        }
    }

    void RenderPhysicsSection(const TSharedPtr<URigidBodyComponent>& RigidBody)
    {
        ImGui::PushItemWidth(180.0f);

        bool bSim = RigidBody->IsSimulatingPhysics();
        if (ImGui::Checkbox("Simulate Physics", &bSim))
            RigidBody->SetSimulatePhysics(bSim);

        if (!bSim)
        {

            ImGui::BeginDisabled(true);
        }

        float mass = RigidBody->GetMass();
        if (ImGui::DragFloat("Mass (kg)", &mass, 0.1f, 0.001f, 10000.0f, "%.2f"))
            RigidBody->SetMass(mass);

        float linDamp = RigidBody->GetLinearDampingValue();
        if (ImGui::DragFloat("Lin. Damping", &linDamp, 0.001f, 0.0f, 1.0f, "%.4f"))
            RigidBody->SetLinearDampingValue(linDamp);

        if (IPhysicsActor* PA = RigidBody->GetPhysicsActor())
        {
            FVector Vel = PA->GetLinearVelocity();
            ImGui::Spacing();
            ImGui::TextDisabled("Velocity: (%.2f, %.2f, %.2f)", Vel.x, Vel.y, Vel.z);

            ImGui::Spacing();
            if (ImGui::Button("  Launch Up  "))
                PA->AddImpulse(FVector(0.0f, 8.0f, 0.0f));

            ImGui::SameLine();
            if (ImGui::Button("  Reset  "))
            {
                PA->SetLinearVelocity(FVector(0.0f));
            }
        }

        if (!bSim) ImGui::EndDisabled();

        ImGui::PopItemWidth();
    }

    TSharedPtr<URigidBodyComponent> FindRigidBodyComponent(const TSharedPtr<AActor>& Actor)
    {
        for (const auto& Comp : Actor->GetComponents())
        {
            if (auto RB = Cast<URigidBodyComponent>(Comp))
                return RB;
        }
        return nullptr;
    }
};
