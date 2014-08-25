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
#include <QTimer>

#include <qwt_scale_div.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_renderer.h>
#include <qwt_symbol.h>

#include "aboutgui.h"
#include "checkboxdelegate.h"
#include "comboboxdelegate.h"
#include "dspinboxdelegate.h"
#include "fastrungui.h"
#include "loggui.h"
#include "notesgui.h"
#include "parameditor.h"
#include "../generate/script/cfileso.h"
#include "../generate/script/cudakernel.h"
#include "../generate/script/cudakernelwithmeasure.h"
#include "../generate/object/executable.h"
#include "../generate/script/mexfile.h"
#include "../generate/object/sharedobj.h"
#include "../file/datfileout.h"
#include "../file/defaultdirmgr.h"
#include "../file/sysfilein.h"
#include "../file/sysfileout.h"
#include "../globals/scopetracker.h"
#include "../memrep/drawmgr.h"
#include "../memrep/modelmgr.h"
#include "../models/tpvtablemodel.h"

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
            VECTOR_FIELD,
            VARIABLE_VIEW
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
                        DEFAULT_VF_RES,
                        DEFAULT_VF_STEP,
                        DEFAULT_VF_TAIL,
                        MAX_BUF_SIZE,
                        SLIDER_INT_LIM, //Because QSliders have integer increments
                        XY_SAMPLES_SHOWN;
        static const double MIN_MODEL_STEP;
            //If Qwt isn't able to draw the samples quickly enough, you get a recursive draw
            //error

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public slots:
        void Error();
        void ExecutableFinished(int id, bool is_normal);
        void FastRunFinished();
        void LoadTempModel(void* models);
        void ParamEditorClosed();
        void ParserToLog();
        void Pause();
        void StartCompiled(int duration, int save_mod_n);
        void StartFastRun(int duration, int save_mod_n);
        void UpdateMousePos(QPointF pos);
        void UpdateTimePlot();
        void UpdateTPData();

    protected:
        virtual void closeEvent(QCloseEvent *) override;

    signals:
        void UpdateSimPBar(int n);

    private slots:
        void on_actionAbout_triggered();
        void on_actionClear_triggered();
        void on_actionCreate_CUDA_kernel_triggered();
        void on_actionCreate_MEX_file_triggered();
        void on_actionCreate_SO_triggered();
        void on_actionCUDA_kernel_with_measure_triggered();
        void on_actionCompile_Run_triggered();
        void on_actionExit_triggered();
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
        void on_actionSet_Init_to_Current_triggered();
        void on_actionSet_Input_Home_Dir_triggered();

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
        void on_cboxNullclines_stateChanged(int state);
        void on_cboxPlotZ_stateChanged(int state);

        void on_cmbDiffMethod_currentIndexChanged(const QString& text);
        void on_cmbPlotX_currentIndexChanged(int index);
        void on_cmbPlotY_currentIndexChanged(int index);
        void on_cmbPlotZ_currentIndexChanged(int index);
        void on_cmbPlotMode_currentIndexChanged(const QString& text);
        void on_cmbSlidePars_currentIndexChanged(int index);

        void on_edModelStep_editingFinished();
        void on_edNumTPSamples_editingFinished();

        void on_lsConditions_clicked(const QModelIndex& index);

        void on_sldParameter_sliderMoved(int value);

        void on_spnStepsPerSec_valueChanged(int value);
        void on_spnTailLength_valueChanged(int value);
        void on_spnVFResolution_valueChanged(int value);

        void ComboBoxChanged(size_t row);
        void ExprnChanged(QModelIndex, QModelIndex);
        void ParamChanged(QModelIndex topLeft, QModelIndex bottomRight);
        void ResultsChanged(QModelIndex, QModelIndex);
        void Replot();
        void UpdatePulseParam();

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

        Executable* CreateExecutable(const std::string& name) const;
        DrawBase* CreateObject(DrawBase::DRAW_TYPE draw_type);
        void ConnectModels();
        void DoFastRun();
        void InitDefaultModel();
        const std::vector<QColor> InitTPColors() const;
        void InitViews();
        void LoadModel(const std::string& file_name);
        ViewRect PhasePlotLimits() const;
        void ResetPhasePlotAxes();
        void ResetResultsList(int cond_row);
        void SaveFigure(QwtPlot* fig, const QString& name, const QSizeF& size) const;
        void SaveData(const std::string& file_name);
        void SaveModel(const std::string& file_name);
        void SetActionBtnsEnabled(bool is_enabled);
        void SetButtonsEnabled(bool is_enabled);
        void SetParamsEnabled(bool is_enabled);
        void SetSaveActionsEnabled(bool is_enabled);
        void SetTPShown(bool is_shown);
        void SetZPlotShown(bool is_shown);
        void UpdateLists();
        void UpdateNotes();
        void UpdateParamEditor();
        void UpdatePulseVList(); // ### There should be a way to make this automatic...
        void UpdateSlider(int index);
        void UpdateSliderPList();
        void UpdateResultsModel(int cond_row);
        void UpdateDOSpecs(DrawBase::DRAW_TYPE draw_type);
        void UpdateTimePlotTable();

        AboutGui* _aboutGui;
        FastRunGui* _fastRunGui;
        LogGui* _logGui;
        NotesGui* _notesGui;
        ParamEditor* _paramEditor;

        DrawMgr* const _drawMgr;
        std::string _fileName;
        std::vector<JobRecord> _jobs;
        Log* const _log;
        ModelMgr* const _modelMgr;
        int _numSimSteps, _numTPSamples;
        PLOT_MODE _plotMode;
        std::string _pulseResetValue;
        int _pulseParIdx;
        int _pulseStepsRemaining,
            //Consider making a little pulse struct/class
            _saveModN, _singleStepsSec, _singleTailLen;
        const std::thread::id _tid;
        QTimer _timer;
        const std::vector<QColor> _tpColors;
        int _vfStepsSec, _vfTailLen;
};

Q_DECLARE_METATYPE(MainWindow::ViewRect)

#endif // MAINWINDOW_H
