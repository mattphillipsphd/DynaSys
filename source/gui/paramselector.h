#ifndef PARAMSELECTOR_H
#define PARAMSELECTOR_H

#include <QWidget>

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

private slots:
    void on_btnDelete_clicked();
    void on_btnNew_clicked();
//    void on_cboxParamSets_itemSelected();

private:
    Ui::ParamSelector *ui;
};

#endif // PARAMSELECTOR_H
