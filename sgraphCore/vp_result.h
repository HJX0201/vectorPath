#pragma once

#include <string>
#include <utility>

namespace Vp
{

// 公共 API 的统一返回类型，用错误信息代替异常传播。
template <typename TValue> class VpResult
{
  public:
    // 构造成功结果。
    static VpResult success(TValue value)
    {
        return VpResult(true, std::move(value), {});
    }

    // 构造失败结果。
    static VpResult failure(std::u16string error_message)
    {
        return VpResult(false, TValue{}, std::move(error_message));
    }

    // 构造失败结果，并附带原生错误细节。
    static VpResult failureWithNativeDetail(std::u16string error_message, std::string native_detail)
    {
        VpResult result = failure(std::move(error_message));
        result.m_native_error_detail = std::move(native_detail);
        return result;
    }

    // 返回原生错误细节。
    const std::string& nativeErrorDetail() const noexcept
    {
        return m_native_error_detail;
    }

    // 返回是否成功。
    bool isSuccess() const noexcept
    {
        return m_is_success;
    }
    // 成功时转换为 true，便于 if 判断。
    explicit operator bool() const noexcept
    {
        return isSuccess();
    }
    // 返回成功值的只读引用。
    const TValue& value() const noexcept
    {
        return m_value;
    }
    // 返回成功值的可写引用。
    TValue& value() noexcept
    {
        return m_value;
    }
    // 返回错误信息。
    const std::u16string& errorMessage() const noexcept
    {
        return m_error_message;
    }

  private:
    // 私有构造，只允许经 success/failure 创建。
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
    // 构造空成功结果。
    static VpResult success()
    {
        return VpResult(true, {});
    }
    // 构造失败结果。
    static VpResult failure(std::u16string error_message)
    {
        return VpResult(false, std::move(error_message));
    }

    // 构造失败结果，并附带原生错误细节。
    static VpResult failureWithNativeDetail(std::u16string error_message, std::string native_detail)
    {
        VpResult result = failure(std::move(error_message));
        result.m_native_error_detail = std::move(native_detail);
        return result;
    }

    // 返回原生错误细节。
    const std::string& nativeErrorDetail() const noexcept
    {
        return m_native_error_detail;
    }

    // 返回是否成功。
    bool isSuccess() const noexcept
    {
        return m_is_success;
    }
    // 成功时转换为 true，便于 if 判断。
    explicit operator bool() const noexcept
    {
        return isSuccess();
    }
    // 返回错误信息。
    const std::u16string& errorMessage() const noexcept
    {
        return m_error_message;
    }

  private:
    // 私有构造，只允许经 success/failure 创建。
    VpResult(bool is_success, std::u16string error_message)
        : m_is_success(is_success), m_error_message(std::move(error_message))
    {
    }

    bool m_is_success = false;
    std::u16string m_error_message;
    std::string m_native_error_detail;
};

} // namespace Vp
