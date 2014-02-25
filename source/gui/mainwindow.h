#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
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

#include "aboutgui.h"
#include "checkboxdelegate.h"
#include "comboboxdelegate.h"
#include "../models/conditionmodel.h"
#include "../models/differentialmodel.h"
#include "../models/initialcondmodel.h"
#include "../models/parammodel.h"
#include "../models/tpvtablemodel.h"
#include "../models/variablemodel.h"
#include "../memrep/parsermgr.h"
#include "../file/sysfilein.h"
#include "../file/sysfileout.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <thread>

#include <QStringListModel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
    public:
        enum PLOT_MODE
        {
            SINGLE,
            VECTOR_FIELD
        };

        static const int MAX_BUF_SIZE,
                        SLEEP_MS,
                        SLIDER_INT_LIM, //Because QSliders have integer increments
                        IP_SAMPLES_SHOWN,
                        XY_SAMPLES_SHOWN,
                        VF_RESOLUTION;
            //If Qwt isn't able to draw the samples quickly enough, you get a recursive draw
            //error

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public slots:

    signals:
        void DoReplot();
        void DoUpdateParams();
        void VFThreadComplete();

    private slots:
        void on_actionAbout_triggered();
        void on_actionClear_triggered();
        void on_actionLoad_triggered();
        void on_actionReload_Current_triggered();
        void on_actionSave_Data_triggered();
        void on_actionSave_Model_triggered();

        void on_btnAddCondition_clicked();
        void on_btnPulse_clicked();
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

        void on_cboxVectorField_stateChanged(int state);

        void on_cmbPlotMode_currentIndexChanged(const QString& text);
        void on_cmbSlidePars_currentIndexChanged(int index);

        void on_lsConditions_clicked(const QModelIndex& index);

        void on_sldParameter_valueChanged(int value);

        void on_spnTailLength_valueChanged(int);
        void on_spnVFResolution_valueChanged(int);

        void ComboBoxChanged(const QString& text);
        void EndVFThread();
        void ExprnChanged(QModelIndex, QModelIndex);
        void ParamChanged(QModelIndex topLeft, QModelIndex bottomRight);
        void ResultsChanged(QModelIndex, QModelIndex);
        void Replot();
        void UpdateParams();

    private:
        Ui::MainWindow *ui;

        void AddVarDelegate(int row);
        void AddVarDelegate(int row, const std::string& type);
        void ConnectModels();
        void Draw();
        void DrawPhasePortrait();
        void DrawVectorField();
        void InitDefaultModel();
        void InitModels(const std::vector<ParamModelBase*>* models = nullptr,
                        ConditionModel* conditions = nullptr);
        void InitParserMgr();
        const std::vector<QColor> InitTPColors() const;
        void LoadModel();
        void ResetPhasePlotAxes();
        void ResetResultsList(int cond_row);
        void UpdatePulseVList(); // ### There should be a way to make this automatic...
        void UpdateSliderPList();
        void UpdateResultsModel(int cond_row);
        void UpdateTimePlotTable();

        AboutGui* _aboutGui;

        // ### Get rid of all of these, store in ParserMgr, just refer to them by name
        ConditionModel* _conditions;
        ParamModelBase* _differentials,
                * _initConds,
                * _parameters, //Must be numeric
                * _variables;   //Can invoke other expressions

        std::vector<ComboBoxDelegate*> _cmbDelegates;
        std::string _fileName;
        volatile bool _isDrawing;
        std::mutex _mutex;
        volatile bool _needDrawVF, _needInitialize, _needUpdateExprns;
        ParserMgr _parserMgr;
        volatile bool _plottingNow;
        PLOT_MODE _plotMode;
        std::vector<QwtPlotItem*> _ppPlotItems;
        std::string _pulseResetValue;
        size_t _pulseParIdx;
        int _pulseStepsRemaining;
            //Consider making a little pulse struct/class
        const std::vector<QColor> _tpColors;
        std::vector<QwtPlotItem*> _vfPlotItems;
        std::thread* _ppThread, * _vfThread;
};

#endif // MAINWINDOW_H
