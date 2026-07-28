#include "s_cad_document.h"

#include <algorithm>
#include <cmath>

namespace smartGraphics
{
namespace
{

bool validTextStyle(const STextStyleRecord& style)
{
    return !style.name.simplified().isEmpty() && style.name.size() <= 128 &&
           !style.font_family.simplified().isEmpty() && std::isfinite(style.fixed_height) &&
           style.fixed_height >= 0.0 && std::isfinite(style.width_factor) &&
           style.width_factor >= 0.01 && style.width_factor <= 100.0 &&
           std::isfinite(style.oblique_angle) && style.oblique_angle > -85.0 &&
           style.oblique_angle < 85.0;
}

QString entityTextStyleName(const SEntityRecord& entity)
{
    if (entity.type == SEntityType::Text)
    {
        return std::get<STextEntity>(entity.geometry).style_name;
    }
    if (entity.type == SEntityType::MText)
    {
        return std::get<SMTextEntity>(entity.geometry).style_name;
    }
    if (entity.type == SEntityType::Leader)
    {
        return std::get<SLeaderEntity>(entity.geometry).style_name;
    }
    return {};
}

} // namespace

const std::vector<STextStyleRecord>& SCadDocument::textStyles() const noexcept
{
    return m_text_styles;
}

const STextStyleRecord* SCadDocument::textStyle(const QString& style_name) const noexcept
{
    const auto iterator =
        std::find_if(m_text_styles.begin(), m_text_styles.end(),
                     [&style_name](const STextStyleRecord& style)
                     {
                         return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                     });
    return iterator == m_text_styles.end() ? nullptr : &*iterator;
}

QString SCadDocument::currentTextStyleName() const
{
    return m_current_text_style_name;
}

bool SCadDocument::addOrUpdateTextStyle(const STextStyleRecord& text_style)
{
    STextStyleRecord normalized = text_style;
    normalized.name = normalized.name.simplified();
    normalized.font_family = normalized.font_family.simplified();
    if (!validTextStyle(normalized))
    {
        return false;
    }
    const std::vector<STextStyleRecord> styles_before = m_text_styles;
    const QString current_style_before = m_current_text_style_name;
    auto iterator =
        std::find_if(m_text_styles.begin(), m_text_styles.end(),
                     [&normalized](const STextStyleRecord& style)
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

bool SCadDocument::removeTextStyle(const QString& style_name)
{
    const auto iterator =
        std::find_if(m_text_styles.begin(), m_text_styles.end(),
                     [&style_name](const STextStyleRecord& style)
                     {
                         return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                     });
    if (iterator == m_text_styles.end() ||
        iterator->name.compare(QLatin1String("Standard"), Qt::CaseInsensitive) == 0 ||
        iterator->name.compare(m_current_text_style_name, Qt::CaseInsensitive) == 0 ||
        std::any_of(m_entities.begin(), m_entities.end(),
                    [&style_name](const SEntityRecord& entity)
                    {
                        return entityTextStyleName(entity).compare(style_name,
                                                                   Qt::CaseInsensitive) == 0;
                    }))
    {
        return false;
    }
    const std::vector<STextStyleRecord> styles_before = m_text_styles;
    const QString current_style_before = m_current_text_style_name;
    m_text_styles.erase(iterator);
    commitTextStyleChange(tr("删除文字样式"), styles_before, current_style_before);
    return true;
}

bool SCadDocument::setCurrentTextStyle(const QString& style_name)
{
    const STextStyleRecord* style = textStyle(style_name);
    if (!style || style->name == m_current_text_style_name)
    {
        return style != nullptr;
    }
    const std::vector<STextStyleRecord> styles_before = m_text_styles;
    const QString current_style_before = m_current_text_style_name;
    m_current_text_style_name = style->name;
    commitTextStyleChange(tr("设置当前文字样式"), styles_before, current_style_before);
    return true;
}

void SCadDocument::commitTextStyleChange(QString label, std::vector<STextStyleRecord> styles_before,
                                         QString current_style_before)
{
    SHistoryEntry entry;
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

} // namespace smartGraphics
