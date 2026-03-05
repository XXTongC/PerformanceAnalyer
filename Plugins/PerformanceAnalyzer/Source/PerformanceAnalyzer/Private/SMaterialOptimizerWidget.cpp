
#include "SMaterialOptimizerWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "MaterialOptimizer"

void SMaterialOptimizerWidget::Construct(const FArguments& InArgs)
{
    // 初始化设置
    CurrentSettings = FMaterialOptimizerSettings();
    bIsProcessing = false;

    ChildSlot
    [
        SNew(SVerticalBox)

        // ===== 标题栏 =====
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("Title", "Material Optimizer"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
        ]

        // ===== 主内容区域 =====
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10.0f)
        [
            SNew(SSplitter)
            .Orientation(Orient_Horizontal)

            // 左侧：设置面板
            + SSplitter::Slot()
            .Value(0.3f)
            [
                SNew(SScrollBox)

                // 扫描设置
                + SScrollBox::Slot()
                .Padding(5.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ScanSettings", "Scan Settings"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]

                        // 只扫描选中的 Actor
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SAssignNew(OnlySelectedCheckBox, SCheckBox)
                                .IsChecked(ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SMaterialOptimizerWidget::OnOnlySelectedChanged)
                            ]

                            + SHorizontalBox::Slot()
                            .Padding(5.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("OnlySelected", "Only Selected Actors"))
                            ]
                        ]

                        // 扫描按钮
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SAssignNew(ScanButton, SButton)
                            .Text(LOCTEXT("ScanScene", "Scan Scene"))
                            .ToolTipText(LOCTEXT("ScanSceneTooltip", "Scan the scene for materials"))
                            .OnClicked(this, &SMaterialOptimizerWidget::OnScanClicked)
                            .HAlign(HAlign_Center)
                        ]
                    ]
                ]

                // 优化设置
                + SScrollBox::Slot()
                .Padding(5.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("OptimizationSettings", "Optimization Settings"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]

                        // 保存路径
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(SVerticalBox)

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("SavePath", "Save Path:"))
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                            [
                                SAssignNew(SavePathTextBox, SEditableTextBox)
                                .Text(FText::FromString(CurrentSettings.SavePath))
                                .OnTextChanged(this, &SMaterialOptimizerWidget::OnSavePathChanged)
                            ]
                        ]

                        // 前缀
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(SVerticalBox)

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("Prefix", "Instance Prefix:"))
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                            [
                                SAssignNew(PrefixTextBox, SEditableTextBox)
                                .Text(FText::FromString(CurrentSettings.InstancePrefix))
                                .OnTextChanged(this, &SMaterialOptimizerWidget::OnPrefixChanged)
                            ]
                        ]

                        // 后缀
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(SVerticalBox)

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("Suffix", "Instance Suffix:"))
                            ]

                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                            [
                                SAssignNew(SuffixTextBox, SEditableTextBox)
                                .Text(FText::FromString(CurrentSettings.InstanceSuffix))
                                .OnTextChanged(this, &SMaterialOptimizerWidget::OnSuffixChanged)
                            ]
                        ]

                        // 自动保存
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SAssignNew(AutoSaveCheckBox, SCheckBox)
                                .IsChecked(CurrentSettings.bAutoSave ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SMaterialOptimizerWidget::OnAutoSaveChanged)
                            ]

                            + SHorizontalBox::Slot()
                            .Padding(5.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("AutoSave", "Auto Save Assets"))
                            ]
                        ]
                    ]
                ]

                // 统计信息
                + SScrollBox::Slot()
                .Padding(5.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("Statistics", "Statistics"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SAssignNew(TotalMaterialsText, STextBlock)
                            .Text(LOCTEXT("TotalMaterials", "Total Materials: 0"))
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SAssignNew(OptimizableMaterialsText, STextBlock)
                            .Text(LOCTEXT("OptimizableMaterials", "Optimizable: 0"))
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SAssignNew(SelectedMaterialsText, STextBlock)
                            .Text(LOCTEXT("SelectedMaterials", "Selected: 0"))
                        ]
                    ]
                ]
            ]

            // 右侧：材质列表
            + SSplitter::Slot()
            .Value(0.7f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(10.0f)
                [
                    SNew(SVerticalBox)

                    // 列表标题
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("MaterialList", "Material List"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]

                        // 全选按钮
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5.0f, 0.0f)
                        [
                            SNew(SButton)
                            .Text(LOCTEXT("SelectAll", "Select All"))
                            .OnClicked(this, &SMaterialOptimizerWidget::OnSelectAllClicked)
                        ]

                        // 取消全选按钮
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SButton)
                            .Text(LOCTEXT("DeselectAll", "Deselect All"))
                            .OnClicked(this, &SMaterialOptimizerWidget::OnDeselectAllClicked)
                        ]
                    ]

                    // 材质列表
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(MaterialListView, SListView<TSharedPtr<FMaterialListItem>>)
                        .ListItemsSource(&MaterialListItems)
                        .OnGenerateRow(this, &SMaterialOptimizerWidget::OnGenerateRow)
                        .HeaderRow
                        (
                            SNew(SHeaderRow)

                            // 复选框列
                            + SHeaderRow::Column("Select")
                            .DefaultLabel(LOCTEXT("Select", "Select"))
                            .FixedWidth(50.0f)

                            // 材质名称列
                            + SHeaderRow::Column("Material")
                            .DefaultLabel(LOCTEXT("Material", "Material"))
                            .FillWidth(0.4f)

                            // 类型列
                            + SHeaderRow::Column("Type")
                            .DefaultLabel(LOCTEXT("Type", "Type"))
                            .FillWidth(0.2f)

                            // 使用数量列
                            + SHeaderRow::Column("Usage")
                            .DefaultLabel(LOCTEXT("Usage", "Usage"))
                            .FillWidth(0.2f)

                            // 状态列
                            + SHeaderRow::Column("Status")
                            .DefaultLabel(LOCTEXT("Status", "Status"))
                            .FillWidth(0.2f)
                        )
                    ]
                ]
            ]
        ]

        // ===== 底部按钮栏 =====
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(OptimizeButton, SButton)
                .Text(LOCTEXT("Optimize", "Optimize Selected Materials"))
                .ToolTipText(LOCTEXT("OptimizeTooltip", "Convert selected materials to material instances"))
                .OnClicked(this, &SMaterialOptimizerWidget::OnOptimizeClicked)
                .HAlign(HAlign_Center)
                .IsEnabled(false)
            ]
        ]
    ];

    // 初始化统计信息
    UpdateStatistics();
}

FReply SMaterialOptimizerWidget::OnScanClicked()
{
    if (bIsProcessing)
    {
        return FReply::Handled();
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoWorld", "No valid world found."));
        return FReply::Handled();
    }

    // 收集设置
    CurrentSettings = GatherSettingsFromUI();

    // 扫描材质
    TArray<FMaterialUsageInfo> MaterialInfos = FMaterialOptimizer::ScanSceneMaterials(
        World, 
        CurrentSettings.bOnlySelectedActors);

    // 清空现有列表
    MaterialListItems.Empty();

    // 填充列表
    for (const FMaterialUsageInfo& Info : MaterialInfos)
    {
        TSharedPtr<FMaterialListItem> Item = MakeShared<FMaterialListItem>();
        Item->MaterialInfo = Info;
        Item->bSelected = Info.bCanOptimize && !Info.bIsInstance;
        MaterialListItems.Add(Item);
    }

    // 刷新列表
    RefreshMaterialList();
    UpdateStatistics();

    // 启用优化按钮
    if (OptimizeButton.IsValid())
    {
        OptimizeButton->SetEnabled(MaterialListItems.Num() > 0);
    }

    // 显示结果
    FString Message = FString::Printf(
        TEXT("Scan Complete!\n\n")
        TEXT("Total Materials Found: %d\n")
        TEXT("Optimizable Materials: %d"),
        MaterialInfos.Num(),
        MaterialInfos.FilterByPredicate([](const FMaterialUsageInfo& Info) {
            return Info.bCanOptimize && !Info.bIsInstance;
        }).Num()
    );

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));

    return FReply::Handled();
}

FReply SMaterialOptimizerWidget::OnOptimizeClicked()
{
    if (bIsProcessing)
    {
        return FReply::Handled();
    }

    // 获取选中的材质
    TArray<FMaterialUsageInfo> SelectedMaterials = GetSelectedMaterials();

    if (SelectedMaterials.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoSelection", "No materials selected for optimization."));
        return FReply::Handled();
    }

    // 收集设置
    CurrentSettings = GatherSettingsFromUI();

    // 确认对话框
    FText ConfirmMessage = FText::Format(
        LOCTEXT("ConfirmOptimization", 
            "Are you sure you want to optimize {0} material(s)?\n\n"
            "This will create material instances and replace materials on actors."),
        FText::AsNumber(SelectedMaterials.Num())
    );

    EAppReturnType::Type Result = FMessageDialog::Open(
        EAppMsgType::YesNo, 
        ConfirmMessage);

    if (Result != EAppReturnType::Yes)
    {
        return FReply::Handled();
    }

    // 确保保存路径存在
    if (!FMaterialOptimizer::EnsureSavePathExists(CurrentSettings.SavePath))
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            FText::Format(LOCTEXT("InvalidPath", "Failed to create save path: {0}"), 
                FText::FromString(CurrentSettings.SavePath)));
        return FReply::Handled();
    }

    // ✅ 创建事务
    CurrentTransaction = MakeUnique<FMaterialOptimizationTransaction>();
    CurrentTransaction->BeginTransaction(
        FText::Format(LOCTEXT("OptimizeMaterials", "Optimize {0} Material(s)"), 
            FText::AsNumber(SelectedMaterials.Num()))
    );

    // 开始优化
    bIsProcessing = true;

    if (OptimizeButton.IsValid())
    {
        OptimizeButton->SetEnabled(false);
    }

    UE_LOG(LogTemp, Log, TEXT("Starting material optimization for %d materials"), SelectedMaterials.Num());

    // ✅ 批量优化（传入事务）
    TArray<FMaterialOptimizationResult> Results = FMaterialOptimizer::BatchOptimizeMaterials(
        SelectedMaterials, 
        CurrentSettings,
        CurrentTransaction.Get()  // 传入事务指针
    );

    // ✅ 结束事务
    CurrentTransaction->EndTransaction();

    // 完成
    bIsProcessing = false;

    if (OptimizeButton.IsValid())
    {
        OptimizeButton->SetEnabled(true);
    }

    // 统计结果
    int32 SuccessCount = 0;
    int32 FailedCount = 0;
    int32 TotalActors = 0;

    for (const FMaterialOptimizationResult& OptResult : Results)
    {
        if (OptResult.bSuccess)
        {
            SuccessCount++;
            TotalActors += OptResult.ActorCount;
        }
        else
        {
            FailedCount++;
        }
    }

    // 显示结果
    FString ResultMessage = FString::Printf(
        TEXT("Material Optimization Complete!\n\n")
        TEXT("Total Materials: %d\n")
        TEXT("Successfully Optimized: %d\n")
        TEXT("Failed: %d\n")
        TEXT("Total Actors Updated: %d\n\n")
        TEXT("You can use Ctrl+Z to undo this operation."),
        Results.Num(),
        SuccessCount,
        FailedCount,
        TotalActors
    );

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));

    UE_LOG(LogTemp, Warning, TEXT("Material Optimization Complete: %d succeeded, %d failed, %d actors updated"), 
        SuccessCount, FailedCount, TotalActors);

    // 重新扫描以更新列表
    OnScanClicked();

    return FReply::Handled();
}

FReply SMaterialOptimizerWidget::OnSelectAllClicked()
{
    for (TSharedPtr<FMaterialListItem>& Item : MaterialListItems)
    {
        if (Item.IsValid() && Item->MaterialInfo.bCanOptimize && !Item->MaterialInfo.bIsInstance)
        {
            Item->bSelected = true;
        }
    }

    RefreshMaterialList();
    UpdateStatistics();

    return FReply::Handled();
}

FReply SMaterialOptimizerWidget::OnDeselectAllClicked()
{
    for (TSharedPtr<FMaterialListItem>& Item : MaterialListItems)
    {
        if (Item.IsValid())
        {
            Item->bSelected = false;
        }
    }

    RefreshMaterialList();
    UpdateStatistics();

    return FReply::Handled();
}

void SMaterialOptimizerWidget::OnOnlySelectedChanged(ECheckBoxState NewState)
{
    CurrentSettings.bOnlySelectedActors = (NewState == ECheckBoxState::Checked);
}

void SMaterialOptimizerWidget::OnAutoSaveChanged(ECheckBoxState NewState)
{
    CurrentSettings.bAutoSave = (NewState == ECheckBoxState::Checked);
}

void SMaterialOptimizerWidget::OnSavePathChanged(const FText& Text)
{
    CurrentSettings.SavePath = Text.ToString();
}

void SMaterialOptimizerWidget::OnPrefixChanged(const FText& Text)
{
    CurrentSettings.InstancePrefix = Text.ToString();
}

void SMaterialOptimizerWidget::OnSuffixChanged(const FText& Text)
{
    CurrentSettings.InstanceSuffix = Text.ToString();
}

TSharedRef<ITableRow> SMaterialOptimizerWidget::OnGenerateRow(
    TSharedPtr<FMaterialListItem> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    if (!Item.IsValid())
    {
        return SNew(STableRow<TSharedPtr<FMaterialListItem>>, OwnerTable);
    }

    const FMaterialUsageInfo& Info = Item->MaterialInfo;

    // 确定类型文本
    FText TypeText;
    if (Info.bIsInstance)
    {
        TypeText = LOCTEXT("Instance", "Instance");
    }
    else
    {
        TypeText = LOCTEXT("Material", "Material");
    }

    // 确定状态文本和颜色
    FText StatusText;
    FSlateColor StatusColor = FSlateColor::UseForeground();

    if (Info.bIsInstance)
    {
        StatusText = LOCTEXT("AlreadyInstance", "Already Instance");
        StatusColor = FSlateColor(FLinearColor::Gray);
    }
    else if (Info.bCanOptimize)
    {
        StatusText = LOCTEXT("CanOptimize", "Can Optimize");
        StatusColor = FSlateColor(FLinearColor::Green);
    }
    else
    {
        StatusText = LOCTEXT("CannotOptimize", "Cannot Optimize");
        StatusColor = FSlateColor(FLinearColor::Red);
    }

    return SNew(STableRow<TSharedPtr<FMaterialListItem>>, OwnerTable)
        [
            SNew(SHorizontalBox)

            // 复选框列
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked(this, &SMaterialOptimizerWidget::GetItemCheckState, Item)
                .OnCheckStateChanged(this, &SMaterialOptimizerWidget::OnItemCheckStateChanged, Item)
                .IsEnabled(Info.bCanOptimize && !Info.bIsInstance)
            ]

            // 材质名称列
            + SHorizontalBox::Slot()
            .FillWidth(0.4f)
            .Padding(5.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Info.Material.IsValid() ? Info.Material->GetName() : TEXT("Invalid")))
            ]

            // 类型列
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .Padding(5.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(TypeText)
            ]

            // 使用数量列
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .Padding(5.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::Format(LOCTEXT("UsageFormat", "{0} actors"), FText::AsNumber(Info.Actors.Num())))
            ]

            // 状态列
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .Padding(5.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(StatusText)
                .ColorAndOpacity(StatusColor)
            ]
        ];
}

void SMaterialOptimizerWidget::OnItemCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FMaterialListItem> Item)
{
    if (Item.IsValid())
    {
        Item->bSelected = (NewState == ECheckBoxState::Checked);
        UpdateStatistics();
    }
}

ECheckBoxState SMaterialOptimizerWidget::GetItemCheckState(TSharedPtr<FMaterialListItem> Item) const
{
    if (Item.IsValid())
    {
        return Item->bSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

void SMaterialOptimizerWidget::RefreshMaterialList()
{
    if (MaterialListView.IsValid())
    {
        MaterialListView->RequestListRefresh();
    }
}

void SMaterialOptimizerWidget::UpdateStatistics()
{
    int32 TotalCount = MaterialListItems.Num();
    int32 OptimizableCount = 0;
    int32 SelectedCount = 0;

    for (const TSharedPtr<FMaterialListItem>& Item : MaterialListItems)
    {
        if (Item.IsValid())
        {
            if (Item->MaterialInfo.bCanOptimize && !Item->MaterialInfo.bIsInstance)
            {
                OptimizableCount++;
            }
            if (Item->bSelected)
            {
                SelectedCount++;
            }
        }
    }

    if (TotalMaterialsText.IsValid())
    {
        TotalMaterialsText->SetText(FText::Format(
            LOCTEXT("TotalMaterialsFormat", "Total Materials: {0}"),
            FText::AsNumber(TotalCount)
        ));
    }

    if (OptimizableMaterialsText.IsValid())
    {
        OptimizableMaterialsText->SetText(FText::Format(
            LOCTEXT("OptimizableMaterialsFormat", "Optimizable: {0}"),
            FText::AsNumber(OptimizableCount)
        ));
    }

    if (SelectedMaterialsText.IsValid())
    {
        SelectedMaterialsText->SetText(FText::Format(
            LOCTEXT("SelectedMaterialsFormat", "Selected: {0}"),
            FText::AsNumber(SelectedCount)
        ));
    }
}

FMaterialOptimizerSettings SMaterialOptimizerWidget::GatherSettingsFromUI()
{
    FMaterialOptimizerSettings Settings;

    Settings.bOnlySelectedActors = OnlySelectedCheckBox.IsValid() && 
        OnlySelectedCheckBox->IsChecked();

    Settings.bAutoSave = AutoSaveCheckBox.IsValid() && 
        AutoSaveCheckBox->IsChecked();

    if (SavePathTextBox.IsValid())
    {
        Settings.SavePath = SavePathTextBox->GetText().ToString();
    }

    if (PrefixTextBox.IsValid())
    {
        Settings.InstancePrefix = PrefixTextBox->GetText().ToString();
    }

    if (SuffixTextBox.IsValid())
    {
        Settings.InstanceSuffix = SuffixTextBox->GetText().ToString();
    }

    return Settings;
}

TArray<FMaterialUsageInfo> SMaterialOptimizerWidget::GetSelectedMaterials() const
{
    TArray<FMaterialUsageInfo> SelectedMaterials;

    for (const TSharedPtr<FMaterialListItem>& Item : MaterialListItems)
    {
        if (Item.IsValid() && Item->bSelected)
        {
            SelectedMaterials.Add(Item->MaterialInfo);
        }
    }

    return SelectedMaterials;
}

#undef LOCTEXT_NAMESPACE