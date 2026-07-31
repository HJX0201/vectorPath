#include "s_polygon_boolean.h"

#include <clipper2/clipper.h>
#include <exception>

namespace smartCam
{
namespace
{

constexpr int kBooleanPrecision = 6;

Clipper2Lib::PathsD toClipperPaths(const SPolygonPaths& paths)
{
    Clipper2Lib::PathsD clipper_paths;
    clipper_paths.reserve(paths.size());
    for (const SPolygonPath& path : paths)
    {
        Clipper2Lib::PathD clipper_path;
        clipper_path.reserve(path.size());
        for (const SPoint2d& point : path)
        {
            clipper_path.emplace_back(point.x, point.y);
        }
        clipper_paths.push_back(std::move(clipper_path));
    }
    return clipper_paths;
}

SPolygonPaths fromClipperPaths(const Clipper2Lib::PathsD& paths)
{
    SPolygonPaths result;
    result.reserve(paths.size());
    for (const Clipper2Lib::PathD& path : paths)
    {
        SPolygonPath result_path;
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

bool hasInvalidPath(const SPolygonPaths& paths)
{
    for (const SPolygonPath& path : paths)
    {
        if (path.size() < 3)
        {
            return true;
        }
    }
    return false;
}

} // namespace

SResult<SPolygonPaths> polygonBoolean(const SPolygonPaths& subject_paths,
                                      const SPolygonPaths& clip_paths,
                                      SPolygonBooleanOperation operation)
{
    if (subject_paths.empty() || hasInvalidPath(subject_paths) || hasInvalidPath(clip_paths))
    {
        return SResult<SPolygonPaths>::failure(
            QStringLiteral("布尔运算需要至少一个有效的闭合多段线。"));
    }
    if (operation != SPolygonBooleanOperation::Union && clip_paths.empty())
    {
        return SResult<SPolygonPaths>::failure(
            QStringLiteral("该布尔运算需要至少两个闭合多段线。"));
    }

    try
    {
        const Clipper2Lib::PathsD subjects = toClipperPaths(subject_paths);
        const Clipper2Lib::PathsD clips = toClipperPaths(clip_paths);
        Clipper2Lib::PathsD result;
        switch (operation)
        {
        case SPolygonBooleanOperation::Union:
            result = Clipper2Lib::Union(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                        kBooleanPrecision);
            break;
        case SPolygonBooleanOperation::Intersection:
            result = Clipper2Lib::Intersect(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                            kBooleanPrecision);
            break;
        case SPolygonBooleanOperation::Difference:
            result = Clipper2Lib::Difference(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                             kBooleanPrecision);
            break;
        case SPolygonBooleanOperation::Xor:
            result = Clipper2Lib::Xor(subjects, clips, Clipper2Lib::FillRule::NonZero,
                                      kBooleanPrecision);
            break;
        case SPolygonBooleanOperation::Complement:
            result = Clipper2Lib::Difference(clips, subjects, Clipper2Lib::FillRule::NonZero,
                                             kBooleanPrecision);
            break;
        }
        return SResult<SPolygonPaths>::success(fromClipperPaths(result));
    }
    catch (const std::exception& exception)
    {
        return SResult<SPolygonPaths>::failure(
            QStringLiteral("Clipper2 布尔运算失败：%1")
                .arg(QString::fromLocal8Bit(exception.what())));
    }
    catch (...)
    {
        return SResult<SPolygonPaths>::failure(QStringLiteral("Clipper2 布尔运算失败。"));
    }
}

} // namespace smartCam
