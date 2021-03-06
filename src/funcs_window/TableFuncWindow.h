#ifndef TABLE_FUNC_WINDOW_H
#define TABLE_FUNC_WINDOW_H

#include "../SchemaWindows.h"
#include "../funcs/TableFunction.h"

#include <QTableWidget>
#include <QItemDelegate>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Ori {
namespace Widgets {
class StatusBar;
}}

class FrozenStateButton;
class TableFunction;

class TableFuncPositionColumnItemDelegate : public QItemDelegate
{
public:
    TableFuncPositionColumnItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const override;

private:
    mutable QModelIndex _paintingIndex;
};


class TableFuncResultTable: public QTableWidget
{
    Q_OBJECT

public:
    TableFuncResultTable(const QVector<TableFunction::ColumnDef>& columns);

    bool showT = true;
    bool showS = true;

    void updateColumnTitles();
    void update(const QVector<TableFunction::Result>& results);

    void copy();

private:
    QVector<TableFunction::ColumnDef> _columns;
    QMenu *_contextMenu = nullptr;

    void showContextMenu(const QPoint& pos);
};


class TableFuncWindow : public SchemaMdiChild, public IEditableWindow
{
    Q_OBJECT

public:
    explicit TableFuncWindow(TableFunction*);
    ~TableFuncWindow() override;

    TableFunction* function() const { return _function; }

    // inherits from BasicMdiChild
    QList<QMenu*> menus() override { return QList<QMenu*>() << _menuTable; }

    // Implementation of SchemaListener
    void recalcRequired(Schema*) override { update(); }

    // Implementation of IEditableWindow
    SupportedCommands supportedCommands() override { return EditCmd_Copy | EditCmd_SelectAll; }
    bool canCopy() override { return true; }
    void copy() override { _table->copy(); }
    void selectAll() override { _table->selectAll(); }

public slots:
    void update();

private slots:
    void activateModeT();
    void activateModeS();
    void freeze(bool);

private:
    TableFunction *_function;
    QMenu *_menuTable;
    QAction *_actnUpdate, *_actnShowT, *_actnShowS, *_actnFreeze, *_actnFrozenInfo;
    FrozenStateButton* _buttonFrozenInfo;
    Ori::Widgets::StatusBar *_statusBar;
    TableFuncResultTable *_table;
    bool _frozen = false;
    bool _needRecalc = false;

    void createActions();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createContent();

    void showStatusError(const QString &message);
    void clearStatusInfo();

    void showModeTS();
    void updateModeTS();
    void updateTable();
};

#endif // TABLE_FUNC_WINDOW_H
