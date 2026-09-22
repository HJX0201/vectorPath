#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Vp
{

struct VpCollectionRecord
{
    std::uint64_t id = 0;
    std::vector<double> geometry;
    inline static std::size_t copies = 0;
    inline static std::size_t copied_payload_bytes = 0;
    inline static std::size_t move_assignments = 0;

    explicit VpCollectionRecord(std::uint64_t record_id, std::size_t point_count = 128)
        : id(record_id), geometry(point_count, static_cast<double>(record_id))
    {
    }

    VpCollectionRecord(const VpCollectionRecord& other) : id(other.id), geometry(other.geometry)
    {
        ++copies;
        copied_payload_bytes += geometry.size() * sizeof(double);
    }

    VpCollectionRecord& operator=(const VpCollectionRecord& other)
    {
        id = other.id;
        geometry = other.geometry;
        ++copies;
        copied_payload_bytes += geometry.size() * sizeof(double);
        return *this;
    }

    VpCollectionRecord(VpCollectionRecord&& other) noexcept = default;

    VpCollectionRecord& operator=(VpCollectionRecord&& other) noexcept
    {
        id = other.id;
        geometry = std::move(other.geometry);
        ++move_assignments;
        return *this;
    }

    static void resetCounters()
    {
        copies = 0;
        copied_payload_bytes = 0;
        move_assignments = 0;
    }
};

struct VpCollectionId
{
    std::uint64_t id = 0;
};

} // namespace Vp
