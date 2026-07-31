#include "s_toolpath_simulation_controller.h"

#include "s_cad_document.h"
#include "s_cad_viewport.h"

#include <algorithm>
#include <array>
#include <unordered_set>

namespace vectorPath
{
namespace
{

constexpr std::array<double, 5> kSpeedMultipliers{0.25, 0.5, 1.0, 2.0, 4.0};

} // namespace

SToolpathSimulationController::SToolpathSimulationController(
    SCadDocument* document, SCadViewport* viewport, QObject* parent)
    : QObject(parent), m_document(document), m_viewport(viewport)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this,
            [this]()
            {
                advanceOneMotion();
            });
    if (m_document)
    {
        connect(m_document, &SCadDocument::documentChanged, this,
                [this]()
                {
                    stop();
                });
    }
    updateTimerInterval();
}

void SToolpathSimulationController::play(
    const std::vector<SEntityId>& selected_entity_ids)
{
    ensureMotions(selected_entity_ids);
    if (m_motions.empty())
    {
        return;
    }
    m_state = SSimulationState::Running;
    m_timer.start();
    emit stateChanged(m_state);
}

void SToolpathSimulationController::pause()
{
    if (m_state != SSimulationState::Running)
    {
        return;
    }
    m_timer.stop();
    m_state = SSimulationState::Paused;
    emit stateChanged(m_state);
}

void SToolpathSimulationController::stop()
{
    m_timer.stop();
    m_motions.clear();
    m_completed_motion_count = 0;
    m_state = SSimulationState::Stopped;
    if (m_viewport)
    {
        m_viewport->clearSimulationOverlay();
    }
    emit stateChanged(m_state);
}

void SToolpathSimulationController::step(
    const std::vector<SEntityId>& selected_entity_ids)
{
    ensureMotions(selected_entity_ids);
    if (m_motions.empty())
    {
        return;
    }
    m_timer.stop();
    m_state = SSimulationState::Paused;
    advanceOneMotion();
    if (m_completed_motion_count < m_motions.size())
    {
        emit stateChanged(m_state);
    }
}

void SToolpathSimulationController::speedUp()
{
    m_speed_index = std::min(m_speed_index + 1, 4);
    updateTimerInterval();
    emit speedChanged(speedMultiplier());
}

void SToolpathSimulationController::speedDown()
{
    m_speed_index = std::max(m_speed_index - 1, 0);
    updateTimerInterval();
    emit speedChanged(speedMultiplier());
}

void SToolpathSimulationController::setTraceVisible(bool is_visible)
{
    m_is_trace_visible = is_visible;
    refreshOverlay();
}

SSimulationState SToolpathSimulationController::state() const noexcept
{
    return m_state;
}

double SToolpathSimulationController::speedMultiplier() const noexcept
{
    return kSpeedMultipliers[static_cast<std::size_t>(m_speed_index)];
}

std::size_t SToolpathSimulationController::completedMotionCount() const noexcept
{
    return m_completed_motion_count;
}

std::size_t SToolpathSimulationController::motionCount() const noexcept
{
    return m_motions.size();
}

bool SToolpathSimulationController::isTraceVisible() const noexcept
{
    return m_is_trace_visible;
}

void SToolpathSimulationController::ensureMotions(
    const std::vector<SEntityId>& selected_entity_ids)
{
    if (!m_motions.empty() || !m_document || !m_viewport)
    {
        return;
    }
    const std::unordered_set<SEntityId> selection(selected_entity_ids.begin(),
                                                   selected_entity_ids.end());
    std::vector<SEntityRecord> entities;
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (isMachinableEntity(entity) &&
            (selection.empty() || selection.find(entity.id) != selection.end()))
        {
            entities.push_back(entity);
        }
    }
    m_motions = generateToolpathMotions(entities, m_viewport->visibleWorldBottomLeft());
    refreshOverlay();
}

void SToolpathSimulationController::advanceOneMotion()
{
    if (m_completed_motion_count >= m_motions.size())
    {
        m_timer.stop();
        m_state = SSimulationState::Stopped;
        emit stateChanged(m_state);
        return;
    }
    ++m_completed_motion_count;
    refreshOverlay();
    if (m_completed_motion_count >= m_motions.size())
    {
        m_timer.stop();
        m_state = SSimulationState::Stopped;
        emit stateChanged(m_state);
    }
}

void SToolpathSimulationController::refreshOverlay()
{
    if (m_viewport && !m_motions.empty())
    {
        m_viewport->setSimulationOverlay(m_motions, m_completed_motion_count,
                                         m_is_trace_visible);
    }
}

void SToolpathSimulationController::updateTimerInterval()
{
    m_timer.setInterval(static_cast<int>(40.0 / speedMultiplier()));
}

} // namespace vectorPath
