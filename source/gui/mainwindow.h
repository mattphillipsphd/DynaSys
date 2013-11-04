#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QInputDialog>
#include <QMainWindow>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>

#include <muParser.h>

#include "comboboxdelegate.h"
#include "../models/conditionmodel.h"
#include "../models/parammodel.h"
#include "../file/sysfilein.h"
#include "../file/sysfileout.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <thread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
    public:
        static const int MAX_BUF_SIZE,
                        SLEEP_MS,
                        IP_SAMPLES_SHOWN;

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public slots:

    signals:
        void DoReplot();

    private slots:
        void on_actionLoad_triggered();
        void on_actionSave_Data_triggered();
        void on_actionSave_Model_triggered();

        void on_btnAddCondition_clicked();
        void on_btnAddDiff_clicked();
        void on_btnAddExpression_clicked();
        void on_btnAddParameter_clicked();
        void on_btnAddVariable_clicked();
        void on_btnRemoveCondition_clicked();
        void on_btnRemoveExpression_clicked();
        void on_btnRemoveDiff_clicked();
        void on_btnRemoveParameter_clicked();
        void on_btnRemoveVariable_clicked();
        void on_btnStart_clicked();

        void ComboBoxChanged(const QString& text);
        void ParamsChanged(QModelIndex, QModelIndex);
        void Replot();

    private:
        Ui::MainWindow *ui;

        void AddVarDelegate(int row, ComboBoxDelegate::TYPE type);
        void Draw();

        ConditionModel* _conditions;
        ParamModel* _differentials,
                * _initConds,
                * _parameters, //Must be numeric
                * _variables;   //Can invoke other expressions

        std::vector<ComboBoxDelegate*> _cmbDelegates;
        volatile bool _isDrawing;
        std::mutex _mutex;
        volatile bool _needGetParams;
        mu::Parser _parser;
        std::vector<mu::Parser> _parserConds;
        std::thread* _thread;
};

#endif // MAINWINDOW_H