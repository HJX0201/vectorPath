#pragma once

#include "vp_design_token.h"

#include <QObject>

namespace Vp
{

// 深浅色主题与界面缩放的切换中心。
class VpThemeManager final : public QObject
{
    Q_OBJECT

  public:
    // 构造主题管理器并应用当前主题。
    explicit VpThemeManager(QObject* parent = nullptr);

    // 返回当前主题模式。
    VpThemeMode themeMode() const noexcept;
    // 返回当前主题的设计令牌。
    const VpDesignToken& tokens() const noexcept;
    // 返回界面缩放百分比。
    int uiScalePercent() const noexcept;
    // 切换主题，失败返回 false。
    bool applyTheme(VpThemeMode theme_mode);
    // 设置界面缩放，非法值返回 false。
    bool setUiScalePercent(int scale_percent);

  signals:
    // 主题切换后发出。
    void themeChanged(Vp::VpThemeMode theme_mode);
    // 界面缩放变化后发出。
    void uiScaleChanged(int scale_percent);

  private:
    // 从资源文件读取设计令牌。
    bool loadTokens(const QString& resource_path, VpDesignToken& tokens) const;
    // 把令牌应用到调色板与样式表。
    void applyPaletteAndStyle();

    VpThemeMode m_theme_mode = VpThemeMode::Dark;
    VpDesignToken m_tokens;
    int m_ui_scale_percent = 100;
};

} // namespace Vp
