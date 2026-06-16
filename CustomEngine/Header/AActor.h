#pragma once

#include "UObject.h"
#include "USceneComponent.h"
#include <vector>

class AActor : public UObject
{
public:
    AActor() = default;
    virtual ~AActor() = default;

    virtual void BeginPlay()
    {
        for (auto& Comp : Components)
        {
            if (Comp->IsActive())
            {
                Comp->BeginPlay();
            }
        }
    }

    virtual void Tick(float DeltaTime)
    {
        for (auto& Comp : Components)
        {
            if (Comp->IsActive())
            {
                Comp->TickComponent(DeltaTime);
            }
        }
    }

    USceneComponent* GetRootComponent() const { return RootComponent; }
    void SetRootComponent(USceneComponent* NewRootComponent)
    {
        RootComponent = NewRootComponent;
    }

    FTransform GetTransform() const
    {
        if (USceneComponent* Root = GetRootComponent()) return Root->GetTransform();
        return FTransform();
    }

    void SetTransform(const FTransform& NewTransform)
    {
        if (USceneComponent* Root = GetRootComponent()) Root->SetTransform(NewTransform);
    }

    void AddComponent(TSharedPtr<UActorComponent> Component)
    {
        Components.push_back(Component);
        Component->SetOuter(this);
    }

    void RemoveComponent(UActorComponent* ComponentToRemove)
    {
        auto it = std::remove_if(Components.begin(), Components.end(),
            [ComponentToRemove](const TSharedPtr<UActorComponent>& Comp) {
                return Comp.get() == ComponentToRemove;
            });

        if (it != Components.end())
        {
            if (RootComponent == ComponentToRemove)
            {

                RootComponent = nullptr;
                for (auto& Comp : Components)
                {
                    if (Comp.get() != ComponentToRemove)
                    {
                        if (auto SceneComp = dynamic_cast<USceneComponent*>(Comp.get()))
                        {
                            RootComponent = SceneComp;
                            break;
                        }
                    }
                }
            }
            Components.erase(it, Components.end());
        }
    }

    template <typename T>
    TSharedPtr<T> CreateDefaultSubobject(const FString& ComponentName)
    {
        TSharedPtr<T> NewComp = MakeShared<T>();
        NewComp->SetName(ComponentName);
        AddComponent(NewComp);
        return NewComp;
    }

    const std::vector<TSharedPtr<UActorComponent>>& GetComponents() const
    {
        return Components;
    }

protected:
    USceneComponent* RootComponent = nullptr;
    std::vector<TSharedPtr<UActorComponent>> Components;
};

inline AActor* UActorComponent::GetOwner() const
{
    return dynamic_cast<AActor*>(GetOuter());
}
