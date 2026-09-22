#pragma once

#include "vp_drawing_settings.h"
#include "vp_entity.h"
#include "vp_layer_record.h"

#include <QColor>
#include <QString>
#include <optional>
#include <vector>

namespace Vp
{

class VpCadDocument;

class VpDocumentTransaction final
{
  public:
    VpDocumentTransaction(VpCadDocument& document, QString label);
    ~VpDocumentTransaction();

    VpDocumentTransaction(const VpDocumentTransaction&) = delete;
    VpDocumentTransaction& operator=(const VpDocumentTransaction&) = delete;

    VpEntityId addLine(const VpPoint2d& start_point, const VpPoint2d& end_point);
    VpEntityId addCircle(const VpPoint2d& center, double radius);
    VpEntityId addArc(const VpPoint2d& center, double radius, double start_angle, double end_angle);
    VpEntityId addEllipse(const VpPoint2d& center, const VpPoint2d& major_axis,
                          const VpPoint2d& minor_axis);
    VpEntityId addSpline(const std::array<VpPoint2d, 4>& control_points);
    VpEntityId addPolyline(std::vector<VpPoint2d> vertices, bool is_closed,
                           std::vector<double> bulges = {}, std::vector<double> start_widths = {},
                           std::vector<double> end_widths = {});
    VpEntityId
    addText(const VpPoint2d& position, QString text, double height = 2.5, double rotation = 0.0,
            VpTextHorizontalAlignment alignment = VpTextHorizontalAlignment::Left,
            VpTextVerticalAlignment vertical_alignment = VpTextVerticalAlignment::Baseline);
    VpEntityId addMText(const VpPoint2d& position, QString rich_text, double width,
                        double height = 2.5, double rotation = 0.0,
                        VpTextHorizontalAlignment alignment = VpTextHorizontalAlignment::Left);
    VpEntityId addLeader(std::vector<VpPoint2d> vertices, QString text, double text_height = 2.5,
                         double arrow_size = 2.5);
    VpEntityId addLinearDimension(const VpPoint2d& first_point, const VpPoint2d& second_point,
                                  const VpPoint2d& dimension_line_point);
    VpEntityId addDimension(VpLinearDimensionEntity dimension);
    VpEntityId addHatch(std::vector<VpPoint2d> boundary);
    VpEntityId addHatch(VpHatchEntity hatch);
    QString ensureLayer(QString preferred_name, const QColor& color, int transparency = 0);
    VpEntityId addEntity(VpEntityType type, VpEntityGeometry geometry, QString layer_name);
    VpEntityId addEntityCopy(VpEntityRecord entity, bool preserve_association = false);
    bool replaceEntity(VpEntityId entity_id, VpEntityRecord replacement);
    bool removeEntity(VpEntityId entity_id);
    bool setEntityLayer(VpEntityId entity_id, QString layer_name);
    bool setEntityLineWidth(VpEntityId entity_id, double line_width_mm);
    bool setDrawingSettings(const VpDrawingSettings& settings);
    bool setEntityOrder(std::vector<VpEntityId> entity_order);
    void commit();

  private:
    QString creationLayerName();

    VpCadDocument& m_document;
    QString m_label;
    std::vector<VpEntityRecord> m_entities;
    std::vector<VpEntityRecord> m_removed_entities;
    std::vector<VpLayerRecord> m_added_layers;
    std::optional<VpDrawingSettings> m_drawing_settings;
    std::optional<std::vector<VpEntityId>> m_entity_order;
    bool m_is_committed = false;
};

} // namespace Vp
