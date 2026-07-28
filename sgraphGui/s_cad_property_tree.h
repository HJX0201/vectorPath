#pragma once

class QTreeWidgetItem;

namespace smartGraphics
{

class SCadDocument;
struct SDesignToken;
struct SEntityRecord;

QTreeWidgetItem* addEntityPropertyTree(QTreeWidgetItem* parent_item, const SEntityRecord& entity,
                                       const SCadDocument& document, const SDesignToken& tokens);

} // namespace smartGraphics
