#pragma once

#include "AActor.h"
#include "URigidBodyComponent.h"
#include "imgui.h"
#include <vector>
#include <functional>

// ============================================================
//  UDetailsPanel
//  언리얼의 Details Panel 역할.
//  선택된 AActor의 Transform + 물리 속성(질량, 활성화 등)을
//  ImGui로 표시하고 실시간 편집 가능.
//
//  URigidBodyComponent 존재 여부를 Cast로 확인해
//  물리 섹션을 조건부 표시 → 비물리 Actor는 물리 섹션이 숨겨짐.
// ============================================================
class UDetailsPanel
{
public:
    /**
     * 매 프레임 RenderUI() 를 호출해 패널을 그림.
     * @param Actors        월드 Actor 배열
     * @param SelectedIndex 현재 선택 인덱스
     */
    void RenderUI(
        const std::vector<TSharedPtr<AActor>>& Actors,
        int SelectedIndex)
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

        // ===== 섹션 1: Actor 이름 =====
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.0f));
        ImGui::Text("[A]  %s", Actor->GetName().c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        // ===== 섹션 2: Transform =====
        RenderTransformSection(Actor);

        // ===== 섹션 3: Physics (URigidBodyComponent 있을 때만) =====
        TSharedPtr<URigidBodyComponent> RigidBody = FindRigidBodyComponent(Actor);
        if (RigidBody)
        {
            ImGui::Spacing();
            RenderPhysicsSection(RigidBody);
        }

        ImGui::End();
    }

private:
    // ---- Transform 섹션 ----
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
            // URigidBodyComponent가 있다면 물리 액터에도 Transform 반영 (텔레포트)
            TSharedPtr<URigidBodyComponent> RB = FindRigidBodyComponent(Actor);
            if (RB)
                RB->SyncTransformToPhysics(T);
            else
                Actor->SetTransform(T);
        }
    }

    // ---- Physics 섹션 ----
    void RenderPhysicsSection(const TSharedPtr<URigidBodyComponent>& RigidBody)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.8f, 0.4f));
        bool open = ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopStyleColor();
        if (!open) return;

        ImGui::PushItemWidth(180.0f);

        // ── Simulate Physics 토글 ──
        bool bSim = RigidBody->IsSimulatingPhysics();
        if (ImGui::Checkbox("Simulate Physics", &bSim))
            RigidBody->SetSimulatePhysics(bSim);

        if (!bSim)
        {
            // 시뮬레이션 꺼진 경우 나머지 항목 비활성화
            ImGui::BeginDisabled(true);
        }

        // ── Mass ──
        float mass = RigidBody->GetMass();
        if (ImGui::DragFloat("Mass (kg)", &mass, 0.1f, 0.001f, 10000.0f, "%.2f"))
            RigidBody->SetMass(mass);

        // ── Linear Damping ──
        float linDamp = RigidBody->GetLinearDampingValue();
        if (ImGui::DragFloat("Lin. Damping", &linDamp, 0.001f, 0.0f, 1.0f, "%.4f"))
            RigidBody->SetLinearDampingValue(linDamp);

        // ── Velocity 표시 (읽기전용) ──
        if (IPhysicsActor* PA = RigidBody->GetPhysicsActor())
        {
            FVector Vel = PA->GetLinearVelocity();
            ImGui::Spacing();
            ImGui::TextDisabled("Velocity: (%.2f, %.2f, %.2f)", Vel.x, Vel.y, Vel.z);

            // ── Add Impulse 버튼 ──
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

    // ---- Helper: Actor에서 URigidBodyComponent 찾기 ----
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
