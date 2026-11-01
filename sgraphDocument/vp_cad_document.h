#pragma once

#include "vp_dimension_style_record.h"
#include "vp_document_audit.h"
#include "vp_drawing_settings.h"
#include "vp_entity.h"
#include "vp_layer_record.h"
#include "vp_result.h"
#include "vp_text_style_record.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <optional>
#include <vector>

namespace Vp
{

class VpDocumentTransaction;

// CAD 文档模型，持有图元、图层与撤销重做历史。
class VpCadDocument final : public QObject
{
    Q_OBJECT

  public:
    // 构造空文档。
    explicit VpCadDocument(QObject* parent = nullptr);

    // 返回全部图元。
    const std::vector<VpEntityRecord>& entities() const noexcept;
    // 返回文档文件路径。
    QString filePath() const;
    // 返回用于显示的文档名。
    QString displayName() const;
    // 返回是否有未保存的修改。
    bool isModified() const noexcept;
    // 返回是否可以撤销。
    bool canUndo() const noexcept;
    // 返回是否可以重做。
    bool canRedo() const noexcept;
    // 返回图纸设置。
    const VpDrawingSettings& drawingSettings() const noexcept;
    // 返回全部图层名称。
    QStringList layerNames() const;
    // 返回全部图层记录。
    const std::vector<VpLayerRecord>& layers() const noexcept;
    // 按名称查找图层，找不到返回空指针。
    const VpLayerRecord* layer(const QString& layer_name) const noexcept;
    // 返回当前图层名称。
    QString currentLayerName() const;
    // 设置首选绘图颜色。
    void setPreferredDrawingColor(const QColor& color);
    // 返回首选绘图颜色。
    QColor preferredDrawingColor() const;
    // 返回图层颜色。
    QColor layerColor(const QString& layer_name) const;
    // 返回图层线宽。
    double layerLineWidth(const QString& layer_name) const noexcept;
    // 返回图层线型。
    QString layerLineType(const QString& layer_name) const;
    // 返回图层透明度。
    int layerTransparency(const QString& layer_name) const noexcept;
    // 返回图层是否可见。
    bool isLayerVisible(const QString& layer_name) const noexcept;
    // 返回图层是否锁定。
    bool isLayerLocked(const QString& layer_name) const noexcept;
    // 返回图层是否冻结。
    bool isLayerFrozen(const QString& layer_name) const noexcept;
    // 新建图层。
    bool addLayer(const QString& layer_name);
    // 删除空图层。
    bool removeLayer(const QString& layer_name);
    // 重命名图层。
    bool renameLayer(const QString& layer_name, const QString& new_name);
    // 把源图层并入目标图层。
    bool mergeLayer(const QString& source_layer_name, const QString& target_layer_name);
    // 按颜色合并图层并返回合并数量。
    int mergeLayersByColor();
    // 删除图层及其上的图元。
    bool removeLayerWithContents(const QString& layer_name);
    // 把图元移入同色图层并返回图层名。
    QString assignEntitiesToColorLayer(const std::vector<VpEntityId>& entity_ids,
                                       const QColor& color);
    // 切换当前图层。
    bool setCurrentLayer(const QString& layer_name);
    // 设置图层可见性。
    bool setLayerVisible(const QString& layer_name, bool is_visible);
    // 设置图层锁定状态。
    bool setLayerLocked(const QString& layer_name, bool is_locked);
    // 设置图层冻结状态。
    bool setLayerFrozen(const QString& layer_name, bool is_frozen);
    // 只显示指定图层，隔离其他图层。
    bool isolateLayer(const QString& layer_name);
    // 返回已保存的图层状态名称。
    QStringList layerStateNames() const;
    // 把当前图层配置保存为状态。
    bool saveLayerState(const QString& state_name);
    // 恢复指定的图层状态。
    bool restoreLayerState(const QString& state_name);
    // 删除图层状态。
    bool removeLayerState(const QString& state_name);
    // 设置图层是否参与打印。
    bool setLayerPlottable(const QString& layer_name, bool is_plottable);
    // 设置图层颜色。
    bool setLayerColor(const QString& layer_name, const QColor& color);
    // 临时预览图层颜色，不记入历史。
    bool previewLayerColor(const QString& layer_name, const QColor& color);
    // 设置图层线宽。
    bool setLayerLineWidth(const QString& layer_name, double line_width_mm);
    // 设置图层线型。
    bool setLayerLineType(const QString& layer_name, const QString& line_type);
    // 设置图层透明度。
    bool setLayerTransparency(const QString& layer_name, int transparency);
    // 返回全部文字样式。
    const std::vector<VpTextStyleRecord>& textStyles() const noexcept;
    // 按名称查找文字样式。
    const VpTextStyleRecord* textStyle(const QString& style_name) const noexcept;
    // 返回当前文字样式名称。
    QString currentTextStyleName() const;
    // 新增或更新文字样式。
    bool addOrUpdateTextStyle(const VpTextStyleRecord& text_style);
    // 删除文字样式。
    bool removeTextStyle(const QString& style_name);
    // 切换当前文字样式。
    bool setCurrentTextStyle(const QString& style_name);
    // 返回全部标注样式。
    const std::vector<VpDimensionStyleRecord>& dimensionStyles() const noexcept;
    // 按名称查找标注样式。
    const VpDimensionStyleRecord* dimensionStyle(const QString& style_name) const noexcept;
    // 返回当前标注样式名称。
    QString currentDimensionStyleName() const;
    // 新增或更新标注样式。
    bool addOrUpdateDimensionStyle(const VpDimensionStyleRecord& dimension_style);
    // 删除标注样式。
    bool removeDimensionStyle(const QString& style_name);
    // 切换当前标注样式。
    bool setCurrentDimensionStyle(const QString& style_name);

    // 开启一次可撤销的编辑事务。
    std::unique_ptr<VpDocumentTransaction> beginTransaction(const QString& label);
    // 保存文档到文件。
    VpResult<void> save(const QString& file_path);
    // 另存一份恢复副本。
    VpResult<void> saveRecoveryCopy(const QString& file_path);
    // 从文件加载文档。
    VpResult<void> load(const QString& file_path);
    // 用恢复文件还原文档。
    VpResult<void> recover(const QString& recovery_file_path, const QString& original_file_path);
    // 检查文档，可选择就地修复。
    VpDocumentAuditReport audit(bool repair);
    // 清空文档内容。
    void clear();
    // 撤销一步。
    void undo();
    // 重做一步。
    void redo();

  signals:
    // 文档内容变化时发出。
    void documentChanged();
    // 文件路径变化时发出。
    void filePathChanged(const QString& file_path);
    // 修改状态变化时发出。
    void modifiedChanged(bool is_modified);
    // 撤销/重做可用性变化时发出。
    void historyChanged(bool can_undo, bool can_redo);
    // 图层集合变化时发出。
    void layersChanged();
    // 当前图层变化时发出。
    void currentLayerChanged(const QString& layer_name);
    // 图层状态集合变化时发出。
    void layerStatesChanged();
    // 图纸设置变化时发出。
    void drawingSettingsChanged();
    // 文字样式集合变化时发出。
    void textStylesChanged();
    // 当前文字样式变化时发出。
    void currentTextStyleChanged(const QString& style_name);
    // 标注样式集合变化时发出。
    void dimensionStylesChanged();
    // 当前标注样式变化时发出。
    void currentDimensionStyleChanged(const QString& style_name);

  private:
    friend class VpDocumentTransaction;

    // 撤销/重做栈中的一条可回放变更记录。
    struct VpHistoryEntry
    {
        QString label;
        std::vector<VpEntityRecord> added_entities;
        std::vector<VpEntityRecord> removed_entities;
        bool has_layer_change = false;
        std::vector<VpLayerRecord> layers_before;
        std::vector<VpLayerRecord> layers_after;
        QString current_layer_before;
        QString current_layer_after;
        bool has_drawing_settings_change = false;
        VpDrawingSettings drawing_settings_before;
        VpDrawingSettings drawing_settings_after;
        bool has_text_style_change = false;
        std::vector<VpTextStyleRecord> text_styles_before;
        std::vector<VpTextStyleRecord> text_styles_after;
        QString current_text_style_before;
        QString current_text_style_after;
        bool has_dimension_style_change = false;
        std::vector<VpDimensionStyleRecord> dimension_styles_before;
        std::vector<VpDimensionStyleRecord> dimension_styles_after;
        QString current_dimension_style_before;
        QString current_dimension_style_after;
        bool has_entity_order_change = false;
        std::vector<VpEntityId> entity_order_before;
        std::vector<VpEntityId> entity_order_after;
    };

    // 预留下一个图元标识。
    VpEntityId reserveEntityId() noexcept;
    // 预留下一个图层标识。
    VpLayerId reserveLayerId() noexcept;
    // 解析新图元应使用的图层名。
    QString resolveDrawingLayerName();
    // 确保 Defpoints 图层存在。
    void ensureDefpointsLayer();
    // 取得可写的图层对象，供内部修改。
    VpLayerRecord* mutableLayer(const QString& layer_name) noexcept;
    // 把图元变更写入撤销历史。
    void commitEntities(QString label, std::vector<VpEntityRecord> added_entities,
                        std::vector<VpEntityRecord> removed_entities,
                        std::optional<VpDrawingSettings> drawing_settings = std::nullopt,
                        std::optional<std::vector<VpEntityId>> entity_order = std::nullopt);
    // 把图层变更写入撤销历史。
    void commitLayerChange(QString label, std::vector<VpLayerRecord> layers_before,
                           QString current_layer_before, std::vector<VpEntityRecord> added_entities,
                           std::vector<VpEntityRecord> removed_entities);
    // 更新并广播修改状态。
    void setModified(bool is_modified);
    // 广播文档状态相关信号。
    void emitDocumentState();
    // 把文字样式变更写入撤销历史。
    void commitTextStyleChange(QString label, std::vector<VpTextStyleRecord> styles_before,
                               QString current_style_before);
    // 把标注样式变更写入撤销历史。
    void commitDimensionStyleChange(QString label,
                                    std::vector<VpDimensionStyleRecord> styles_before,
                                    QString current_style_before);
    // 保存的实现细节。
    VpResult<void> saveInternal(const QString& file_path, bool update_document_state);

    std::vector<VpEntityRecord> m_entities;
    std::vector<VpHistoryEntry> m_undo_stack;
    std::vector<VpHistoryEntry> m_redo_stack;
    VpEntityId m_next_entity_id = 1;
    VpLayerId m_next_layer_id = 1;
    QString m_file_path;
    std::vector<VpLayerRecord> m_layers{
        {0, QStringLiteral("0"), QColor(220, 228, 238), 0.25, true, false, true},
    };
    std::vector<VpLayerStateRecord> m_layer_states;
    QString m_current_layer_name = QStringLiteral("0");
    QColor m_preferred_drawing_color;
    VpDrawingSettings m_drawing_settings;
    std::vector<VpTextStyleRecord> m_text_styles{{}};
    QString m_current_text_style_name = QStringLiteral("Standard");
    std::vector<VpDimensionStyleRecord> m_dimension_styles{{}};
    QString m_current_dimension_style_name = QStringLiteral("Standard");
    bool m_is_modified = false;
};

} // namespace Vp
