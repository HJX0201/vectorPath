#include "s_cad_document.h"

#include <algorithm>
#include <cmath>

namespace vectorPath
{
namespace
{

bool validDimensionStyle(const SDimensionStyleRecord& style)
{
    return !style.name.simplified().isEmpty() && style.name.size() <= 128 &&
           std::isfinite(style.text_height) && style.text_height > 0.0 &&
           style.text_height <= 1.0e6 && std::isfinite(style.arrow_size) &&
           style.arrow_size > 0.0 && style.arrow_size <= 1.0e6 &&
           std::isfinite(style.overall_scale) && style.overall_scale > 0.0 &&
           style.overall_scale <= 1.0e6 && std::isfinite(style.linear_scale) &&
           style.linear_scale > 0.0 && style.linear_scale <= 1.0e6 && style.linear_precision >= 0 &&
           style.linear_precision <= 8 && style.angular_precision >= 0 &&
           style.angular_precision <= 8 && style.prefix.size() <= 64 && style.suffix.size() <= 64;
}

} // namespace

const std::vector<SDimensionStyleRecord>& SCadDocument::dimensionStyles() const noexcept
{
    return m_dimension_styles;
}

const SDimensionStyleRecord* SCadDocument::dimensionStyle(const QString& style_name) const noexcept
{
    const auto iterator =
        std::find_if(m_dimension_styles.begin(), m_dimension_styles.end(),
                     [&style_name](const SDimensionStyleRecord& style)
                     {
                         return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                     });
    return iterator == m_dimension_styles.end() ? nullptr : &*iterator;
}

QString SCadDocument::currentDimensionStyleName() const
{
    return m_current_dimension_style_name;
}

bool SCadDocument::addOrUpdateDimensionStyle(const SDimensionStyleRecord& dimension_style)
{
    SDimensionStyleRecord normalized = dimension_style;
    normalized.name = normalized.name.simplified();
    if (!validDimensionStyle(normalized))
    {
        return false;
    }
    const std::vector<SDimensionStyleRecord> styles_before = m_dimension_styles;
    const QString current_style_before = m_current_dimension_style_name;
    auto iterator =
        std::find_if(m_dimension_styles.begin(), m_dimension_styles.end(),
                     [&normalized](const SDimensionStyleRecord& style)
                     {
                         return style.name.compare(normalized.name, Qt::CaseInsensitive) == 0;
                     });
    if (iterator == m_dimension_styles.end())
    {
        m_dimension_styles.push_back(std::move(normalized));
    }
    else
    {
        normalized.name = iterator->name;
        *iterator = std::move(normalized);
    }
    commitDimensionStyleChange(tr("添加或更新标注样式"), styles_before, current_style_before);
    return true;
}

bool SCadDocument::removeDimensionStyle(const QString& style_name)
{
    const auto iterator =
        std::find_if(m_dimension_styles.begin(), m_dimension_styles.end(),
                     [&style_name](const SDimensionStyleRecord& style)
                     {
                         return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                     });
    const bool is_in_use =
        std::any_of(m_entities.begin(), m_entities.end(),
                    [&style_name](const SEntityRecord& entity)
                    {
                        return entity.type == SEntityType::LinearDimension &&
                               std::get<SLinearDimensionEntity>(entity.geometry)
                                       .style_name.compare(style_name, Qt::CaseInsensitive) == 0;
                    });
    if (iterator == m_dimension_styles.end() ||
        iterator->name.compare(QLatin1String("Standard"), Qt::CaseInsensitive) == 0 ||
        iterator->name.compare(m_current_dimension_style_name, Qt::CaseInsensitive) == 0 ||
        is_in_use)
    {
        return false;
    }
    const std::vector<SDimensionStyleRecord> styles_before = m_dimension_styles;
    const QString current_style_before = m_current_dimension_style_name;
    m_dimension_styles.erase(iterator);
    commitDimensionStyleChange(tr("删除标注样式"), styles_before, current_style_before);
    return true;
}

bool SCadDocument::setCurrentDimensionStyle(const QString& style_name)
{
    const SDimensionStyleRecord* style = dimensionStyle(style_name);
    if (!style || style->name == m_current_dimension_style_name)
    {
        return style != nullptr;
    }
    const std::vector<SDimensionStyleRecord> styles_before = m_dimension_styles;
    const QString current_style_before = m_current_dimension_style_name;
    m_current_dimension_style_name = style->name;
    commitDimensionStyleChange(tr("设置当前标注样式"), styles_before, current_style_before);
    return true;
}

void SCadDocument::commitDimensionStyleChange(QString label,
                                              std::vector<SDimensionStyleRecord> styles_before,
                                              QString current_style_before)
{
    SHistoryEntry entry;
    entry.label = std::move(label);
    entry.has_dimension_style_change = true;
    entry.dimension_styles_before = std::move(styles_before);
    entry.dimension_styles_after = m_dimension_styles;
    entry.current_dimension_style_before = std::move(current_style_before);
    entry.current_dimension_style_after = m_current_dimension_style_name;
    m_undo_stack.push_back(std::move(entry));
    m_redo_stack.clear();
    setModified(true);
    emit dimensionStylesChanged();
    emit currentDimensionStyleChanged(m_current_dimension_style_name);
    emitDocumentState();
}

} // namespace vectorPath
