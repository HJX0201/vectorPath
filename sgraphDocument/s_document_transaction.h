#pragma once

#include "s_drawing_settings.h"
#include "s_entity.h"
#include "s_layer_record.h"

#include <QColor>
#include <QString>
#include <optional>
#include <vector>

namespace vectorPath
{

class SCadDocument;

class SDocumentTransaction final
{
  public:
    SDocumentTransaction(SCadDocument& document, QString label);
    ~SDocumentTransaction();

    SDocumentTransaction(const SDocumentTransaction&) = delete;
    SDocumentTransaction& operator=(const SDocumentTransaction&) = delete;

    SEntityId addLine(const SPoint2d& start_point, const SPoint2d& end_point);
    SEntityId addCircle(const SPoint2d& center, double radius);
    SEntityId addArc(const SPoint2d& center, double radius, double start_angle, double end_angle);
    SEntityId addEllipse(const SPoint2d& center, const SPoint2d& major_axis,
                         const SPoint2d& minor_axis);
    SEntityId addSpline(const std::array<SPoint2d, 4>& control_points);
    SEntityId addPolyline(std::vector<SPoint2d> vertices, bool is_closed,
                          std::vector<double> bulges = {}, std::vector<double> start_widths = {},
                          std::vector<double> end_widths = {});
    SEntityId addText(const SPoint2d& position, QString text, double height = 2.5,
                      double rotation = 0.0,
                      STextHorizontalAlignment alignment = STextHorizontalAlignment::Left,
                      STextVerticalAlignment vertical_alignment = STextVerticalAlignment::Baseline);
    SEntityId addMText(const SPoint2d& position, QString rich_text, double width,
                       double height = 2.5, double rotation = 0.0,
                       STextHorizontalAlignment alignment = STextHorizontalAlignment::Left);
    SEntityId addLeader(std::vector<SPoint2d> vertices, QString text, double text_height = 2.5,
                        double arrow_size = 2.5);
    SEntityId addLinearDimension(const SPoint2d& first_point, const SPoint2d& second_point,
                                 const SPoint2d& dimension_line_point);
    SEntityId addDimension(SLinearDimensionEntity dimension);
    SEntityId addHatch(std::vector<SPoint2d> boundary);
    SEntityId addHatch(SHatchEntity hatch);
    QString ensureLayer(QString preferred_name, const QColor& color,
                        int transparency = 0);
    SEntityId addEntity(SEntityType type, SEntityGeometry geometry,
                        QString layer_name);
    SEntityId addEntityCopy(SEntityRecord entity, bool preserve_association = false);
    bool replaceEntity(SEntityId entity_id, SEntityRecord replacement);
    bool removeEntity(SEntityId entity_id);
    bool setEntityLayer(SEntityId entity_id, QString layer_name);
    bool setEntityLineWidth(SEntityId entity_id, double line_width_mm);
    bool setDrawingSettings(const SDrawingSettings& settings);
    bool setEntityOrder(std::vector<SEntityId> entity_order);
    void commit();

  private:
    QString creationLayerName();

    SCadDocument& m_document;
    QString m_label;
    std::vector<SEntityRecord> m_entities;
    std::vector<SEntityRecord> m_removed_entities;
    std::vector<SLayerRecord> m_added_layers;
    std::optional<SDrawingSettings> m_drawing_settings;
    std::optional<std::vector<SEntityId>> m_entity_order;
    bool m_is_committed = false;
};

} // namespace vectorPath
