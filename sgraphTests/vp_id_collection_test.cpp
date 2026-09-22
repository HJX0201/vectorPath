#include "vp_id_collection.h"
#include "vp_id_collection_fixture.h"

#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace Vp
{
namespace
{

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::vector<std::uint64_t> ids(const std::vector<VpCollectionRecord>& records)
{
    std::vector<std::uint64_t> result;
    for (const VpCollectionRecord& record : records)
    {
        result.push_back(record.id);
    }
    return result;
}

void emptySingleAndMissingIds()
{
    std::vector<VpCollectionRecord> records;
    const std::vector<VpCollectionId> missing{{42}};
    expect(eraseRecordsById(records, missing) == 0, "empty collection must be unchanged");
    records.emplace_back(1);
    records.emplace_back(2);
    records.emplace_back(1);
    expect(eraseRecordsById(records, std::vector<VpCollectionId>{}) == 0,
           "empty removal must be unchanged");
    expect(eraseRecordsById(records, missing) == 0, "missing ID must be ignored");
    expect(eraseRecordsById(records, std::vector<VpCollectionId>{{1}}) == 2,
           "single ID must erase every matching record");
    expect(ids(records) == std::vector<std::uint64_t>{2}, "single removal must retain survivor");
    expect(eraseRecordsById(records, records) == 1, "single aliased input must erase itself");
    expect(records.empty(), "aliased removal must leave no records");
}

void duplicatesAndStableSurvivors()
{
    std::vector<VpCollectionRecord> records;
    for (std::uint64_t id : {9, 2, 7, 2, 8, 4})
    {
        records.emplace_back(id, 1024);
    }
    const std::vector<VpCollectionId> removed{{2}, {404}, {2}, {4}};
    VpCollectionRecord::resetCounters();
    expect(eraseRecordsById(records, removed) == 3, "duplicate removal IDs must be idempotent");
    expect(ids(records) == std::vector<std::uint64_t>({9, 7, 8}),
           "surviving records must retain their original order");
    expect(VpCollectionRecord::copies == 0, "removal must not copy heavy records");
    expect(VpCollectionRecord::copied_payload_bytes == 0,
           "removal must not duplicate geometry payloads");
    expect(VpCollectionRecord::move_assignments <= records.size(),
           "survivors must be moved at most once");
    for (const VpCollectionRecord& record : records)
    {
        expect(record.geometry.size() == 1024, "moving records must retain geometry");
        expect(record.geometry.front() == static_cast<double>(record.id),
               "geometry must stay associated with its ID");
    }
    expect(eraseRecordsById(records, records) == 3, "aliased bulk removal must erase all IDs");
    expect(records.empty(), "bulk aliased removal must leave no records");
}

void randomizedReferenceComparison()
{
    std::mt19937 generator(2048);
    for (int trial = 0; trial < 200; ++trial)
    {
        std::vector<VpCollectionRecord> records;
        std::vector<VpCollectionId> removed;
        const std::size_t count = generator() % 80;
        const std::size_t removal_count = generator() % 30;
        for (std::size_t index = 0; index < count; ++index)
        {
            records.emplace_back(generator() % 20, 8);
        }
        for (std::size_t index = 0; index < removal_count; ++index)
        {
            removed.push_back({generator() % 25});
        }
        std::vector<std::uint64_t> expected = ids(records);
        for (const VpCollectionId& removed_id : removed)
        {
            expected.erase(std::remove(expected.begin(), expected.end(), removed_id.id),
                           expected.end());
        }
        VpCollectionRecord::resetCounters();
        expect(eraseRecordsById(records, removed) == count - expected.size(),
               "removal count must agree with sequential reference");
        expect(ids(records) == expected, "result must agree with sequential reference");
        expect(VpCollectionRecord::copies == 0, "randomized removal must not copy records");
    }
}

} // namespace
} // namespace Vp

int vpRunIdCollectionTests()
{
    try
    {
        Vp::emptySingleAndMissingIds();
        Vp::duplicatesAndStableSurvivors();
        Vp::randomizedReferenceComparison();
        std::cout << "ID collection tests passed (200 randomized cases, zero record copies).\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
