#ifndef JACOBIANGUI_H
#define JACOBIANGUI_H

#include "../globals/globals.h"
#include "../memrep/modelmgr.h"

namespace Ui {
class JacobianGui;
}

class QTableView;
class JacobianGui : public QWidget
{
    Q_OBJECT

    public:
        explicit JacobianGui(QWidget *parent = 0);
        ~JacobianGui();

        bool IsFull() const;
        QTableView* Table();

    private slots:
        void on_btnCancel_clicked();
        void on_btnValidate_clicked();

    private:
        Ui::JacobianGui *ui;

        ModelMgr* const _modelMgr;
};

#endif // JACOBIANGUI_H
