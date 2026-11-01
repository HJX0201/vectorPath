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

// 一次可撤销编辑的事务入口，登记图元与图层变更。
class VpDocumentTransaction final
{
  public:
    // 开启一次编辑事务，析构时提交或回滚。
    VpDocumentTransaction(VpCadDocument& document, QString label);
    // 析构：未提交时回滚事务。
    ~VpDocumentTransaction();

    // 事务不可拷贝。
    VpDocumentTransaction(const VpDocumentTransaction&) = delete;
    VpDocumentTransaction& operator=(const VpDocumentTransaction&) = delete;

    // 新建直线。
    VpEntityId addLine(const VpPoint2d& start_point, const VpPoint2d& end_point);
    // 新建圆。
    VpEntityId addCircle(const VpPoint2d& center, double radius);
    // 新建圆弧。
    VpEntityId addArc(const VpPoint2d& center, double radius, double start_angle, double end_angle);
    // 新建椭圆。
    VpEntityId addEllipse(const VpPoint2d& center, const VpPoint2d& major_axis,
                          const VpPoint2d& minor_axis);
    // 新建样条。
    VpEntityId addSpline(const std::array<VpPoint2d, 4>& control_points);
    // 新建多段线。
    VpEntityId addPolyline(std::vector<VpPoint2d> vertices, bool is_closed,
                           std::vector<double> bulges = {}, std::vector<double> start_widths = {},
                           std::vector<double> end_widths = {});
    // 新建单行文字。
    VpEntityId
    addText(const VpPoint2d& position, QString text, double height = 2.5, double rotation = 0.0,
            VpTextHorizontalAlignment alignment = VpTextHorizontalAlignment::Left,
            VpTextVerticalAlignment vertical_alignment = VpTextVerticalAlignment::Baseline);
    // 新建多行文字。
    VpEntityId addMText(const VpPoint2d& position, QString rich_text, double width,
                        double height = 2.5, double rotation = 0.0,
                        VpTextHorizontalAlignment alignment = VpTextHorizontalAlignment::Left);
    // 新建引线。
    VpEntityId addLeader(std::vector<VpPoint2d> vertices, QString text, double text_height = 2.5,
                         double arrow_size = 2.5);
    // 新建线性标注。
    VpEntityId addLinearDimension(const VpPoint2d& first_point, const VpPoint2d& second_point,
                                  const VpPoint2d& dimension_line_point);
    // 按完整定义新建标注。
    VpEntityId addDimension(VpLinearDimensionEntity dimension);
    // 用边界新建填充。
    VpEntityId addHatch(std::vector<VpPoint2d> boundary);
    // 按完整定义新建填充。
    VpEntityId addHatch(VpHatchEntity hatch);
    // 取得或新建图层。
    QString ensureLayer(QString preferred_name, const QColor& color, int transparency = 0);
    // 按类型与几何新建图元。
    VpEntityId addEntity(VpEntityType type, VpEntityGeometry geometry, QString layer_name);
    // 复制已有图元为新图元。
    VpEntityId addEntityCopy(VpEntityRecord entity, bool preserve_association = false);
    // 用新记录替换图元。
    bool replaceEntity(VpEntityId entity_id, VpEntityRecord replacement);
    // 删除图元。
    bool removeEntity(VpEntityId entity_id);
    // 修改图元所属图层。
    bool setEntityLayer(VpEntityId entity_id, QString layer_name);
    // 修改图元线宽。
    bool setEntityLineWidth(VpEntityId entity_id, double line_width_mm);
    // 修改图纸设置。
    bool setDrawingSettings(const VpDrawingSettings& settings);
    // 重排图元绘制顺序。
    bool setEntityOrder(std::vector<VpEntityId> entity_order);
    // 提交事务并写入撤销历史。
    void commit();

  private:
    // 返回新建图元默认使用的图层名。
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
