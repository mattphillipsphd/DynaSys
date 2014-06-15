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
#include <unordered_set>

#include <QColor>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QStringListModel>

#include <qwt_scale_div.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_renderer.h>
#include <qwt_symbol.h>

#include <muParser.h>

#include "aboutgui.h"
#include "checkboxdelegate.h"
#include "comboboxdelegate.h"
#include "dspinboxdelegate.h"
#include "fastrungui.h"
#include "fitgui.h"
#include "loggui.h"
#include "notesgui.h"
#include "parameditor.h"
#include "../compile/cfileso.h"
#include "../compile/cudakernel.h"
#include "../compile/executable.h"
#include "../compile/mexfile.h"
#include "../compile/sharedobj.h"
#include "../file/datfileout.h"
#include "../file/defaultdirmgr.h"
#include "../file/sysfilein.h"
#include "../file/sysfileout.h"
#include "../globals/scopetracker.h"
#include "../models/conditionmodel.h"
#include "../models/differentialmodel.h"
#include "../models/initialcondmodel.h"
#include "../models/parammodel.h"
#include "../models/tpvtablemodel.h"
#include "../models/variablemodel.h"
#include "../memrep/arrowhead.h"
#include "../memrep/parsermgr.h"

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
        enum PLAY_STATE
        {
            STOPPED,
            DRAWING,
            PAUSED
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
                        TP_SAMPLES_SHOWN,
                        TP_WINDOW_LENGTH,
                        XY_SAMPLES_SHOWN,
                        VF_RESOLUTION,
                        VF_SLEEP_MS;
        static const double MIN_MODEL_STEP;
            //If Qwt isn't able to draw the samples quickly enough, you get a recursive draw
            //error

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public slots:
        void Error();
        void ExecutableFinished(int id, bool is_normal);
        void FastRunFinished();
        void FitFinished();
        void LoadTempModel(void* models);
        void ParamEditorClosed();
        void ParserToLog();
        void Pause();
        void SaveNotes();
        void StartCompiled(int duration, int save_mod_n);
        void StartFastRun(int duration, int save_mod_n);
        void StartFit();
        void UpdateMousePos(QPointF pos);
        void UpdateTimePlot();

    protected:
        virtual void closeEvent(QCloseEvent *) override;

    signals:
        void DoAttachVF(bool attach); //Can't have default parameter values in signals!!
        void DoInitParserMgr();
        void DoReplot(const ViewRect& pp_data, const ViewRect& tp_data);
        void DoUpdateParams();
        void UpdateSimPBar(int n);

    private slots:
        void on_actionAbout_triggered();
        void on_actionClear_triggered();
        void on_actionCreate_CUDA_kernel_triggered();
        void on_actionCreate_MEX_file_triggered();
        void on_actionCreate_SO_triggered();
        void on_actionCompile_Run_triggered();
        void on_actionExit_triggered();
        void on_actionFit_triggered();
        void on_actionLoad_triggered();
        void on_actionLog_triggered();
        void on_actionNotes_triggered();
        void on_actionParameters_triggered();
        void on_actionReload_Current_triggered();
        void on_actionRun_Offline_triggered();
        void on_actionSave_Data_triggered();
        void on_actionSave_Model_triggered();
        void on_actionSave_Model_As_triggered();
        void on_actionSave_Phase_Plot_triggered();
        void on_actionSave_Time_Plot_triggered();
        void on_actionSave_Vector_Field_triggered();

        void on_btnAddCondition_clicked();
        void on_btnPulse_clicked();
        void on_btnAddDiff_clicked();
        void on_btnAddExpression_clicked();
        void on_btnAddParameter_clicked();
        void on_btnAddVariable_clicked();
        void on_btnFitView_clicked();
        void on_btnRemoveCondition_clicked();
        void on_btnRemoveExpression_clicked();
        void on_btnRemoveDiff_clicked();
        void on_btnRemoveParameter_clicked();
        void on_btnRemoveVariable_clicked();
        void on_btnStart_clicked();

        void on_cboxVectorField_stateChanged(int state);
        void on_cboxNullclines_stateChanged(int state);

        void on_cmbPlotMode_currentIndexChanged(const QString& text);
        void on_cmbSlidePars_currentIndexChanged(int index);

        void on_edModelStep_editingFinished();
        void on_edNumTPSamples_editingFinished();

        void on_lsConditions_clicked(const QModelIndex& index);

        void on_sldParameter_sliderMoved(int value);

        void on_spnStepsPerSec_valueChanged(int value);
        void on_spnTailLength_valueChanged(int value);
        void on_spnVFResolution_valueChanged(int);

        void AttachPhasePlot(bool attach = true);
        void AttachTimePlot(bool attach = true);
        void AttachVectorField(bool attach = true);
        void ComboBoxChanged(size_t row);
        void ExprnChanged(QModelIndex, QModelIndex);
        void InitParserMgr();
        void ParamChanged(QModelIndex topLeft, QModelIndex bottomRight);
        void ResultsChanged(QModelIndex, QModelIndex);
        void Replot(const ViewRect& pp_data, const ViewRect& tp_data);
        void UpdateParams();

    private:
        struct JobRecord
        {
            JobRecord(int id_, Executable* exe_,
                      std::chrono::time_point<std::chrono::system_clock> start_)
                : id(id_), exe(exe_), start(start_)
            {}
            int id;
            Executable* exe;
            std::chrono::time_point<std::chrono::system_clock> start;
        };

        Ui::MainWindow *ui;

        void ClearPlots();
        Executable* CreateExecutable(const std::string& name) const;
        void ConnectModels();
        void DoFastRun();
        void DoFit();
        void Draw();
        void DrawNullclines();
        void DrawPhasePortrait();
        ViewRect DrawTimePlot(bool replot_now);
        void DrawVectorField();
        void InitBuffers();
        void InitDefaultModel();
        void InitDraw();
        void InitModels(const std::vector<ParamModelBase*>* models = nullptr,
                        ConditionModel* conditions = nullptr);
        void InitPlots();
        const std::vector<QColor> InitTPColors() const;
        bool IsVFPresent() const;
        void LoadModel(const std::string& file_name);
        void ResetPhasePlotAxes();
        void ResetResultsList(int cond_row);
        void ResumeDraw();
        void SaveFigure(QwtPlot* fig, const QString& name, const QSizeF& size) const;
        void SaveModel(const std::string& file_name);
        void SetButtonsEnabled(bool is_enabled);
        void SetParamsEnabled(bool is_enabled);
        void UpdateLists();
        void UpdateNotes();
        void UpdateNullclines();
        void UpdateParamEditor();
        void UpdatePulseVList(); // ### There should be a way to make this automatic...
        void UpdateSlider(int index);
        void UpdateSliderPList();
        void UpdateResultsModel(int cond_row);
        void UpdateTimePlotTable();
        void UpdateVectorField();

        AboutGui* _aboutGui;
        FastRunGui* _fastRunGui;
        FitGui* _fitGui;
        LogGui* _logGui;
        NotesGui* _notesGui;
        ParamEditor* _paramEditor;

        // ### Get rid of all of these, store in ParserMgr, just refer to them by name
        ConditionModel* _conditions;
        ParamModelBase* _differentials,
                * _initConds,
                * _inputs, //Must be numeric
                * _variables;   //Can invoke other expressions

        std::condition_variable _condVar;
        std::string _fileName;
        volatile bool _finishedReplot;
        volatile bool _isVFAttached;
        std::vector<JobRecord> _jobs;
        Log* const _log;
        std::mutex _mutex;
        volatile bool _needClearVF, _needInitialize, _needUpdateExprns,
                    _needUpdateNullclines, _needUpdateVF;
        std::vector<QwtPlotItem*> _ncPlotItems;
        int _numSimSteps, _numTPSamples;
        ParserMgr _parserMgr;
        int _pastDVSampsCt, _pastIPSampsCt; //Samples outside the buffer
        volatile PLAY_STATE _playState;
        PLOT_MODE _plotMode;
        std::vector<QwtPlotItem*> _ppPlotItems;
        std::string _pulseResetValue;
        int _pulseParIdx;
        int _pulseStepsRemaining,
            //Consider making a little pulse struct/class
            _saveModN, _singleStepsSec, _singleTailLen;
        const std::thread::id _tid;
        const std::vector<QColor> _tpColors;
        std::vector<QwtPlotItem*> _vfPlotItems;
        int _vfStepsSec, _vfTailLen;

        std::vector< std::deque<double> > _diffPts, _varPts;
        std::deque<double> _ip;

        QwtPlotCurve* _curve;
        QwtPlotMarker* _marker;
        std::vector<QwtPlotCurve*> _tpCurves;
};

Q_DECLARE_METATYPE(MainWindow::ViewRect)

#endif // MAINWINDOW_H
