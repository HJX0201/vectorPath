#pragma once

class QTreeWidgetItem;

namespace Vp
{

class VpCadDocument;
struct VpDesignToken;
struct VpEntityRecord;

// 把图元属性填充为属性树的子节点。
QTreeWidgetItem* addEntityPropertyTree(QTreeWidgetItem* parent_item, const VpEntityRecord& entity,
                                       const VpCadDocument& document, const VpDesignToken& tokens);

} // namespace Vp
