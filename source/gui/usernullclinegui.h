#ifndef USERNULLCLINEGUI_H
#define USERNULLCLINEGUI_H

#include "../globals/globals.h"
#include "../memrep/modelmgr.h"

namespace Ui {
class UserNullclineGui;
}

class UserNullclineGui : public QWidget
{
    Q_OBJECT

    public:
        explicit UserNullclineGui(QWidget *parent = 0);
        ~UserNullclineGui();

        size_t NumNCs() const;

    private slots:
        void on_btnValidate_clicked();

    private:
        Ui::UserNullclineGui *ui;

        ModelMgr* const _modelMgr;
        VecStr _nullclines;
};

#endif // USERNULLCLINEGUI_H
