#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

class FScopedTransaction;

/**
 * LOD 生成事务管理器
 * 支持撤销/重做操作
 */
class FLODGenerationTransaction
{
public:
	FLODGenerationTransaction();
	~FLODGenerationTransaction();

	/** 开始事务 */
	void BeginTransaction(const FText& Description);

	/** 结束事务 */
	void EndTransaction();

	/** 取消事务（撤销所有更改） */
	void CancelTransaction();

	/** 备份网格的 LOD 设置 */
	void BackupMeshLODSettings(UStaticMesh* Mesh);

	/** 恢复网格的 LOD 设置 */
	void RestoreMeshLODSettings(UStaticMesh* Mesh);

	/** 检查是否有活动事务 */
	bool IsActive() const { return bIsActive; }

private:
	/** 网格 LOD 备份数据 */
	struct FMeshLODBackup
	{
		TWeakObjectPtr<UStaticMesh> Mesh;
		int32 OriginalNumLODs;

		struct FLODSettings
		{
			FMeshBuildSettings BuildSettings;
			FMeshReductionSettings ReductionSettings;
			float ScreenSize;
		};

		TArray<FLODSettings> LODSettings;
	};

	/** 是否有活动事务 */
	bool bIsActive;

	/** 事务描述 */
	FText TransactionDescription;

	/** 备份数据 */
	TMap<UStaticMesh*, FMeshLODBackup> BackupData;

	/** ✅ 移除 FScopedTransaction，改用事务索引 */
	int32 TransactionIndex;
};