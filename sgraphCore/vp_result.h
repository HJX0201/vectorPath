#pragma once

#include <string>
#include <utility>

namespace Vp
{

template <typename TValue> class VpResult
{
  public:
    static VpResult success(TValue value)
    {
        return VpResult(true, std::move(value), {});
    }

    static VpResult failure(std::u16string error_message)
    {
        return VpResult(false, TValue{}, std::move(error_message));
    }

    static VpResult failureWithNativeDetail(std::u16string error_message, std::string native_detail)
    {
        VpResult result = failure(std::move(error_message));
        result.m_native_error_detail = std::move(native_detail);
        return result;
    }

    const std::string& nativeErrorDetail() const noexcept
    {
        return m_native_error_detail;
    }

    bool isSuccess() const noexcept
    {
        return m_is_success;
    }
    explicit operator bool() const noexcept
    {
        return isSuccess();
    }
    const TValue& value() const noexcept
    {
        return m_value;
    }
    TValue& value() noexcept
    {
        return m_value;
    }
    const std::u16string& errorMessage() const noexcept
    {
        return m_error_message;
    }

  private:
    VpResult(bool is_success, TValue value, std::u16string error_message)
        : m_is_success(is_success), m_value(std::move(value)),
          m_error_message(std::move(error_message))
    {
    }

    bool m_is_success = false;
    TValue m_value{};
    std::u16string m_error_message;
    std::string m_native_error_detail;
};

template <> class VpResult<void>
{
  public:
    static VpResult success()
    {
        return VpResult(true, {});
    }
    static VpResult failure(std::u16string error_message)
    {
        return VpResult(false, std::move(error_message));
    }

    static VpResult failureWithNativeDetail(std::u16string error_message, std::string native_detail)
    {
        VpResult result = failure(std::move(error_message));
        result.m_native_error_detail = std::move(native_detail);
        return result;
    }

    const std::string& nativeErrorDetail() const noexcept
    {
        return m_native_error_detail;
    }

    bool isSuccess() const noexcept
    {
        return m_is_success;
    }
    explicit operator bool() const noexcept
    {
        return isSuccess();
    }
    const std::u16string& errorMessage() const noexcept
    {
        return m_error_message;
    }

  private:
    VpResult(bool is_success, std::u16string error_message)
        : m_is_success(is_success), m_error_message(std::move(error_message))
    {
    }

    bool m_is_success = false;
    std::u16string m_error_message;
    std::string m_native_error_detail;
};

} // namespace Vp
