#pragma once

#include "vp_toolpath.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <vector>

namespace Vp
{

class VpCadDocument;
class VpCadViewport;

enum class VpSimulationState
{
    Stopped,
    Paused,
    Running
};

// 刀具路径仿真的播放、暂停与步进控制。
class VpToolpathSimulationController final : public QObject
{
    Q_OBJECT

  public:
    // 构造仿真控制器。
    VpToolpathSimulationController(VpCadDocument* document, VpCadViewport* viewport,
                                   QObject* parent = nullptr);

    // 开始或继续仿真。
    void play(const std::vector<VpEntityId>& selected_entity_ids = {});
    // 暂停仿真。
    void pause();
    // 停止仿真并复位。
    void stop();
    // 单步前进一段运动。
    void step(const std::vector<VpEntityId>& selected_entity_ids = {});
    // 提高仿真倍速。
    void speedUp();
    // 降低仿真倍速。
    void speedDown();
    // 设置是否显示走刀轨迹。
    void setTraceVisible(bool is_visible);
    // 返回当前仿真状态。
    VpSimulationState state() const noexcept;
    // 返回当前倍速。
    double speedMultiplier() const noexcept;
    // 返回已完成的运动段数。
    std::size_t completedMotionCount() const noexcept;
    // 返回总运动段数。
    std::size_t motionCount() const noexcept;
    // 返回走刀轨迹是否可见。
    bool isTraceVisible() const noexcept;

  signals:
    // 仿真状态变化时发出。
    void stateChanged(Vp::VpSimulationState state);
    // 倍速变化时发出。
    void speedChanged(double multiplier);

  private:
    // 按选中图元准备运动段列表。
    void ensureMotions(const std::vector<VpEntityId>& selected_entity_ids);
    // 前进一段运动。
    void advanceOneMotion();
    // 刷新视口上的轨迹叠加层。
    void refreshOverlay();
    // 按倍速换算定时器间隔。
    void updateTimerInterval();

    QPointer<VpCadDocument> m_document;
    QPointer<VpCadViewport> m_viewport;
    QTimer m_timer;
    std::vector<VpToolpathMotion> m_motions;
    std::size_t m_completed_motion_count = 0;
    int m_speed_index = 2;
    bool m_is_trace_visible = true;
    VpSimulationState m_state = VpSimulationState::Stopped;
};

} // namespace Vp
