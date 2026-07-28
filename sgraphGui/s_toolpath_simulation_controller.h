#pragma once

#include "s_toolpath.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <vector>

namespace smartGraphics
{

class SCadDocument;
class SCadViewport;

enum class SSimulationState
{
    Stopped,
    Paused,
    Running
};

class SToolpathSimulationController final : public QObject
{
    Q_OBJECT

  public:
    SToolpathSimulationController(SCadDocument* document, SCadViewport* viewport,
                                  QObject* parent = nullptr);

    void play(const std::vector<SEntityId>& selected_entity_ids = {});
    void pause();
    void stop();
    void step(const std::vector<SEntityId>& selected_entity_ids = {});
    void speedUp();
    void speedDown();
    void setTraceVisible(bool is_visible);
    SSimulationState state() const noexcept;
    double speedMultiplier() const noexcept;
    std::size_t completedMotionCount() const noexcept;
    std::size_t motionCount() const noexcept;
    bool isTraceVisible() const noexcept;

  signals:
    void stateChanged(smartGraphics::SSimulationState state);
    void speedChanged(double multiplier);

  private:
    void ensureMotions(const std::vector<SEntityId>& selected_entity_ids);
    void advanceOneMotion();
    void refreshOverlay();
    void updateTimerInterval();

    QPointer<SCadDocument> m_document;
    QPointer<SCadViewport> m_viewport;
    QTimer m_timer;
    std::vector<SToolpathMotion> m_motions;
    std::size_t m_completed_motion_count = 0;
    int m_speed_index = 2;
    bool m_is_trace_visible = true;
    SSimulationState m_state = SSimulationState::Stopped;
};

} // namespace smartGraphics
