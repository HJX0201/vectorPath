#include "vp_cad_document.h"

#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

bool validTextStyle(const VpTextStyleRecord& style)
{
    return !style.name.simplified().isEmpty() && style.name.size() <= 128 &&
           !style.font_family.simplified().isEmpty() && std::isfinite(style.fixed_height) &&
           style.fixed_height >= 0.0 && std::isfinite(style.width_factor) &&
           style.width_factor >= 0.01 && style.width_factor <= 100.0 &&
           std::isfinite(style.oblique_angle) && style.oblique_angle > -85.0 &&
           style.oblique_angle < 85.0;
}

QString entityTextStyleName(const VpEntityRecord& entity)
{
    if (entity.type == VpEntityType::Text)
    {
        return std::get<VpTextEntity>(entity.geometry).style_name;
    }
    if (entity.type == VpEntityType::MText)
    {
        return std::get<VpMTextEntity>(entity.geometry).style_name;
    }
    if (entity.type == VpEntityType::Leader)
    {
        return std::get<VpLeaderEntity>(entity.geometry).style_name;
    }
    return {};
}

} // namespace

const std::vector<VpTextStyleRecord>& VpCadDocument::textStyles() const noexcept
{
    return m_text_styles;
}

const VpTextStyleRecord* VpCadDocument::textStyle(const QString& style_name) const noexcept
{
    const auto iterator =
        std::find_if(m_text_styles.begin(), m_text_styles.end(),
                     [&style_name](const VpTextStyleRecord& style)
                     {
                         return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                     });
    return iterator == m_text_styles.end() ? nullptr : &*iterator;
}

QString VpCadDocument::currentTextStyleName() const
{
    return m_current_text_style_name;
}

bool VpCadDocument::addOrUpdateTextStyle(const VpTextStyleRecord& text_style)
{
    VpTextStyleRecord normalized = text_style;
    normalized.name = normalized.name.simplified();
    normalized.font_family = normalized.font_family.simplified();
    if (!validTextStyle(normalized))
    {
        return false;
    }
    const std::vector<VpTextStyleRecord> styles_before = m_text_styles;
    const QString current_style_before = m_current_text_style_name;
    auto iterator =
        std::find_if(m_text_styles.begin(), m_text_styles.end(),
                     [&normalized](const VpTextStyleRecord& style)
                     {
                         return style.name.compare(normalized.name, Qt::CaseInsensitive) == 0;
                     });
    if (iterator == m_text_styles.end())
    {
        m_text_styles.push_back(std::move(normalized));
    }
    else
    {
        normalized.name = iterator->name;
        *iterator = std::move(normalized);
    }
    commitTextStyleChange(tr("添加或更新文字样式"), styles_before, current_style_before);
    return true;
}

bool VpCadDocument::removeTextStyle(const QString& style_name)
{
    const auto iterator =
        std::find_if(m_text_styles.begin(), m_text_styles.end(),
                     [&style_name](const VpTextStyleRecord& style)
                     {
                         return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                     });
    if (iterator == m_text_styles.end() ||
        iterator->name.compare(QLatin1String("Standard"), Qt::CaseInsensitive) == 0 ||
        iterator->name.compare(m_current_text_style_name, Qt::CaseInsensitive) == 0 ||
        std::any_of(m_entities.begin(), m_entities.end(),
                    [&style_name](const VpEntityRecord& entity)
                    {
                        return entityTextStyleName(entity).compare(style_name,
                                                                   Qt::CaseInsensitive) == 0;
                    }))
    {
        return false;
    }
    const std::vector<VpTextStyleRecord> styles_before = m_text_styles;
    const QString current_style_before = m_current_text_style_name;
    m_text_styles.erase(iterator);
    commitTextStyleChange(tr("删除文字样式"), styles_before, current_style_before);
    return true;
}

bool VpCadDocument::setCurrentTextStyle(const QString& style_name)
{
    const VpTextStyleRecord* style = textStyle(style_name);
    if (!style || style->name == m_current_text_style_name)
    {
        return style != nullptr;
    }
    const std::vector<VpTextStyleRecord> styles_before = m_text_styles;
    const QString current_style_before = m_current_text_style_name;
    m_current_text_style_name = style->name;
    commitTextStyleChange(tr("设置当前文字样式"), styles_before, current_style_before);
    return true;
}

void VpCadDocument::commitTextStyleChange(QString label,
                                          std::vector<VpTextStyleRecord> styles_before,
                                          QString current_style_before)
{
    VpHistoryEntry entry;
    entry.label = std::move(label);
    entry.has_text_style_change = true;
    entry.text_styles_before = std::move(styles_before);
    entry.text_styles_after = m_text_styles;
    entry.current_text_style_before = std::move(current_style_before);
    entry.current_text_style_after = m_current_text_style_name;
    m_undo_stack.push_back(std::move(entry));
    m_redo_stack.clear();
    setModified(true);
    emit textStylesChanged();
    emit currentTextStyleChanged(m_current_text_style_name);
    emitDocumentState();
}

} // namespace Vp
