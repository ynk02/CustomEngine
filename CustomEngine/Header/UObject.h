#pragma once

#include "CoreMinimal.h"
#include <memory>
#include <string>
#include <vector>

template<typename T> using TSharedPtr = std::shared_ptr<T>;
template<typename T> using TWeakPtr = std::weak_ptr<T>;
template<typename T> using TUniquePtr = std::unique_ptr<T>;

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename To, typename From>
TSharedPtr<To> Cast(TSharedPtr<From> InPtr)
{
    return std::dynamic_pointer_cast<To>(InPtr);
}

class UObject : public std::enable_shared_from_this<UObject>
{
public:
    UObject() = default;
    virtual ~UObject() = default;

    FString GetName() const { return Name; }
    void SetName(const FString& InName) { Name = InName; }

    UObject* GetOuter() const { return Outer; }
    void SetOuter(UObject* InOuter) { Outer = InOuter; }

    template <typename T>
    TSharedPtr<T> GetSharedThis() { return Cast<T>(shared_from_this()); }

protected:
    FString Name;

private:
    UObject* Outer = nullptr;
};
