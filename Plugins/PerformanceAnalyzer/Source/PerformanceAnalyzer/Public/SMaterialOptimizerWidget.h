
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"  // 添加这个包含
#include "MaterialOptimizer.h"
#include "MaterialOptimizationTransaction.h"

class SEditableTextBox;
class SCheckBox;
class SButton;
class STextBlock;
class SScrollBox;

/**
 * 材质列表项数据
 */
struct FMaterialListItem
{
    /** 材质使用信息 */
    FMaterialUsageInfo MaterialInfo;
    
    /** 是否选中 */
    bool bSelected = false;
};

/**
 * 材质优化器 UI 界面
 */
class PERFORMANCEANALYZER_API SMaterialOptimizerWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMaterialOptimizerWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // ===== UI 组件 =====
    
    // 扫描设置
    TSharedPtr<SCheckBox> OnlySelectedCheckBox;
    TSharedPtr<SButton> ScanButton;
    
    // 优化设置
    TSharedPtr<SEditableTextBox> SavePathTextBox;
    TSharedPtr<SEditableTextBox> PrefixTextBox;
    TSharedPtr<SEditableTextBox> SuffixTextBox;
    TSharedPtr<SCheckBox> AutoSaveCheckBox;
    
    // 材质列表
    TSharedPtr<SListView<TSharedPtr<FMaterialListItem>>> MaterialListView;
    TArray<TSharedPtr<FMaterialListItem>> MaterialListItems;
    
    // 统计信息
    TSharedPtr<STextBlock> TotalMaterialsText;
    TSharedPtr<STextBlock> OptimizableMaterialsText;
    TSharedPtr<STextBlock> SelectedMaterialsText;
    
    // 按钮
    TSharedPtr<SButton> OptimizeButton;
    
    // ===== 数据 =====
    
    // 当前设置
    FMaterialOptimizerSettings CurrentSettings;
    
    // 处理状态
    bool bIsProcessing;
    
    // 事务支持
    TUniquePtr<FMaterialOptimizationTransaction> CurrentTransaction;
    
    // ===== UI 回调函数 =====
    
    // 扫描和优化
    FReply OnScanClicked();
    FReply OnOptimizeClicked();
    FReply OnSelectAllClicked();
    FReply OnDeselectAllClicked();
    
    // 设置更改
    void OnOnlySelectedChanged(ECheckBoxState NewState);
    void OnAutoSaveChanged(ECheckBoxState NewState);
    void OnSavePathChanged(const FText& Text);
    void OnPrefixChanged(const FText& Text);
    void OnSuffixChanged(const FText& Text);
    
    // 列表视图
    TSharedRef<ITableRow> OnGenerateRow(
        TSharedPtr<FMaterialListItem> Item,
        const TSharedRef<STableViewBase>& OwnerTable);
    
    void OnItemCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FMaterialListItem> Item);
    ECheckBoxState GetItemCheckState(TSharedPtr<FMaterialListItem> Item) const;
    
    // ===== 辅助函数 =====
    
    // 刷新材质列表
    void RefreshMaterialList();
    
    // 更新统计信息
    void UpdateStatistics();
    
    // 从 UI 收集设置
    FMaterialOptimizerSettings GatherSettingsFromUI();
    
    // 获取选中的材质
    TArray<FMaterialUsageInfo> GetSelectedMaterials() const;
};