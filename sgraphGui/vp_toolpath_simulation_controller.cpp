#include "vp_toolpath_simulation_controller.h"

#include "vp_cad_document.h"
#include "vp_cad_viewport.h"

#include <algorithm>
#include <array>
#include <unordered_set>

namespace Vp
{
namespace
{

constexpr std::array<double, 5> kSpeedMultipliers{0.25, 0.5, 1.0, 2.0, 4.0};

} // namespace

VpToolpathSimulationController::VpToolpathSimulationController(VpCadDocument* document,
                                                               VpCadViewport* viewport,
                                                               QObject* parent)
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
        connect(m_document, &VpCadDocument::documentChanged, this,
                [this]()
                {
                    stop();
                });
    }
    updateTimerInterval();
}

void VpToolpathSimulationController::play(const std::vector<VpEntityId>& selected_entity_ids)
{
    ensureMotions(selected_entity_ids);
    if (m_motions.empty())
    {
        return;
    }
    m_state = VpSimulationState::Running;
    m_timer.start();
    emit stateChanged(m_state);
}

void VpToolpathSimulationController::pause()
{
    if (m_state != VpSimulationState::Running)
    {
        return;
    }
    m_timer.stop();
    m_state = VpSimulationState::Paused;
    emit stateChanged(m_state);
}

void VpToolpathSimulationController::stop()
{
    m_timer.stop();
    m_motions.clear();
    m_completed_motion_count = 0;
    m_state = VpSimulationState::Stopped;
    if (m_viewport)
    {
        m_viewport->clearSimulationOverlay();
    }
    emit stateChanged(m_state);
}

void VpToolpathSimulationController::step(const std::vector<VpEntityId>& selected_entity_ids)
{
    ensureMotions(selected_entity_ids);
    if (m_motions.empty())
    {
        return;
    }
    m_timer.stop();
    m_state = VpSimulationState::Paused;
    advanceOneMotion();
    if (m_completed_motion_count < m_motions.size())
    {
        emit stateChanged(m_state);
    }
}

void VpToolpathSimulationController::speedUp()
{
    m_speed_index = std::min(m_speed_index + 1, 4);
    updateTimerInterval();
    emit speedChanged(speedMultiplier());
}

void VpToolpathSimulationController::speedDown()
{
    m_speed_index = std::max(m_speed_index - 1, 0);
    updateTimerInterval();
    emit speedChanged(speedMultiplier());
}

void VpToolpathSimulationController::setTraceVisible(bool is_visible)
{
    m_is_trace_visible = is_visible;
    refreshOverlay();
}

VpSimulationState VpToolpathSimulationController::state() const noexcept
{
    return m_state;
}

double VpToolpathSimulationController::speedMultiplier() const noexcept
{
    return kSpeedMultipliers[static_cast<std::size_t>(m_speed_index)];
}

std::size_t VpToolpathSimulationController::completedMotionCount() const noexcept
{
    return m_completed_motion_count;
}

std::size_t VpToolpathSimulationController::motionCount() const noexcept
{
    return m_motions.size();
}

bool VpToolpathSimulationController::isTraceVisible() const noexcept
{
    return m_is_trace_visible;
}

void VpToolpathSimulationController::ensureMotions(
    const std::vector<VpEntityId>& selected_entity_ids)
{
    if (!m_motions.empty() || !m_document || !m_viewport)
    {
        return;
    }
    const std::unordered_set<VpEntityId> selection(selected_entity_ids.begin(),
                                                   selected_entity_ids.end());
    std::vector<VpEntityRecord> entities;
    for (const VpEntityRecord& entity : m_document->entities())
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

void VpToolpathSimulationController::advanceOneMotion()
{
    if (m_completed_motion_count >= m_motions.size())
    {
        m_timer.stop();
        m_state = VpSimulationState::Stopped;
        emit stateChanged(m_state);
        return;
    }
    ++m_completed_motion_count;
    refreshOverlay();
    if (m_completed_motion_count >= m_motions.size())
    {
        m_timer.stop();
        m_state = VpSimulationState::Stopped;
        emit stateChanged(m_state);
    }
}

void VpToolpathSimulationController::refreshOverlay()
{
    if (m_viewport && !m_motions.empty())
    {
        m_viewport->setSimulationOverlay(m_motions, m_completed_motion_count, m_is_trace_visible);
    }
}

void VpToolpathSimulationController::updateTimerInterval()
{
    m_timer.setInterval(static_cast<int>(40.0 / speedMultiplier()));
}

} // namespace Vp
