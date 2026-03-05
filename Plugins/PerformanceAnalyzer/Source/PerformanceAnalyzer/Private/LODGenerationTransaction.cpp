#include "LODGenerationTransaction.h"
#include "Editor.h"

FLODGenerationTransaction::FLODGenerationTransaction()
    : bIsActive(false)
    , TransactionIndex(INDEX_NONE)
{
}

FLODGenerationTransaction::~FLODGenerationTransaction()
{
    if (bIsActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔴 Transaction still active in destructor, ending it"));
        EndTransaction();
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔴 FLODGenerationTransaction destroyed"));
}

void FLODGenerationTransaction::BeginTransaction(const FText& Description)
{
    if (bIsActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Transaction already active, ending previous one"));
        EndTransaction();
    }

    TransactionDescription = Description;
    BackupData.Empty();
    
    // ✅ 使用 GEditor->BeginTransaction 开始事务
    if (GEditor)
    {
        TransactionIndex = GEditor->BeginTransaction(TEXT("LODGeneration"), Description, nullptr);
        bIsActive = true;
        
        UE_LOG(LogTemp, Log, TEXT("✅ Transaction Started: %s (Index: %d)"), 
            *Description.ToString(), TransactionIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ GEditor is null, cannot start transaction"));
    }
}

void FLODGenerationTransaction::EndTransaction()
{
    if (!bIsActive)
    {
        return;
    }

    // ✅ 使用 GEditor->EndTransaction 结束事务
    if (GEditor && TransactionIndex != INDEX_NONE)
    {
        GEditor->EndTransaction();
        
        UE_LOG(LogTemp, Log, TEXT("✅ Transaction Ended: %s (Index: %d)"), 
            *TransactionDescription.ToString(), TransactionIndex);
    }
    
    bIsActive = false;
    TransactionIndex = INDEX_NONE;
}

void FLODGenerationTransaction::CancelTransaction()
{
    if (!bIsActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("No active transaction to cancel"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Cancelling Transaction: %s"), *TransactionDescription.ToString());

    // ✅ 使用 GEditor->CancelTransaction 取消事务
    if (GEditor && TransactionIndex != INDEX_NONE)
    {
        GEditor->CancelTransaction(TransactionIndex);
    }
    
    // 恢复所有备份的网格
    for (auto& Pair : BackupData)
    {
        RestoreMeshLODSettings(Pair.Key);
    }

    bIsActive = false;
    TransactionIndex = INDEX_NONE;
    BackupData.Empty();

    UE_LOG(LogTemp, Log, TEXT("Transaction cancelled and all changes reverted"));
}

void FLODGenerationTransaction::BackupMeshLODSettings(UStaticMesh* Mesh)
{
    if (!Mesh || !bIsActive)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ BackupMeshLODSettings failed: Mesh=%s, bIsActive=%s"),
            Mesh ? *Mesh->GetName() : TEXT("NULL"),
            bIsActive ? TEXT("true") : TEXT("false"));
        return;
    }

    // 如果已经备份过，跳过
    if (BackupData.Contains(Mesh))
    {
        return;
    }

    // ✅ 调用 Modify() 将对象注册到事务系统
    Mesh->Modify();
    UE_LOG(LogTemp, Log, TEXT("✅ Called Mesh->Modify() for: %s"), *Mesh->GetName());

    FMeshLODBackup Backup;
    Backup.Mesh = Mesh;
    
    // 备份当前 LOD 数量
    Backup.OriginalNumLODs = Mesh->GetNumSourceModels();
    
    // 备份每个 LOD 的设置
    Backup.LODSettings.Empty();
    for (int32 i = 0; i < Backup.OriginalNumLODs; ++i)
    {
        if (Mesh->IsSourceModelValid(i))
        {
            FMeshLODBackup::FLODSettings Settings;
            const FStaticMeshSourceModel& SourceModel = Mesh->GetSourceModel(i);
            
            Settings.BuildSettings = SourceModel.BuildSettings;
            Settings.ReductionSettings = SourceModel.ReductionSettings;
            Settings.ScreenSize = SourceModel.ScreenSize.Default;
            
            Backup.LODSettings.Add(Settings);
        }
    }

    BackupData.Add(Mesh, Backup);

    UE_LOG(LogTemp, Log, TEXT("Backed up LOD settings for mesh: %s (LODs: %d)"), 
        *Mesh->GetName(), Backup.OriginalNumLODs);
}

void FLODGenerationTransaction::RestoreMeshLODSettings(UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        return;
    }

    FMeshLODBackup* Backup = BackupData.Find(Mesh);
    if (!Backup)
    {
        UE_LOG(LogTemp, Warning, TEXT("No backup found for mesh: %s"), *Mesh->GetName());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Restoring LOD settings for mesh: %s"), *Mesh->GetName());

    Mesh->Modify();

    // 恢复 LOD 数量
    int32 CurrentNumLODs = Mesh->GetNumSourceModels();
    
    if (CurrentNumLODs != Backup->OriginalNumLODs)
    {
        Mesh->SetNumSourceModels(Backup->OriginalNumLODs);
    }

    // 恢复每个 LOD 的设置
    for (int32 i = 0; i < Backup->LODSettings.Num() && i < Backup->OriginalNumLODs; ++i)
    {
        if (Mesh->IsSourceModelValid(i))
        {
            FStaticMeshSourceModel& SourceModel = Mesh->GetSourceModel(i);
            const FMeshLODBackup::FLODSettings& BackupSettings = Backup->LODSettings[i];
            
            SourceModel.BuildSettings = BackupSettings.BuildSettings;
            SourceModel.ReductionSettings = BackupSettings.ReductionSettings;
            SourceModel.ScreenSize.Default = BackupSettings.ScreenSize;
        }
    }

    Mesh->Build();
    Mesh->PostEditChange();

    UE_LOG(LogTemp, Log, TEXT("Successfully restored LOD settings for: %s (LODs: %d)"), 
        *Mesh->GetName(), Backup->OriginalNumLODs);
}