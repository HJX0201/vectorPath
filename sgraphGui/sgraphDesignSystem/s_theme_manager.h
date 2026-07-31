#pragma once

#include "s_design_token.h"

#include <QObject>

namespace smartCam
{

class SThemeManager final : public QObject
{
    Q_OBJECT

  public:
    explicit SThemeManager(QObject* parent = nullptr);

    SThemeMode themeMode() const noexcept;
    const SDesignToken& tokens() const noexcept;
    int uiScalePercent() const noexcept;
    bool applyTheme(SThemeMode theme_mode);
    bool setUiScalePercent(int scale_percent);

  signals:
    void themeChanged(smartCam::SThemeMode theme_mode);
    void uiScaleChanged(int scale_percent);

  private:
    bool loadTokens(const QString& resource_path, SDesignToken& tokens) const;
    void applyPaletteAndStyle();

    SThemeMode m_theme_mode = SThemeMode::Dark;
    SDesignToken m_tokens;
    int m_ui_scale_percent = 100;
};

} // namespace smartCam
