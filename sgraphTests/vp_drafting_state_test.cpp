#include "vp_drafting_state.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace Vp
{
namespace
{

void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void checkPoint(const VpPoint2d& actual, const VpPoint2d& expected, const char* message)
{
    check(distance(actual, expected) <= 1.0e-9, message);
}

void checkDefaultsAndSettings()
{
    VpDraftingState state;
    check(!state.isGridSnapEnabled(), "grid snap must default to disabled");
    check(!state.isOrthoEnabled(), "ortho must default to disabled");
    check(state.isTrackingEnabled(), "tracking must default to enabled");
    check(state.gridSnapSpacing() == 10.0, "default grid spacing changed");
    check(state.gridRotation() == 0.0, "default grid rotation changed");
    check(state.trackingPoints().empty(), "new drafting state must have no anchors");

    check(!state.setGridSnapEnabled(false), "unchanged grid mode must report no transition");
    check(state.setGridSnapEnabled(true), "grid mode transition was lost");
    check(!state.setGridSnapEnabled(true), "repeated grid mode must report no transition");
    check(state.setOrthoEnabled(true), "ortho mode transition was lost");
    check(!state.setOrthoEnabled(true), "repeated ortho mode must report no transition");

    check(state.setGridSnapSpacing(25.0), "valid grid spacing rejected");
    check(!state.setGridSnapSpacing(0.0), "zero grid spacing accepted");
    check(!state.setGridSnapSpacing(-1.0), "negative grid spacing accepted");
    check(!state.setGridSnapSpacing(1.0e-9), "degenerate grid spacing accepted");
    check(!state.setGridSnapSpacing(1.0e10), "oversized grid spacing accepted");
    check(!state.setGridSnapSpacing(std::numeric_limits<double>::infinity()),
          "infinite grid spacing accepted");
    check(!state.setGridSnapSpacing(std::numeric_limits<double>::quiet_NaN()),
          "NaN grid spacing accepted");
    check(state.gridSnapSpacing() == 25.0, "invalid settings altered grid spacing");
    check(state.setGridSnapSpacing(1.0e9), "maximum grid spacing rejected");
    check(state.setGridSnapSpacing(1.0e9), "valid repeated spacing must remain accepted");

    check(state.setGridRotation(-450.0), "negative grid rotation rejected");
    check(state.gridRotation() == 270.0, "negative grid rotation not normalized");
    check(state.setGridRotation(450.0), "large grid rotation rejected");
    check(state.gridRotation() == 90.0, "large grid rotation not normalized");
    check(!state.setGridRotation(std::numeric_limits<double>::infinity()),
          "infinite grid rotation accepted");
    check(!state.setGridRotation(std::numeric_limits<double>::quiet_NaN()),
          "NaN grid rotation accepted");
    check(state.gridRotation() == 90.0, "invalid settings altered grid rotation");
}

void checkGridAndOrtho()
{
    VpDraftingState state;
    checkPoint(state.constrainToGridAndOrtho({16.0, 14.0}, VpPoint2d{3.0, 4.0}), {16.0, 14.0},
               "disabled constraints changed the point");
    state.setGridSnapEnabled(true);
    checkPoint(state.constrainToGridAndOrtho({16.0, 14.0}, std::nullopt), {20.0, 10.0},
               "grid snapping changed");
    checkPoint(state.constrainToGridAndOrtho({-16.0, -14.0}, std::nullopt), {-20.0, -10.0},
               "negative grid snapping changed");
    state.setOrthoEnabled(true);
    checkPoint(state.constrainToGridAndOrtho({16.0, 14.0}, VpPoint2d{3.0, 4.0}), {20.0, 4.0},
               "ortho must constrain after grid snapping");
    checkPoint(state.constrainToGridAndOrtho({6.0, 16.0}, VpPoint2d{3.0, 4.0}), {3.0, 20.0},
               "vertical ortho changed");
    state.setGridSnapEnabled(false);
    checkPoint(state.constrainToGridAndOrtho({5.0, 5.0}, VpPoint2d{0.0, 0.0}), {5.0, 0.0},
               "equal ortho distances must prefer horizontal");
    checkPoint(state.constrainToGridAndOrtho({5.0, 5.0}, std::nullopt), {5.0, 5.0},
               "ortho without reference must leave point unchanged");
    state.setGridSnapEnabled(true);
    state.setGridRotation(45.0);
    const double diagonal = 10.0 / std::sqrt(2.0);
    checkPoint(state.constrainToGridAndOrtho({6.0, 8.0}, std::nullopt), {diagonal, diagonal},
               "rotated grid snapping changed");

    const VpPoint2d point{5.0, 7.0};
    checkPoint(snapPointToDraftingGrid(point, 10.0, {{1.0, 0.0}, {2.0, 0.0}}), point,
               "singular grid basis must leave point unchanged");
    checkPoint(snapPointToDraftingGrid(point, 0.0, draftingGridBasis(0.0)), point,
               "zero grid spacing must leave point unchanged");
}

void checkTrackingCandidates()
{
    VpDraftingState state;
    check(!state.trackingSnap({0.2, 20.0}, std::nullopt, 1.0),
          "tracking without anchors or reference produced a candidate");
    const auto reference_snap = state.trackingSnap({0.2, 20.0}, VpPoint2d{0.0, 0.0}, 1.0);
    check(reference_snap.has_value(), "reference point must supply a tracking anchor");
    checkPoint(*reference_snap, {0.0, 20.0}, "reference tracking changed");
    check(state.trackingPoints().empty(), "reference point must not become an acquired anchor");

    state.acquireTrackingPoint({0.0, 0.0});
    const auto vertical_snap = state.trackingSnap({0.2, 20.0}, std::nullopt, 1.0);
    check(vertical_snap.has_value(), "vertical tracking candidate missing");
    checkPoint(*vertical_snap, {0.0, 20.0}, "vertical tracking changed");
    const auto horizontal_snap = state.trackingSnap({20.0, 0.2}, std::nullopt, 1.0);
    check(horizontal_snap.has_value(), "horizontal tracking candidate missing");
    checkPoint(*horizontal_snap, {20.0, 0.0}, "horizontal tracking changed");
    const auto tie_snap = state.trackingSnap({0.5, 0.5}, std::nullopt, 1.0);
    check(tie_snap.has_value(), "equal-distance tracking candidate missing");
    checkPoint(*tie_snap, {0.5, 0.0}, "tracking ties must keep last matching candidate");
    check(!state.trackingSnap({2.0, 20.0}, std::nullopt, 1.0),
          "tracking accepted a point outside tolerance");
    check(!state.trackingSnap({0.0, 0.0}, std::nullopt, -1.0),
          "negative tolerance must not produce a candidate");
}

void checkTrackingAcquisitionAndReset()
{
    VpDraftingState state;
    for (int index = 0; index < 8; ++index)
    {
        state.acquireTrackingPoint({static_cast<double>(index), 20.0});
    }
    state.acquireTrackingPoint({0.5e-9, 20.0});
    check(state.trackingPoints().size() == 8, "near duplicate anchor changed capacity");
    checkPoint(state.trackingPoints().front(), {0.0, 20.0},
               "duplicate anchor must not change acquisition order");
    state.acquireTrackingPoint({8.0, 20.0});
    check(state.trackingPoints().size() == 8, "tracking anchor capacity exceeded eight");
    checkPoint(state.trackingPoints().front(), {1.0, 20.0},
               "tracking overflow must remove oldest anchor");
    checkPoint(state.trackingPoints().back(), {8.0, 20.0}, "new tracking anchor missing");

    check(state.setTrackingEnabled(false), "tracking disable transition missing");
    check(state.trackingPoints().empty(), "disabling tracking must clear acquired anchors");
    check(!state.trackingSnap({0.2, 20.0}, VpPoint2d{0.0, 0.0}, 1.0),
          "disabled tracking produced a reference candidate");

    // Direct object snaps still acquire anchors while tracking is disabled.
    state.acquireTrackingPoint({3.0, 4.0});
    check(!state.setTrackingEnabled(false), "repeated tracking disable must report no change");
    check(state.trackingPoints().size() == 1, "repeated disable changed acquired anchors");
    check(state.setTrackingEnabled(true), "tracking enable transition missing");
    const auto resumed_snap = state.trackingSnap({3.2, 20.0}, std::nullopt, 1.0);
    check(resumed_snap.has_value(), "re-enabled tracking lost acquired anchor");
    checkPoint(*resumed_snap, {3.0, 20.0}, "re-enabled tracking result changed");
    state.clearTrackingPoints();
    check(state.trackingPoints().empty(), "command or document reset must clear anchors");
    check(!state.trackingSnap({3.2, 20.0}, std::nullopt, 1.0),
          "cleared anchors still affected tracking");
}

} // namespace
} // namespace Vp

int vpRunDraftingStateTests()
{
    try
    {
        Vp::checkDefaultsAndSettings();
        Vp::checkGridAndOrtho();
        Vp::checkTrackingCandidates();
        Vp::checkTrackingAcquisitionAndReset();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Drafting state tests passed.\n";
    return 0;
}
