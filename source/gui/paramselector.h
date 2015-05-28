#ifndef PARAMSELECTOR_H
#define PARAMSELECTOR_H

#include "../globals/globals.h"
#include "../globals/log.h"
#include "../globals/scopetracker.h"
#include "../memrep/modelmgr.h"

namespace Ui {
class ParamSelector;
}

class ParamSelector : public QWidget
{
    Q_OBJECT

    public:
        explicit ParamSelector(QWidget *parent = 0);
        ~ParamSelector();

        void SetParamTableModel(QAbstractItemModel* model);
        void LoadData(int index = 0);

    private slots:
        void on_btnDelete_clicked();
        void on_btnNew_clicked();
        void on_btnSet_clicked();
        void on_cmbSelect_currentIndexChanged(int index);
        void on_tblPars_cellChanged(int row, int column);
        void on_tblInputFiles_cellChanged(int row, int column);
        void on_txtNotes_textChanged();

    private:
        Ui::ParamSelector *ui;

        void UpdateParVariant(int i);

        Log* const _log;
        ModelMgr* const _modelMgr;

        bool _doNotUpdate;
};

#endif // PARAMSELECTOR_H
