#include "MaterialOptimizationTransaction.h"
#include "Editor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"

FMaterialOptimizationTransaction::FMaterialOptimizationTransaction()
    : bIsActive(false)
    , TransactionIndex(INDEX_NONE)
{
}

FMaterialOptimizationTransaction::~FMaterialOptimizationTransaction()
{
    if (bIsActive)
    {
        EndTransaction();
    }
}

void FMaterialOptimizationTransaction::BeginTransaction(const FText& Description)
{
    if (bIsActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Transaction already active"));
        return;
    }

    // 使用 GEditor->Trans 开始事务
    TransactionIndex = GEditor->BeginTransaction(Description);
    bIsActive = true;

    // 清空记录
    CreatedInstances.Empty();

    UE_LOG(LogTemp, Log, TEXT("Material optimization transaction started: %s"), *Description.ToString());
}

void FMaterialOptimizationTransaction::EndTransaction()
{
    if (!bIsActive)
    {
        return;
    }

    // 使用 GEditor->Trans 结束事务
    GEditor->EndTransaction();
    bIsActive = false;

    UE_LOG(LogTemp, Log, TEXT("Material optimization transaction ended (%d instances created)"), 
        CreatedInstances.Num());
}

void FMaterialOptimizationTransaction::CancelTransaction()
{
    if (!bIsActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("No active transaction to cancel"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Cancelling material optimization transaction..."));

    // 使用 GEditor->Trans 取消事务
    GEditor->CancelTransaction(TransactionIndex);
    bIsActive = false;

    // 清理创建的材质实例
    for (const TWeakObjectPtr<UMaterialInstanceConstant>& InstancePtr : CreatedInstances)
    {
        if (UMaterialInstanceConstant* Instance = InstancePtr.Get())
        {
            Instance->ClearFlags(RF_Standalone | RF_Public);
            Instance->MarkAsGarbage();
        }
    }

    CreatedInstances.Empty();

    UE_LOG(LogTemp, Log, TEXT("Material optimization transaction cancelled"));
}

void FMaterialOptimizationTransaction::RecordMaterialReplacement(
    AActor* Actor,
    UActorComponent* Component,
    UMaterialInstanceConstant* MaterialInstance)
{
    if (!bIsActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot record - transaction not active"));
        return;
    }

    if (!Actor || !Component || !MaterialInstance)
    {
        return;
    }

    // 调用 Modify() 让 UE 的事务系统记录状态
    Actor->Modify();
    Component->Modify();
    MaterialInstance->Modify();

    // 记录创建的实例
    CreatedInstances.AddUnique(MaterialInstance);
}