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

class VpCadDocument final : public QObject
{
    Q_OBJECT

  public:
    explicit VpCadDocument(QObject* parent = nullptr);

    const std::vector<VpEntityRecord>& entities() const noexcept;
    QString filePath() const;
    QString displayName() const;
    bool isModified() const noexcept;
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    const VpDrawingSettings& drawingSettings() const noexcept;
    QStringList layerNames() const;
    const std::vector<VpLayerRecord>& layers() const noexcept;
    const VpLayerRecord* layer(const QString& layer_name) const noexcept;
    QString currentLayerName() const;
    void setPreferredDrawingColor(const QColor& color);
    QColor preferredDrawingColor() const;
    QColor layerColor(const QString& layer_name) const;
    double layerLineWidth(const QString& layer_name) const noexcept;
    QString layerLineType(const QString& layer_name) const;
    int layerTransparency(const QString& layer_name) const noexcept;
    bool isLayerVisible(const QString& layer_name) const noexcept;
    bool isLayerLocked(const QString& layer_name) const noexcept;
    bool isLayerFrozen(const QString& layer_name) const noexcept;
    bool addLayer(const QString& layer_name);
    bool removeLayer(const QString& layer_name);
    bool renameLayer(const QString& layer_name, const QString& new_name);
    bool mergeLayer(const QString& source_layer_name, const QString& target_layer_name);
    int mergeLayersByColor();
    bool removeLayerWithContents(const QString& layer_name);
    QString assignEntitiesToColorLayer(const std::vector<VpEntityId>& entity_ids,
                                       const QColor& color);
    bool setCurrentLayer(const QString& layer_name);
    bool setLayerVisible(const QString& layer_name, bool is_visible);
    bool setLayerLocked(const QString& layer_name, bool is_locked);
    bool setLayerFrozen(const QString& layer_name, bool is_frozen);
    bool isolateLayer(const QString& layer_name);
    QStringList layerStateNames() const;
    bool saveLayerState(const QString& state_name);
    bool restoreLayerState(const QString& state_name);
    bool removeLayerState(const QString& state_name);
    bool setLayerPlottable(const QString& layer_name, bool is_plottable);
    bool setLayerColor(const QString& layer_name, const QColor& color);
    bool previewLayerColor(const QString& layer_name, const QColor& color);
    bool setLayerLineWidth(const QString& layer_name, double line_width_mm);
    bool setLayerLineType(const QString& layer_name, const QString& line_type);
    bool setLayerTransparency(const QString& layer_name, int transparency);
    const std::vector<VpTextStyleRecord>& textStyles() const noexcept;
    const VpTextStyleRecord* textStyle(const QString& style_name) const noexcept;
    QString currentTextStyleName() const;
    bool addOrUpdateTextStyle(const VpTextStyleRecord& text_style);
    bool removeTextStyle(const QString& style_name);
    bool setCurrentTextStyle(const QString& style_name);
    const std::vector<VpDimensionStyleRecord>& dimensionStyles() const noexcept;
    const VpDimensionStyleRecord* dimensionStyle(const QString& style_name) const noexcept;
    QString currentDimensionStyleName() const;
    bool addOrUpdateDimensionStyle(const VpDimensionStyleRecord& dimension_style);
    bool removeDimensionStyle(const QString& style_name);
    bool setCurrentDimensionStyle(const QString& style_name);

    std::unique_ptr<VpDocumentTransaction> beginTransaction(const QString& label);
    VpResult<void> save(const QString& file_path);
    VpResult<void> saveRecoveryCopy(const QString& file_path);
    VpResult<void> load(const QString& file_path);
    VpResult<void> recover(const QString& recovery_file_path, const QString& original_file_path);
    VpDocumentAuditReport audit(bool repair);
    void clear();
    void undo();
    void redo();

  signals:
    void documentChanged();
    void filePathChanged(const QString& file_path);
    void modifiedChanged(bool is_modified);
    void historyChanged(bool can_undo, bool can_redo);
    void layersChanged();
    void currentLayerChanged(const QString& layer_name);
    void layerStatesChanged();
    void drawingSettingsChanged();
    void textStylesChanged();
    void currentTextStyleChanged(const QString& style_name);
    void dimensionStylesChanged();
    void currentDimensionStyleChanged(const QString& style_name);

  private:
    friend class VpDocumentTransaction;

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

    VpEntityId reserveEntityId() noexcept;
    VpLayerId reserveLayerId() noexcept;
    QString resolveDrawingLayerName();
    void ensureDefpointsLayer();
    VpLayerRecord* mutableLayer(const QString& layer_name) noexcept;
    void commitEntities(QString label, std::vector<VpEntityRecord> added_entities,
                        std::vector<VpEntityRecord> removed_entities,
                        std::optional<VpDrawingSettings> drawing_settings = std::nullopt,
                        std::optional<std::vector<VpEntityId>> entity_order = std::nullopt);
    void commitLayerChange(QString label, std::vector<VpLayerRecord> layers_before,
                           QString current_layer_before, std::vector<VpEntityRecord> added_entities,
                           std::vector<VpEntityRecord> removed_entities);
    void setModified(bool is_modified);
    void emitDocumentState();
    void commitTextStyleChange(QString label, std::vector<VpTextStyleRecord> styles_before,
                               QString current_style_before);
    void commitDimensionStyleChange(QString label,
                                    std::vector<VpDimensionStyleRecord> styles_before,
                                    QString current_style_before);
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
