#pragma once

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Vp
{

// Records expose an id member. Only IDs are copied; surviving records retain their order.
template <typename VpRecord, typename VpRemovedRecord>
std::size_t eraseRecordsById(std::vector<VpRecord>& records,
                             const std::vector<VpRemovedRecord>& records_to_erase)
{
    if (records.empty() || records_to_erase.empty())
    {
        return 0;
    }
    const std::size_t original_size = records.size();
    using VpId = std::decay_t<decltype(std::declval<const VpRecord&>().id)>;
    if (records_to_erase.size() == 1)
    {
        const VpId removed_id = records_to_erase.front().id;
        records.erase(std::remove_if(records.begin(), records.end(),
                                     [removed_id](const VpRecord& record)
                                     {
                                         return record.id == removed_id;
                                     }),
                      records.end());
        return original_size - records.size();
    }

    std::unordered_set<VpId> removed_ids;
    removed_ids.reserve(records_to_erase.size());
    for (const VpRemovedRecord& record : records_to_erase)
    {
        removed_ids.insert(record.id);
    }
    records.erase(std::remove_if(records.begin(), records.end(),
                                 [&removed_ids](const VpRecord& record)
                                 {
                                     return removed_ids.find(record.id) != removed_ids.end();
                                 }),
                  records.end());
    return original_size - records.size();
}

} // namespace Vp
