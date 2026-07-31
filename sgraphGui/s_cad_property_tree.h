#pragma once

class QTreeWidgetItem;

namespace vectorPath
{

class SCadDocument;
struct SDesignToken;
struct SEntityRecord;

QTreeWidgetItem* addEntityPropertyTree(QTreeWidgetItem* parent_item, const SEntityRecord& entity,
                                       const SCadDocument& document, const SDesignToken& tokens);

} // namespace vectorPath
