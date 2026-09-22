#include "vp_id_collection.h"
#include "vp_id_collection_fixture.h"

#include <chrono>
#include <iostream>

namespace Vp
{
namespace
{

void eraseSequentially(std::vector<VpCollectionRecord>& records,
                       const std::vector<VpCollectionRecord>& removed)
{
    for (const VpCollectionRecord& record : removed)
    {
        records.erase(std::remove_if(records.begin(), records.end(),
                                     [record](const VpCollectionRecord& candidate)
                                     {
                                         return candidate.id == record.id;
                                     }),
                      records.end());
    }
}

template <typename VpOperation>
void measure(const char* name, std::vector<VpCollectionRecord>& records,
             const std::vector<VpCollectionRecord>& removed, VpOperation operation)
{
    VpCollectionRecord::resetCounters();
    const auto start = std::chrono::steady_clock::now();
    operation(records, removed);
    const double milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << name << ',' << milliseconds << ',' << VpCollectionRecord::copies << ','
              << VpCollectionRecord::copied_payload_bytes << ','
              << VpCollectionRecord::move_assignments << '\n';
}

} // namespace
} // namespace Vp

int runSample(std::size_t record_count)
{
    const std::size_t removal_count = (record_count + 4) / 5;
    std::vector<Vp::VpCollectionRecord> sequential;
    std::vector<Vp::VpCollectionRecord> removed;
    sequential.reserve(record_count);
    removed.reserve(removal_count);
    for (std::size_t index = 0; index < record_count; ++index)
    {
        sequential.emplace_back(index);
        if (index % 5 == 0)
        {
            removed.emplace_back(index);
        }
    }
    std::vector<Vp::VpCollectionRecord> indexed = sequential;
    std::cout << "records=" << record_count << ", removed=" << removal_count
              << ", doubles_per_geometry=128\n"
              << "strategy,milliseconds,record_copies,cumulative_copied_payload_bytes,"
                 "move_assignments\n";
    Vp::measure("sequential_reference", sequential, removed, Vp::eraseSequentially);
    Vp::measure("indexed_stable", indexed, removed,
                [](auto& records, const auto& records_to_erase)
                {
                    Vp::eraseRecordsById(records, records_to_erase);
                });
    const bool same_result =
        sequential.size() == indexed.size() &&
        std::equal(sequential.begin(), sequential.end(), indexed.begin(),
                   [](const auto& first, const auto& second)
                   {
                       return first.id == second.id && first.geometry == second.geometry;
                   });
    std::cout << "results_equal=" << same_result
              << "\nCopied payload bytes are cumulative copy traffic, not peak memory.\n";
    return same_result ? 0 : 1;
}

int main()
{
    for (std::size_t record_count : {1000U, 10000U, 30000U})
    {
        if (runSample(record_count) != 0)
        {
            return 1;
        }
    }
    return 0;
}
