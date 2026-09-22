#pragma once

#include "vp_design_token.h"

#include <QObject>

namespace Vp
{

class VpThemeManager final : public QObject
{
    Q_OBJECT

  public:
    explicit VpThemeManager(QObject* parent = nullptr);

    VpThemeMode themeMode() const noexcept;
    const VpDesignToken& tokens() const noexcept;
    int uiScalePercent() const noexcept;
    bool applyTheme(VpThemeMode theme_mode);
    bool setUiScalePercent(int scale_percent);

  signals:
    void themeChanged(Vp::VpThemeMode theme_mode);
    void uiScaleChanged(int scale_percent);

  private:
    bool loadTokens(const QString& resource_path, VpDesignToken& tokens) const;
    void applyPaletteAndStyle();

    VpThemeMode m_theme_mode = VpThemeMode::Dark;
    VpDesignToken m_tokens;
    int m_ui_scale_percent = 100;
};

} // namespace Vp
