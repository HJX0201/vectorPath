#include "vp_polygon_boolean.h"

#include <clipper2/clipper.h>
#include <exception>

namespace Vp
{
namespace
{

constexpr int kBooleanPrecision = 6;

Clipper2Lib::PathsD toClipperPaths(const VpPolygonPaths& paths)
{
    Clipper2Lib::PathsD clipper_paths;
    clipper_paths.reserve(paths.size());
    for (const VpPolygonPath& path : paths)
    {
        Clipper2Lib::PathD clipper_path;
        clipper_path.reserve(path.size());
        for (const VpPoint2d& point : path)
        {
            clipper_path.emplace_back(point.x, point.y);
        }
        clipper_paths.push_back(std::move(clipper_path));
    }
    return clipper_paths;
}

VpPolygonPaths fromClipperPaths(const Clipper2Lib::PathsD& paths)
{
    VpPolygonPaths result;
    result.reserve(paths.size());
    for (const Clipper2Lib::PathD& path : paths)
    {
        VpPolygonPath result_path;
        result_path.reserve(path.size());
        for (const Clipper2Lib::PointD& point : path)
        {
            result_path.push_back({point.x, point.y});
        }
        if (result_path.size() >= 3)
        {
            result.push_back(std::move(result_path));
        }
    }
    return result;
}

bool hasInvalidPath(const VpPolygonPaths& paths)
{
    for (const VpPolygonPath& path : paths)
    {
        if (path.size() < 3)
        {
            return true;
        }
    }
    return false;
}

} // namespace

VpResult<VpPolygonPaths> polygonBoolean(const VpPolygonPaths& subject_paths,
                                        const VpPolygonPaths& clip_paths,
                                        VpPolygonBooleanOperation operation)
{
    if (subject_paths.empty() || hasInvalidPath(subject_paths) || hasInvalidPath(clip_paths))
    {
        return VpResult<VpPolygonPaths>::failure(u"布尔运算需要至少一个有效的闭合多段线。");
    }
    if (operation != VpPolygonBooleanOperation::Union && clip_paths.empty())
    {
        return VpResult<VpPolygonPaths>::failure(u"该布尔运算需要至少两个闭合多段线。");
    }

    try
    {
        const Clipper2Lib::PathsD subjects = toClipperPaths(subject_paths);
        const Clipper2Lib::PathsD clips = toClipperPaths(clip_paths);
        Clipper2Lib::PathsD result;
        switch (operation)
        {
        case VpPolygonBooleanOperation::Union:
            result = Clipper2Lib::Union(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                        kBooleanPrecision);
            break;
        case VpPolygonBooleanOperation::Intersection:
            result = Clipper2Lib::Intersect(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                            kBooleanPrecision);
            break;
        case VpPolygonBooleanOperation::Difference:
            result = Clipper2Lib::Difference(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                             kBooleanPrecision);
            break;
        case VpPolygonBooleanOperation::Xor:
            result = Clipper2Lib::Xor(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                      kBooleanPrecision);
            break;
        case VpPolygonBooleanOperation::Complement:
            result = Clipper2Lib::Difference(clips, subjects, Clipper2Lib::FillRule::NonZero,
                                             kBooleanPrecision);
            break;
        }
        return VpResult<VpPolygonPaths>::success(fromClipperPaths(result));
    }
    catch (const std::exception& exception)
    {
        return VpResult<VpPolygonPaths>::failureWithNativeDetail(u"Clipper2 布尔运算失败：",
                                                                 exception.what());
    }
    catch (...)
    {
        return VpResult<VpPolygonPaths>::failure(u"Clipper2 布尔运算失败。");
    }
}

} // namespace Vp
