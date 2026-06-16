#pragma once

#include "IPhysicsSystem.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace AVBDMath
{
    inline glm::mat3 diagonal(float m00, float m11, float m22) {
        return glm::mat3(
            m00, 0, 0,
            0, m11, 0,
            0, 0, m22
        );
    }

    inline glm::mat3 orthonormal(const glm::vec3& normal) {
        glm::vec3 n = glm::normalize(normal);
        glm::vec3 t1 = (std::abs(n.x) > std::abs(n.z)) ? glm::vec3(-n.y, n.x, 0.0f) : glm::vec3(0.0f, -n.z, n.y);
        t1 = glm::normalize(t1);
        glm::vec3 t2 = glm::cross(n, t1);

        return glm::mat3(
            n.x, t1.x, t2.x,
            n.y, t1.y, t2.y,
            n.z, t1.z, t2.z
        );
    }

    inline void solve6x6(const glm::mat3& aLin, const glm::mat3& aAng, const glm::mat3& aCross,
                         const glm::vec3& bLin, const glm::vec3& bAng,
                         glm::vec3& xLin, glm::vec3& xAng)
    {

        float A11 = aLin[0][0];
        float A21 = aLin[0][1], A22 = aLin[1][1];
        float A31 = aLin[0][2], A32 = aLin[1][2], A33 = aLin[2][2];
        float A41 = aCross[0][0], A42 = aCross[1][0], A43 = aCross[2][0], A44 = aAng[0][0];
        float A51 = aCross[0][1], A52 = aCross[1][1], A53 = aCross[2][1], A54 = aAng[0][1], A55 = aAng[1][1];
        float A61 = aCross[0][2], A62 = aCross[1][2], A63 = aCross[2][2], A64 = aAng[0][2], A65 = aAng[1][2], A66 = aAng[2][2];

        float L21 = A21 / A11;
        float L31 = A31 / A11;
        float L41 = A41 / A11;
        float L51 = A51 / A11;
        float L61 = A61 / A11;

        float D1 = A11;

        float D2 = A22 - L21 * L21 * D1;
        float L32 = (D2 == 0.0f) ? 0.0f : (A32 - L21 * L31 * D1) / D2;
        float L42 = (D2 == 0.0f) ? 0.0f : (A42 - L21 * L41 * D1) / D2;
        float L52 = (D2 == 0.0f) ? 0.0f : (A52 - L21 * L51 * D1) / D2;
        float L62 = (D2 == 0.0f) ? 0.0f : (A62 - L21 * L61 * D1) / D2;

        float D3 = A33 - (L31 * L31 * D1 + L32 * L32 * D2);
        float L43 = (D3 == 0.0f) ? 0.0f : (A43 - L31 * L41 * D1 - L32 * L42 * D2) / D3;
        float L53 = (D3 == 0.0f) ? 0.0f : (A53 - L31 * L51 * D1 - L32 * L52 * D2) / D3;
        float L63 = (D3 == 0.0f) ? 0.0f : (A63 - L31 * L61 * D1 - L32 * L62 * D2) / D3;

        float D4 = A44 - (L41 * L41 * D1 + L42 * L42 * D2 + L43 * L43 * D3);
        float L54 = (D4 == 0.0f) ? 0.0f : (A54 - L41 * L51 * D1 - L42 * L52 * D2 - L43 * L53 * D3) / D4;
        float L64 = (D4 == 0.0f) ? 0.0f : (A64 - L41 * L61 * D1 - L42 * L62 * D2 - L43 * L63 * D3) / D4;

        float D5 = A55 - (L51 * L51 * D1 + L52 * L52 * D2 + L53 * L53 * D3 + L54 * L54 * D4);
        float L65 = (D5 == 0.0f) ? 0.0f : (A65 - L51 * L61 * D1 - L52 * L62 * D2 - L53 * L63 * D3 - L54 * L64 * D4) / D5;

        float D6 = A66 - (L61 * L61 * D1 + L62 * L62 * D2 + L63 * L63 * D3 + L64 * L64 * D4 + L65 * L65 * D5);

        float y1 = bLin[0];
        float y2 = bLin[1] - L21 * y1;
        float y3 = bLin[2] - L31 * y1 - L32 * y2;
        float y4 = bAng[0] - L41 * y1 - L42 * y2 - L43 * y3;
        float y5 = bAng[1] - L51 * y1 - L52 * y2 - L53 * y3 - L54 * y4;
        float y6 = bAng[2] - L61 * y1 - L62 * y2 - L63 * y3 - L64 * y4 - L65 * y5;

        float z1 = (D1 == 0.0f) ? 0.0f : y1 / D1;
        float z2 = (D2 == 0.0f) ? 0.0f : y2 / D2;
        float z3 = (D3 == 0.0f) ? 0.0f : y3 / D3;
        float z4 = (D4 == 0.0f) ? 0.0f : y4 / D4;
        float z5 = (D5 == 0.0f) ? 0.0f : y5 / D5;
        float z6 = (D6 == 0.0f) ? 0.0f : y6 / D6;

        xAng[2] = z6;
        xAng[1] = z5 - L65 * xAng[2];
        xAng[0] = z4 - L54 * xAng[1] - L64 * xAng[2];
        xLin[2] = z3 - L43 * xAng[0] - L53 * xAng[1] - L63 * xAng[2];
        xLin[1] = z2 - L32 * xLin[2] - L42 * xAng[0] - L52 * xAng[1] - L62 * xAng[2];
        xLin[0] = z1 - L21 * xLin[1] - L31 * xLin[2] - L41 * xAng[0] - L51 * xAng[1] - L61 * xAng[2];
    }

    inline glm::mat3 skew(const glm::vec3& r) {
        return glm::mat3(
            0, r.z, -r.y,
            -r.z, 0, r.x,
            r.y, -r.x, 0
        );
    }
}

struct FAVBDConfig
{
    int   SolverIterations = 10;
    float Gravity          = -9.81f;
    float Alpha            = 0.99f;
    float BetaLin          = 10000.f;
    float BetaAng          = 100.f;
    float Gamma            = 0.999f;
    float FloorY           = 0.0f;
    float DefaultFriction  = 0.5f;
};

struct FAVBDContact
{
    glm::vec3 rA;
    glm::vec3 rB;
    glm::vec3 C0;
    glm::vec3 Penalty;
    glm::vec3 Lambda;
    bool      bStick;

    FAVBDContact() : rA(0.0f), rB(0.0f), C0(0.0f), Penalty(1e-4f), Lambda(0.0f), bStick(false) {}
};

class AVBDPhysicsActor;

struct FAVBDManifold
{
    AVBDPhysicsActor* BodyA;
    AVBDPhysicsActor* BodyB;

    FAVBDContact Contacts[8];
    int NumContacts = 0;

    glm::mat3 Basis;
    float Friction;

    FAVBDManifold(AVBDPhysicsActor* A, AVBDPhysicsActor* B)
        : BodyA(A), BodyB(B), NumContacts(0), Friction(0.5f), Basis(1.0f) {}
};

class AVBDPhysicsActor final : public IPhysicsActor
{
public:
    explicit AVBDPhysicsActor(const FPhysicsActorDesc& Desc)
        : Size(Desc.HalfExtents * 2.0f)
        , Mass(Desc.bKinematic ? 0.0f : Desc.Mass)
        , bKinematic(Desc.bKinematic)
        , bSimulate(Desc.bSimulate)
        , Friction(0.5f)
        , PositionLin(Desc.InitialTransform.Location)
        , VelocityLin(0.0f)
        , VelocityAng(0.0f)
        , PrevVelocityLin(0.0f)
        , LocalTransformExtents(Desc.InitialTransform.Scale3D)
    {

        glm::vec3 rads = glm::radians(Desc.InitialTransform.Rotation);
        PositionAng = glm::quat(rads);

        if (Mass > 0.0f)
        {
            float m = Mass;
            Moment = glm::vec3(
                (Size.y * Size.y + Size.z * Size.z) / 12.0f * m,
                (Size.x * Size.x + Size.z * Size.z) / 12.0f * m,
                (Size.x * Size.x + Size.y * Size.y) / 12.0f * m
            );
        }
        else
        {
            Moment = glm::vec3(0.0f);
        }

        Radius = glm::length(Desc.HalfExtents);
    }

    FTransform GetTransform() const override
    {
        FTransform T;
        T.Location = PositionLin;
        T.Rotation = glm::degrees(glm::eulerAngles(PositionAng));
        T.Scale3D  = LocalTransformExtents;
        return T;
    }

    void SetTransform(const FTransform& T) override
    {
        PositionLin = T.Location;
        PositionAng = glm::quat(glm::radians(T.Rotation));
        VelocityLin = glm::vec3(0.0f);
        VelocityAng = glm::vec3(0.0f);
        PrevVelocityLin = glm::vec3(0.0f);
        LocalTransformExtents = T.Scale3D;
    }

    void  SetMass(float InMass)  override { Mass = bKinematic ? 0.0f : glm::max(InMass, 0.001f); }
    float GetMass()        const override { return Mass; }

    void  SetKinematic(bool bInK)      override { bKinematic = bInK; if(bInK) Mass = 0.0f; }
    bool  IsKinematic()          const override { return bKinematic; }

    void  SetSimulatePhysics(bool bSim) override { bSimulate = bSim; }
    bool  IsSimulatingPhysics()   const override { return bSimulate; }

    void  SetLinearDamping(float D)  override { }
    void  SetAngularDamping(float D) override { }

    void AddForce(const FVector& Force) override { }
    void AddImpulse(const FVector& Impulse) override
    {
        if (Mass > 0.0f)
            VelocityLin += Impulse / Mass;
    }

    FVector GetLinearVelocity()           const override { return VelocityLin; }
    void    SetLinearVelocity(const FVector& V) override { VelocityLin = V; }

    FVector GetHalfExtents() const override { return Size * 0.5f; }

    glm::vec3 PositionLin;
    glm::quat PositionAng;
    glm::vec3 InitialLin;
    glm::quat InitialAng;
    glm::vec3 InertialLin;
    glm::quat InertialAng;
    glm::vec3 VelocityLin;
    glm::vec3 VelocityAng;
    glm::vec3 PrevVelocityLin;

    glm::vec3 Size;
    glm::vec3 Moment;
    float Mass;
    float Friction;
    float Radius;
    bool bKinematic;
    bool bSimulate;
    glm::vec3 LocalTransformExtents;
};

class AVBDPhysicsScene final : public IPhysicsScene
{
public:
    FAVBDConfig Config;

    void Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) override
    {
        Config.Gravity = Gravity.y;
    }

    IPhysicsActor* CreateActor(const FPhysicsActorDesc& Desc) override
    {
        auto Actor = std::make_unique<AVBDPhysicsActor>(Desc);
        AVBDPhysicsActor* RawPtr = Actor.get();
        Actors.push_back(std::move(Actor));
        return RawPtr;
    }

    void RemoveActor(IPhysicsActor* Actor) override
    {
        Actors.erase(
            std::remove_if(Actors.begin(), Actors.end(),
                [Actor](const std::unique_ptr<AVBDPhysicsActor>& a) { return a.get() == Actor; }),
            Actors.end());
    }

    void Simulate(float DeltaTime) override
    {
        if (DeltaTime <= 0.0f) return;
        float dt = DeltaTime;

        std::vector<FAVBDManifold> Manifolds;

        for (auto& Actor : Actors)
        {
            if (Actor->Mass <= 0.0f || !Actor->bSimulate) continue;

            float bottom = Actor->PositionLin.y - (Actor->Size.y * 0.5f);
            if (bottom < Config.FloorY + 0.1f)
            {
                FAVBDManifold m(Actor.get(), nullptr);
                m.Basis = glm::mat3(
                    0, 1, 0,
                    1, 0, 0,
                    0, 0, 1
                );

                FAVBDContact c;
                c.rA = glm::vec3(0, -Actor->Size.y * 0.5f, 0);
                c.rB = glm::vec3(Actor->PositionLin.x, Config.FloorY, Actor->PositionLin.z);
                c.Penalty = glm::vec3(Config.BetaLin * 0.1f);
                c.Lambda = glm::vec3(0.0f);

                glm::vec3 xA = Actor->PositionLin + c.rA;
                glm::vec3 xB = c.rB;

                c.C0 = m.Basis * (xA - xB) + glm::vec3(0.001f, 0, 0);

                c.Lambda *= (Config.Alpha * Config.Gamma);
                c.Penalty = glm::clamp(c.Penalty * Config.Gamma, 1.0f, 100000.0f);

                m.Contacts[0] = c;
                m.NumContacts = 1;
                Manifolds.push_back(m);
            }
        }

        for (auto& body : Actors)
        {
            body->InertialLin = body->PositionLin + body->VelocityLin * dt;
            if (body->Mass > 0)
                body->InertialLin += glm::vec3(0, Config.Gravity, 0) * (dt * dt);

            glm::quat qAngVel(0.0f, body->VelocityAng.x * 0.5f * dt, body->VelocityAng.y * 0.5f * dt, body->VelocityAng.z * 0.5f * dt);
            body->InertialAng = glm::normalize(body->PositionAng + qAngVel * body->PositionAng);

            glm::vec3 accel = (body->VelocityLin - body->PrevVelocityLin) / dt;
            float accelExt = accel.y * glm::sign(Config.Gravity);
            float accelWeight = glm::clamp(std::abs(Config.Gravity) > 1e-5f ? accelExt / std::abs(Config.Gravity) : 0.0f, 0.0f, 1.0f);

            body->InitialLin = body->PositionLin;
            body->InitialAng = body->PositionAng;

            if (body->Mass > 0)
            {
                body->PositionLin = body->PositionLin + body->VelocityLin * dt + glm::vec3(0, Config.Gravity, 0) * (accelWeight * dt * dt);
                body->PositionAng = body->InertialAng;
            }
        }

        for (int it = 0; it < Config.SolverIterations; it++)
        {

            for (auto& body : Actors)
            {
                if (body->Mass <= 0 || !body->bSimulate) continue;

                glm::mat3 MLin = AVBDMath::diagonal(body->Mass, body->Mass, body->Mass);
                glm::mat3 MAng = AVBDMath::diagonal(body->Moment.x, body->Moment.y, body->Moment.z);

                glm::mat3 lhsLin = MLin * (1.0f / (dt * dt));
                glm::mat3 lhsAng = MAng * (1.0f / (dt * dt));
                glm::mat3 lhsCross = glm::mat3(0.0f);

                glm::vec3 rhsLin = MLin * (1.0f / (dt * dt)) * (body->PositionLin - body->InertialLin);
                glm::vec3 rhsAng = glm::vec3(0.0f);

                for (auto& m : Manifolds)
                {
                    if (m.BodyA != body.get()) continue;

                    glm::vec3 dqALin = body->PositionLin - body->InitialLin;

                    glm::quat dqQuat = body->PositionAng * glm::conjugate(body->InitialAng);
                    glm::vec3 dqAAng = glm::vec3(dqQuat.x, dqQuat.y, dqQuat.z) * 2.0f;

                    for (int i = 0; i < m.NumContacts; i++)
                    {
                        FAVBDContact& c = m.Contacts[i];

                        glm::mat3 jALin = m.Basis;
                        glm::vec3 rAWorld = body->PositionLin + c.rA - body->PositionLin;

                        glm::mat3 jAAng(
                            glm::cross(rAWorld, glm::vec3(jALin[0][0], jALin[1][0], jALin[2][0])),
                            glm::cross(rAWorld, glm::vec3(jALin[0][1], jALin[1][1], jALin[2][1])),
                            glm::cross(rAWorld, glm::vec3(jALin[0][2], jALin[1][2], jALin[2][2]))
                        );

                        glm::mat3 K = AVBDMath::diagonal(c.Penalty.x, c.Penalty.y, c.Penalty.z);

                        glm::vec3 C = c.C0 * (1.0f - Config.Alpha) + (jALin * dqALin) + (jAAng * dqAAng);

                        glm::vec3 F = K * C + c.Lambda;
                        F.x = glm::min(F.x, 0.0f);

                        float bounds = std::abs(F.x) * m.Friction;
                        float fricScale = glm::length(glm::vec2(F.y, F.z));
                        if (fricScale > bounds && fricScale > 0)
                        {
                            F.y *= bounds / fricScale;
                            F.z *= bounds / fricScale;
                        }

                        glm::mat3 jLinT = glm::transpose(jALin);
                        glm::mat3 jAngT = glm::transpose(jAAng);
                        glm::mat3 jAngTk = jAngT * K;

                        lhsLin += jLinT * K * jALin;
                        lhsAng += jAngTk * jAAng;
                        lhsCross += jAngTk * jALin;

                        rhsLin += jLinT * F;
                        rhsAng += jAngT * F;
                    }
                }

                glm::vec3 dxLin(0.0f), dxAng(0.0f);
                AVBDMath::solve6x6(lhsLin, lhsAng, lhsCross, -rhsLin, -rhsAng, dxLin, dxAng);

                body->PositionLin += dxLin;

                glm::quat qDelta(0.0f, dxAng.x * 0.5f, dxAng.y * 0.5f, dxAng.z * 0.5f);
                body->PositionAng = glm::normalize(body->PositionAng + qDelta * body->PositionAng);
            }

            for (auto& m : Manifolds)
            {
                if(m.BodyA->Mass <= 0 || !m.BodyA->bSimulate) continue;

                glm::vec3 dqALin = m.BodyA->PositionLin - m.BodyA->InitialLin;
                glm::quat dqQuat = m.BodyA->PositionAng * glm::conjugate(m.BodyA->InitialAng);
                glm::vec3 dqAAng = glm::vec3(dqQuat.x, dqQuat.y, dqQuat.z) * 2.0f;

                for (int i = 0; i < m.NumContacts; i++)
                {
                    FAVBDContact& c = m.Contacts[i];

                    glm::mat3 jALin = m.Basis;
                    glm::vec3 rAWorld = m.BodyA->PositionLin + c.rA - m.BodyA->PositionLin;

                    glm::mat3 jAAng(
                        glm::cross(rAWorld, glm::vec3(jALin[0][0], jALin[1][0], jALin[2][0])),
                        glm::cross(rAWorld, glm::vec3(jALin[0][1], jALin[1][1], jALin[2][1])),
                        glm::cross(rAWorld, glm::vec3(jALin[0][2], jALin[1][2], jALin[2][2]))
                    );

                    glm::mat3 K = AVBDMath::diagonal(c.Penalty.x, c.Penalty.y, c.Penalty.z);
                    glm::vec3 C = c.C0 * (1.0f - Config.Alpha) + (jALin * dqALin) + (jAAng * dqAAng);

                    glm::vec3 F = K * C + c.Lambda;
                    F.x = glm::min(F.x, 0.0f);

                    float bounds = std::abs(F.x) * m.Friction;
                    float fricScale = glm::length(glm::vec2(F.y, F.z));
                    if (fricScale > bounds && fricScale > 0)
                    {
                        F.y *= bounds / fricScale;
                        F.z *= bounds / fricScale;
                    }

                    c.Lambda = F;

                    if (F.x < 0)
                        c.Penalty.x = glm::min(c.Penalty.x + Config.BetaLin * std::abs(C.x), 100000.0f);

                    if (fricScale <= bounds)
                    {
                        c.Penalty.y = glm::min(c.Penalty.y + Config.BetaLin * std::abs(C.y), 100000.0f);
                        c.Penalty.z = glm::min(c.Penalty.z + Config.BetaLin * std::abs(C.z), 100000.0f);
                        c.bStick = glm::length(glm::vec2(C.y, C.z)) < 1e-4f;
                    }
                }
            }
        }

        for (auto& body : Actors)
        {
            body->PrevVelocityLin = body->VelocityLin;
            if (body->Mass > 0 && body->bSimulate)
            {
                body->VelocityLin = (body->PositionLin - body->InitialLin) / dt;

                glm::quat dqQuat = body->PositionAng * glm::conjugate(body->InitialAng);
                body->VelocityAng = glm::vec3(dqQuat.x, dqQuat.y, dqQuat.z) * (2.0f / dt);
            }
        }
    }

    FRaycastHit Raycast(const FRay& Ray, float MaxDistance) const override
    {
        FRaycastHit BestHit;
        float ClosestT = MaxDistance;

        for (const auto& Actor : Actors)
        {
            FTransform T = Actor->GetTransform();
            FVector HE   = Actor->Size * 0.5f * T.Scale3D;
            FVector Mn   = T.Location - HE;
            FVector Mx   = T.Location + HE;

            float tMin = -FLT_MAX, tMax = FLT_MAX;
            bool bHit = true;

            for (int Axis = 0; Axis < 3; ++Axis)
            {
                if (std::abs(Ray.Direction[Axis]) < 1e-6f)
                {
                    if (Ray.Origin[Axis] < Mn[Axis] || Ray.Origin[Axis] > Mx[Axis])
                    { bHit = false; break; }
                }
                else
                {
                    float t1 = (Mn[Axis] - Ray.Origin[Axis]) / Ray.Direction[Axis];
                    float t2 = (Mx[Axis] - Ray.Origin[Axis]) / Ray.Direction[Axis];
                    if (t1 > t2) std::swap(t1, t2);
                    tMin = std::max(tMin, t1);
                    tMax = std::min(tMax, t2);
                    if (tMin > tMax) { bHit = false; break; }
                }
            }

            if (bHit)
            {
                float tHit = (tMin > 0.0f) ? tMin : tMax;
                if (tHit > 0.0f && tHit < ClosestT)
                {
                    ClosestT = tHit;
                    BestHit.bHit        = true;
                    BestHit.Distance    = tHit;
                    BestHit.HitLocation = Ray.Origin + Ray.Direction * tHit;
                    BestHit.HitActor    = Actor.get();
                }
            }
        }
        return BestHit;
    }

    void    SetGravity(const FVector& Gravity) override { Config.Gravity = Gravity.y; }
    FVector GetGravity()                 const override { return FVector(0.0f, Config.Gravity, 0.0f); }

private:
    std::vector<std::unique_ptr<AVBDPhysicsActor>> Actors;
};

class AVBDPhysicsSystem final : public IPhysicsSystem
{
public:
    bool Initialize() override { return true; }
    void Shutdown()   override {}

    std::unique_ptr<IPhysicsScene> CreateScene() override
    {
        return std::make_unique<AVBDPhysicsScene>();
    }
};
