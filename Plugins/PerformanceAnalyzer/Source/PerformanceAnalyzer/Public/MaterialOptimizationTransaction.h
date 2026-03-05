#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "GameFramework/Actor.h"

/**
 * 材质优化结果
 */
struct FMaterialOptimizationResult;

/**
 * 材质优化事务 - 支持撤销/重做
 * 使用 GEditor->Trans 直接管理事务
 */
class PERFORMANCEANALYZER_API FMaterialOptimizationTransaction
{
public:
    FMaterialOptimizationTransaction();
    ~FMaterialOptimizationTransaction();

    /**
     * 开始事务
     */
    void BeginTransaction(const FText& Description);

    /**
     * 结束事务
     */
    void EndTransaction();

    /**
     * 取消事务（撤销所有更改）
     */
    void CancelTransaction();

    /**
     * 记录材质替换（在替换前调用）
     * @param Actor 要修改的 Actor
     * @param Component 要修改的组件
     * @param MaterialInstance 新创建的材质实例
     */
    void RecordMaterialReplacement(
        AActor* Actor,
        UActorComponent* Component,
        UMaterialInstanceConstant* MaterialInstance
    );

    /**
     * 检查是否有活动事务
     */
    bool IsTransactionActive() const { return bIsActive; }

    
    /**
     * 获取记录数量
     */
    int32 GetRecordCount() const { return CreatedInstances.Num(); }
private:
    /** 是否有活动事务 */
    bool bIsActive;

    /** 事务索引 */
    int32 TransactionIndex;

    /** 记录的对象（用于取消时清理） */
    TArray<TWeakObjectPtr<UMaterialInstanceConstant>> CreatedInstances;
};