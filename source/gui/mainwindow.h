#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <thread>

#include <QColor>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QMainWindow>
#include <QStringListModel>

#include <qwt_scale_div.h>
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

//#define DEBUG_FUNC


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

        struct ViewRect
        {
            ViewRect(double xmi, double xma, double ymi, double yma)
                : xmin(xmi), xmax(xma), ymin(ymi), ymax(yma)
            {}
            ViewRect() : xmin(0.0), xmax(0.0), ymin(0.0), ymax(0.0)
            {}
            bool operator==(const ViewRect& o) const
            {
                return xmin==o.xmin && xmax==o.xmax && ymin==o.ymin && ymax==o.ymax;
            }
            bool operator!=(const ViewRect& o) const
            {
                return !operator==(o);
            }

            double xmin, xmax, ymin, ymax;
        };

        static const int DEFAULT_SINGLE_STEP,
                        DEFAULT_SINGLE_TAIL,
                        DEFAULT_VF_STEP,
                        DEFAULT_VF_TAIL,
                        MAX_BUF_SIZE,
                        SLEEP_MS,
                        SLIDER_INT_LIM, //Because QSliders have integer increments
                        IP_SAMPLES_SHOWN,
                        XY_SAMPLES_SHOWN,
                        VF_RESOLUTION,
                        VF_SLEEP_MS;
            //If Qwt isn't able to draw the samples quickly enough, you get a recursive draw
            //error

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public slots:

    signals:
        void DoAttachVF(bool attach); //Can't have default parameter values in signals!!
        void DoInitParserMgr();
        void DoReplot(const ViewRect& pp_data, const ViewRect& tp_data);
        void DoUpdateParams();

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
        void on_btnFitVF_clicked();
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

        void on_spnStepsPerSec_valueChanged(int value);
        void on_spnTailLength_valueChanged(int value);
        void on_spnVFResolution_valueChanged(int);

        void AttachPhasePlot(bool attach = true);
        void AttachTimePlot(bool attach = true);
        void AttachVectorField(bool attach = true);
        void ComboBoxChanged(const QString& text);
        void ExprnChanged(QModelIndex, QModelIndex);
        void InitParserMgr();
        void ParamChanged(QModelIndex topLeft, QModelIndex bottomRight);
        void ResultsChanged(QModelIndex, QModelIndex);
        void Replot(const ViewRect& pp_data, const ViewRect& tp_data);
        void UpdateParams();

    private:
        Ui::MainWindow *ui;

        void AddVarDelegate(int row);
        void AddVarDelegate(int row, const std::string& type);
        void ClearPlots();
        void ConnectModels();
        void Draw();
        void DrawPhasePortrait();
        void DrawVectorField();
        void InitDefaultModel();
        void InitDraw();
        void InitModels(const std::vector<ParamModelBase*>* models = nullptr,
                        ConditionModel* conditions = nullptr);
        void InitPlots();
        const std::vector<QColor> InitTPColors() const;
        bool IsVFPresent() const;
        void LoadModel();
        void ResetPhasePlotAxes();
        void ResetResultsList(int cond_row);
        void SetButtonsEnabled(bool is_enabled);
        void UpdateLists();
        void UpdatePulseVList(); // ### There should be a way to make this automatic...
        void UpdateSliderPList();
        void UpdateResultsModel(int cond_row);
        void UpdateTimePlotTable();
        void UpdateVectorField();

        AboutGui* _aboutGui;

        // ### Get rid of all of these, store in ParserMgr, just refer to them by name
        ConditionModel* _conditions;
        ParamModelBase* _differentials,
                * _initConds,
                * _parameters, //Must be numeric
                * _variables;   //Can invoke other expressions

        std::vector<ComboBoxDelegate*> _cmbDelegates;
        std::condition_variable _condVar;
        std::string _fileName;
        volatile bool _finishedReplot;
        const std::thread::id _guiTid;
        volatile bool _isDrawing, _isVFAttached;
        std::mutex _mutex;
        volatile bool _needClearVF, _needInitialize, _needUpdateExprns, _needUpdateVF;
        ParserMgr _parserMgr;
        PLOT_MODE _plotMode;
        std::vector<QwtPlotItem*> _ppPlotItems;
        std::string _pulseResetValue;
        size_t _pulseParIdx;
        int _pulseStepsRemaining,
            //Consider making a little pulse struct/class
            _singleStepsSec, _singleTailLen;
        const std::vector<QColor> _tpColors;
        std::vector<QwtPlotItem*> _vfPlotItems;
        int _vfStepsSec, _vfTailLen;

        QwtPlotCurve* _curve;
        QwtPlotMarker* _marker;
        std::vector<QwtPlotCurve*> _tpCurves;
};

Q_DECLARE_METATYPE(MainWindow::ViewRect)

#endif // MAINWINDOW_H
