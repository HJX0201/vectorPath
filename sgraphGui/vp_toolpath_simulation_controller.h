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

class VpToolpathSimulationController final : public QObject
{
    Q_OBJECT

  public:
    VpToolpathSimulationController(VpCadDocument* document, VpCadViewport* viewport,
                                   QObject* parent = nullptr);

    void play(const std::vector<VpEntityId>& selected_entity_ids = {});
    void pause();
    void stop();
    void step(const std::vector<VpEntityId>& selected_entity_ids = {});
    void speedUp();
    void speedDown();
    void setTraceVisible(bool is_visible);
    VpSimulationState state() const noexcept;
    double speedMultiplier() const noexcept;
    std::size_t completedMotionCount() const noexcept;
    std::size_t motionCount() const noexcept;
    bool isTraceVisible() const noexcept;

  signals:
    void stateChanged(Vp::VpSimulationState state);
    void speedChanged(double multiplier);

  private:
    void ensureMotions(const std::vector<VpEntityId>& selected_entity_ids);
    void advanceOneMotion();
    void refreshOverlay();
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
