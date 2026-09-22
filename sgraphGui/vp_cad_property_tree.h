#pragma once

class QTreeWidgetItem;

namespace Vp
{

class VpCadDocument;
struct VpDesignToken;
struct VpEntityRecord;

QTreeWidgetItem* addEntityPropertyTree(QTreeWidgetItem* parent_item, const VpEntityRecord& entity,
                                       const VpCadDocument& document, const VpDesignToken& tokens);

} // namespace Vp
