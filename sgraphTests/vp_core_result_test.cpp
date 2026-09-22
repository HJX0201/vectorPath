#include "vp_result.h"

#include <iostream>
#include <memory>
#include <string>

int main()
{
    using namespace Vp;
    auto success = VpResult<std::unique_ptr<int>>::success(std::make_unique<int>(42));
    if (!success || *success.value() != 42 || !success.errorMessage().empty())
    {
        std::cerr << "Move-only success payload failed\n";
        return 1;
    }
    const std::u16string message = u"中文错误 \U0001f642";
    auto failure = VpResult<int>::failure(message);
    if (failure || failure.errorMessage() != message || failure.value() != 0)
    {
        std::cerr << "Failure semantics or UTF-16 preservation failed\n";
        return 1;
    }
    const auto detailed = VpResult<void>::failureWithNativeDetail(message, "native bytes");
    if (detailed || detailed.nativeErrorDetail() != "native bytes" ||
        detailed.errorMessage() != message || !VpResult<void>::success())
    {
        std::cerr << "Void result or uninterpreted diagnostic failed\n";
        return 1;
    }
    return 0;
}
