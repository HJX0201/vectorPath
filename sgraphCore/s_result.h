#pragma once

#include <QString>
#include <utility>

namespace vectorPath
{

template <typename TValue> class SResult
{
  public:
    static SResult success(TValue value)
    {
        return SResult(true, std::move(value), {});
    }

    static SResult failure(QString error_message)
    {
        return SResult(false, TValue{}, std::move(error_message));
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
    const QString& errorMessage() const noexcept
    {
        return m_error_message;
    }

  private:
    SResult(bool is_success, TValue value, QString error_message)
        : m_is_success(is_success), m_value(std::move(value)),
          m_error_message(std::move(error_message))
    {
    }

    bool m_is_success = false;
    TValue m_value{};
    QString m_error_message;
};

template <> class SResult<void>
{
  public:
    static SResult success()
    {
        return SResult(true, {});
    }
    static SResult failure(QString error_message)
    {
        return SResult(false, std::move(error_message));
    }

    bool isSuccess() const noexcept
    {
        return m_is_success;
    }
    explicit operator bool() const noexcept
    {
        return isSuccess();
    }
    const QString& errorMessage() const noexcept
    {
        return m_error_message;
    }

  private:
    SResult(bool is_success, QString error_message)
        : m_is_success(is_success), m_error_message(std::move(error_message))
    {
    }

    bool m_is_success = false;
    QString m_error_message;
};

} // namespace vectorPath
