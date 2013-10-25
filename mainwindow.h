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

#include "conditionmodel.h"
#include "parammodel.h"
#include "sysfilein.h"
#include "sysfileout.h"

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

    signals:
        void DoReplot();

    private slots:
        void on_actionLoad_triggered();
        void on_actionSave_Data_triggered();
        void on_actionSave_Model_triggered();

        void on_btnAddDiff_clicked();
        void on_btnAddParameter_clicked();
        void on_btnAddVariable_clicked();
        void on_btnRemoveDiff_clicked();
        void on_btnRemoveParameter_clicked();
        void on_btnRemoveVariable_clicked();
        void on_btnStart_clicked();

        void ParamsChanged(QModelIndex, QModelIndex);
        void Replot();

    private:
        Ui::MainWindow *ui;

        void Draw();

        ConditionModel* _conditions;
        ParamModel* _differentials,
                * _initConds,
                * _parameters, //Must be numeric
                * _variables;   //Can invoke other expressions

//        QWidget* _guiVariable, * _guiDifferential;
        volatile bool _isDrawing;
        std::mutex _mutex;
        volatile bool _needGetParams;
        mu::Parser _parser;
        std::vector<mu::Parser> _parserConds;
        std::thread* _thread;
};

#endif // MAINWINDOW_H
