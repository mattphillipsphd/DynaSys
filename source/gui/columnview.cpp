#include "columnview.h"

ColumnView::ColumnView(QWidget *parent) :
    QColumnView(parent)
{
}

QAbstractItemView* ColumnView::createColumn(const QModelIndex& rootIndex)
{
    if (!rootIndex.parent().isValid()) return QColumnView::createColumn(rootIndex);
    QListView* view = new QListView();
    view->setEnabled(false);
    return view;
}
