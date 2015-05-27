#ifndef USERNULLCLINEGUI_H
#define USERNULLCLINEGUI_H

#include "../globals/globals.h"
#include "../memrep/modelmgr.h"

namespace Ui {
class UserNullclineGui;
}

class QTableView;
class UserNullclineGui : public QWidget
{
    Q_OBJECT

    public:
        explicit UserNullclineGui(QWidget *parent = 0);
        ~UserNullclineGui();

        size_t NumValidNCs() const;
        QTableView* Table();

    private slots:
        void on_btnCancel_clicked();
        void on_btnValidate_clicked();

    private:
        Ui::UserNullclineGui *ui;

        ModelMgr* const _modelMgr;
};

#endif // USERNULLCLINEGUI_H
