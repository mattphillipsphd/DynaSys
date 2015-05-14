#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::DEFAULT_SINGLE_STEP = 10;
const int MainWindow::DEFAULT_SINGLE_TAIL = -1;
const int MainWindow::DEFAULT_VF_RES = 20;
const int MainWindow::DEFAULT_VF_STEP = 1;
const int MainWindow::DEFAULT_VF_TAIL = 1;
const int MainWindow::MAX_BUF_SIZE = 8 * 1024 * 1024;
const double MainWindow::MIN_MODEL_STEP = 1e-7;
const int MainWindow::PLOT_REFRESH = 50;
const int MainWindow::SLIDER_INT_LIM = 10000;
const int MainWindow::XY_SAMPLES_SHOWN = 128 * 1024;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
<<<<<<< HEAD
    _aboutGui(new AboutGui()), _fastRunGui(new FastRunGui()),
    _logGui(new LogGui()), _notesGui(new NotesGui()), _paramEditor(new ParamEditor()), _paramSelector(new ParamSelector()),
=======
    _aboutGui(new AboutGui()), _eventViewer(new EventViewer()), _fastRunGui(new FastRunGui()),
    _logGui(new LogGui()), _notesGui(new NotesGui()), _paramEditor(new ParamEditor()),
>>>>>>> master
    _drawMgr(DrawMgr::Instance()), _fileName(""),
    _log(Log::Instance()), _modelMgr(ModelMgr::Instance()), _numTPSamples(DrawBase::TP_WINDOW_LENGTH),
    _plotMode(SINGLE), _pulseResetValue("-666"), _pulseStepsRemaining(-1),
    _singleStepsSec(DEFAULT_SINGLE_STEP), _singleTailLen(DEFAULT_SINGLE_TAIL),
    _tid(std::this_thread::get_id()), _tpColors(ds::TraceColors()),
    _vfStepsSec(DEFAULT_VF_STEP), _vfTailLen(DEFAULT_VF_TAIL)
{
#ifdef Q_OS_WIN
    ds::InitThreadColors();
#endif

    ds::AddThread(_tid);
#ifdef DEBUG_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    std::stringstream s; s << _tid;
    qDebug() << "Enter MainWindow::MainWindow, thread id:" << s.str().c_str();
    ScopeTracker st("MainWindow::MainWindow", _tid);
#endif

    qRegisterMetaType<ViewRect>("ViewRect");

    ui->setupUi(this);

#ifdef Q_OS_WIN
//    ui->actionCompile_Run->setEnabled(false);
//    ui->actionCreate_SO->setEnabled(false);
#endif
    ui->actionFit->setEnabled(false);
    ui->cmbDiffMethod->setEnabled(false);

//    ui->qwtPhasePlot->setAutoReplot(true);
//    ui->qwtTimePlot->setAutoReplot(true);

    QStringList diff_methods;
    diff_methods << "Euler" << "Euler2" << "Runge-Kutta";
    ui->cmbDiffMethod->setModel( new QStringListModel(diff_methods) );

    QStringList modes;
    modes << "Single" << "Vector field" << "Variable View";
    ui->cmbPlotMode->setModel( new QStringListModel(modes) );

    ui->cmbPlotZ->hide();
    ui->lblPlotZ->hide();

    ui->edModelStep->setText( QString("%1").arg(ds::DEFAULT_MODEL_STEP) );
    ui->edNumTPSamples->setText( std::to_string(DrawBase::TP_WINDOW_LENGTH).c_str() );
    ui->tblTimePlot->horizontalHeader()->setStretchLastSection(true);
    ui->sldParameter->setRange(0, SLIDER_INT_LIM);
    ui->spnVFResolution->setValue(DEFAULT_VF_RES);
    ui->spnStepsPerSec->setValue(_singleStepsSec);
    ui->spnTailLength->setValue(_singleTailLen);
    ui->spnTailLength->setMaximum(XY_SAMPLES_SHOWN);

    ui->qwtPhasePlot->setAutoDelete(false);
    ui->qwtTimePlot->setAutoDelete(false);

    setWindowTitle("DynaSys " + QString(ds::VERSION_STR.c_str()) + " - default model");
    setWindowIcon( QIcon("Logo.ico") );
    //Use converticon.com to quickly create icon file

    InitDefaultModel();

    ResetPhasePlotAxes();
    UpdateTimePlotTable();

    ConnectModels();

    connect(_fastRunGui, SIGNAL(StartCompiled(int,int)), this, SLOT(StartCompiled(int,int)));
    connect(_fastRunGui, SIGNAL(StartFastRun(int,int)), this, SLOT(StartFastRun(int,int)), Qt::DirectConnection);
    connect(_fastRunGui, SIGNAL(Finished()), this, SLOT(FastRunFinished()));
    connect(this, SIGNAL(UpdateSimPBar(int)), _fastRunGui, SLOT(UpdatePBar(int)));
    connect(_logGui, SIGNAL(ShowParser()), this, SLOT(ParserToLog()));
    connect(_notesGui, SIGNAL(SaveNotes()), this, SLOT(on_actionSave_Model_triggered()));
    connect(_paramEditor, SIGNAL(CloseEditor()), this, SLOT(ParamEditorClosed()));
    connect(_paramEditor, SIGNAL(ModelChanged(void*)), this, SLOT(LoadTempModel(void*)));

    connect(&_timer, SIGNAL(timeout()), this, SLOT(Replot()));
    connect(ui->qwtPhasePlot, SIGNAL(MousePos(QPointF)), this, SLOT(UpdateMousePos(QPointF)));
    connect(ui->qwtPhasePlot, SIGNAL(MouseClick()), this, SLOT(Pause()));
    connect(ui->qwtTimePlot, SIGNAL(MousePos(QPointF)), this, SLOT(UpdateMousePos(QPointF)));
    connect(ui->qwtTimePlot, SIGNAL(MouseClick()), this, SLOT(Pause()));

    connect(_drawMgr, SIGNAL(Error()), this, SLOT(Error()));

    _aboutGui->setWindowModality(Qt::ApplicationModal);
    _fastRunGui->setWindowModality(Qt::ApplicationModal);

    connect(_eventViewer, SIGNAL(ListSelection(int)), this, SLOT(EventViewerSelection(int)));
    connect(_eventViewer, SIGNAL(Threshold(double)), this, SLOT(EventViewerThreshold(double)));
    connect(_eventViewer, SIGNAL(IsThreshAbove(bool)), this, SLOT(EventViewerIsAbove(bool)));

//    _timer.start(PLOT_REFRESH);
}
MainWindow::~MainWindow()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::~MainWindow", _tid);
#endif
    _drawMgr->Stop();
    delete ui;
    QFile temp_file(ds::TEMP_FILE.c_str());
    if (temp_file.exists()) temp_file.remove();
}
void MainWindow::ExecutableFinished(int id, bool is_normal)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ExecutableFinished", std::this_thread::get_id());
#endif
    auto it = std::find_if(_jobs.begin(), _jobs.end(), [&](const JobRecord& jrec)
    {
        return jrec.id == id;
    });
    if (it == _jobs.end())
    {
        _log->AddExcept("MainWindow::ExecutableFinished: Job id not found!");
        return;
    }
    JobRecord job = *it;

    auto dur = std::chrono::system_clock::now() - job.start;
    int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    std::string tm = dur_ms>10000
            ? std::to_string(dur_ms/1000) + "sec" : std::to_string(dur_ms) + "ms";
    _log->AddMesg("Job " + std::to_string(id) + " finished in " + tm
                  + ", exiting " + (is_normal ? "normally." : "abnormally."));

    delete job.exe;
    _jobs.erase(it);
}
void MainWindow::FastRunFinished()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::FastRunFinished", std::this_thread::get_id());
#endif
    setEnabled(true);
    _drawMgr->Stop();
    ui->btnStart->setText("Start");
    StopEventViewer();
}
void MainWindow::Error()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::Error", std::this_thread::get_id());
#endif
    _drawMgr->Stop();
    ui->btnStart->setText("Start");
    SetButtonsEnabled(true);
    SetParamsEnabled(true);
    SetSaveActionsEnabled(true);
    SetActionBtnsEnabled(true);
    setEnabled(true);
    _paramEditor->setEnabled(true);
    _logGui->show();
    _timer.stop();
}
void MainWindow::EventViewerSelection(int i)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::EventViewerSelection", _tid);
#endif
    DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
    tp->SetSpec("event_index", i);
}
void MainWindow::EventViewerThreshold(double d)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::EventViewerThreshold", _tid);
#endif
    DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
    tp->SetSpec("event_thresh", d);
}
void MainWindow::EventViewerIsAbove(bool b)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::EventViewerIsAbove", _tid);
#endif
    DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
    tp->SetSpec("thresh_above", b);
}
void MainWindow::LoadTempModel(void* models) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::LoadTempModel", _tid);
#endif
    try
    {
        const VecStr& vmodels = *static_cast<VecStr*>(models);
        SysFileOut out(ds::TEMP_MODEL_FILE);
        out.Save(vmodels, _modelMgr->GetNotes());

        LoadModel(ds::TEMP_MODEL_FILE);
        SaveModel(_fileName);
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::LoadTempModel: " + std::string(e.what()));
    }
}
void MainWindow::ParamEditorClosed() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ParamEditorClosed", _tid);
#endif
    ui->btnStart->setEnabled(true);
    SetButtonsEnabled(true);
    SetParamsEnabled(true);
    UpdateLists();
}
void MainWindow::ParserToLog() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ParserToLog", _tid);
#endif
    const DrawBase* pp = _drawMgr->GetObject(DrawBase::SINGLE);
    if (pp)
    {
        std::string parser = pp->GetParserMgr(0).ParserContents();
        std::replace(parser.begin(), parser.end(), ',', '\n');
        _log->AddMesg(parser);
    }
}
void MainWindow::Pause() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::Pause", _tid);
#endif
    switch (_drawMgr->DrawState())
    {
        case DrawBase::STOPPED:
            SetSaveActionsEnabled(true);
            break;
        case DrawBase::DRAWING:
            _drawMgr->Pause();
            ui->btnStart->setText("Resume");
            SetSaveActionsEnabled(true);
            break;
        case DrawBase::PAUSED:
            _drawMgr->Resume();
            ui->btnStart->setText("Stop");
            SetSaveActionsEnabled(false);
            break;
    }
}
void MainWindow::StartCompiled(int duration, int save_mod_n) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::StartCompiled", std::this_thread::get_id());
#endif
    _numSimSteps = duration;
    _saveModN = save_mod_n;

    try
    {
        std::string dat_file_name = _fastRunGui->FullFileName();
        size_t pos = dat_file_name.find_last_of('.');
        Executable* exe = CreateExecutable( dat_file_name.substr(0,pos) );
        connect(exe, SIGNAL(Finished(int,bool)), this, SLOT(ExecutableFinished(int,bool)),
                Qt::QueuedConnection);

        int job_id = exe->Launch();
        const VecStr& args = exe->Arguments();
        _log->AddMesg("Job " + std::to_string(job_id) + " started, with arguments:");
        int ct=1;
        for (const auto& it : args)
            _log->AddMesg(std::to_string(ct++) + ":\t" + it);

        _jobs.push_back( JobRecord(job_id, exe, std::chrono::system_clock::now()) );
    }
    catch (std::exception& e)
    {
        _log->AddExcept(std::string("MainWindow::StartCompiled: ") + e.what());
    }

    setEnabled(true);
}
void MainWindow::StartFastRun(int duration, int save_mod_n)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::StartFastRun", std::this_thread::get_id());
#endif
    _numSimSteps = duration;
    _saveModN = save_mod_n;
    _drawMgr->ClearObjects();

    std::thread t( std::bind(&MainWindow::DoFastRun, this) );
    t.detach();
    _timer.start(PLOT_REFRESH);
}
void MainWindow::UpdateMousePos(QPointF pos) //slot
{
//#ifdef DEBUG_FUNC
//    ScopeTracker st("MainWindow::UpdateMousePos", _tid);
//#endif
    ui->lblMouseX->setText( std::to_string(pos.x()).c_str() );
    ui->lblMouseY->setText( std::to_string(pos.y()).c_str() );
}
void MainWindow::UpdateTimePlot() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateTimePlot", _tid);
#endif
    if (_drawMgr->DrawState()!=DrawBase::DRAWING)
    {
        const DrawBase* pp = _drawMgr->GetObject(DrawBase::SINGLE);
        if (pp)
        {
            DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
            tp->SetNonConstOpaqueSpec("dv_data", pp->DataCopy());
            _drawMgr->Resume(DrawBase::TIME_PLOT, 1);
        }
    }
}
void MainWindow::UpdateTPData() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateTPData", _tid);
#endif
    const DrawBase* pp = _drawMgr->GetObject(DrawBase::SINGLE);
    const MapStr& specs = pp->Specs();
    DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
    tp->SetSpecs(specs);
    tp->SetNonConstOpaqueSpec("dv_data", pp->DataCopy());
}

void MainWindow::closeEvent(QCloseEvent *)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::closeEvent", _tid);
#endif
    _aboutGui->close();
    _eventViewer->close();
    _logGui->close();
    _notesGui->close();
    _paramEditor->close();
}

void MainWindow::on_actionAll_MEX_and_CUDA_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionAll_MEX_and_CUDA_triggered", _tid);
#endif
    std::string objective_fun = QFileDialog::getOpenFileName(nullptr,
                                                         "Select objective function (measure)",
                                                         DDM::MEXFilesDir().c_str()).toStdString();
    if (objective_fun.empty()) return;
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Select model file name",
                                                         DDM::MEXFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    try
    {
        DDM::SetMEXFilesDir(file_name);
        MEXFileWithMeasure mfwm(file_name, objective_fun);
        mfwm.Make();
        mfwm.MakeMFiles();
        MEXFile mf(file_name);
        mf.Make();
        mf.MakeMFiles();
        DDM::SetCudaFilesDir(file_name);
        CudaKernelWithMeasure ckwm(file_name, objective_fun);
        ckwm.Make();
        ckwm.MakeMFiles();
        CudaKernel ck(file_name);
        ck.Make();
        ck.MakeMFiles();
        _log->AddMesg("MEX file with measure " + ds::StripPath(objective_fun)
                      + ", standard MEX file, and associated m-files created."
                      "  The MEX file with measure file has an '_mm' suffix appended to it.  Both"
                      " need to be compiled with mex.");
        _log->AddMesg("CUDA kernel file with measure " + ds::StripPath(objective_fun)
                      + ", standard CUDA kernel, and associated m-files created."
                      "  The kernel file has an '_cm' suffix appended to it.  Both"
                      " need to be compiled with nvcc and the -ptx option.");
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::on_actionCUDA_kernel_with_measure_triggered: " + std::string(e.what()));
    }
}
void MainWindow::on_actionAbout_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionAbout_triggered", _tid);
#endif
    _aboutGui->show();
}

void MainWindow::on_actionClear_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionClear_triggered", _tid);
#endif
    _modelMgr->CreateModels();
    InitViews();
    UpdateLists();
}

void MainWindow::on_actionCreate_CUDA_kernel_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionCreate_CUDA_kernel_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Select CUDA kernel file name",
                                                         DDM::CudaFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    try
    {
        DDM::SetCudaFilesDir(file_name);
        CudaKernel cuda_kernel(file_name);
        cuda_kernel.Make();
        cuda_kernel.MakeMFiles();
        _log->AddMesg("CUDA kernel file " + cuda_kernel.Name() + " and associated m-files created."
                      "  The kernel needs to be compiled with nvcc using the -ptx option.");
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::on_actionCreate_CUDA_kernel_triggered: " + std::string(e.what()));
    }
}

void MainWindow::on_actionCreate_MEX_file_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionCreate_MEX_file_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Select MEX file name",
                                                         DDM::MEXFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    try
    {
        DDM::SetMEXFilesDir(file_name);
        MEXFile mex_file(file_name);
        mex_file.Make();
        mex_file.MakeMFiles();
        _log->AddMesg("MEX file " + mex_file.Name() + " and associated m-files created.");
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::on_actionCreate_MEX_file_triggered: " + std::string(e.what()));
    }
}

void MainWindow::on_actionCreate_SO_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionCreate_SO_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Select MEX file name",
                                                         "").toStdString();
    if (file_name.empty()) return;
    try
    {
        SharedObj so(file_name);
        so.Compile();
        _log->AddMesg("SO created.");
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::on_actionCreate_SO_triggered: " + std::string(e.what()));
    }
}

void MainWindow::on_actionCUDA_kernel_with_measure_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionCUDA_kernel_with_measure_triggered", _tid);
#endif
    std::string objective_fun = QFileDialog::getOpenFileName(nullptr,
                                                         "Select objective function (measure)",
                                                         DDM::CudaFilesDir().c_str()).toStdString();
    if (objective_fun.empty()) return;
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Select CUDA kernel file name",
                                                         DDM::CudaFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    try
    {
        DDM::SetCudaFilesDir(file_name);
        CudaKernelWithMeasure ckwm(file_name, objective_fun);
        ckwm.Make();
        ckwm.MakeMFiles();
        CudaKernel ck(file_name);
        ck.Make();
        ck.MakeMFiles();
        _log->AddMesg("CUDA kernel file " + ckwm.Name() + " with measure " + ds::StripPath(objective_fun)
                      + ", standard CUDA kernel, and associated m-files created."
                      "  The kernel file has an '_cm' suffix appended to it.  Both"
                      " need to be compiled with nvcc and the -ptx option.");
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::on_actionCUDA_kernel_with_measure_triggered: " + std::string(e.what()));
    }
}

void MainWindow::on_actionCompile_Run_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionCompile_Run_triggered", _tid);
#endif
    _fastRunGui->SetMethod(FastRunGui::COMPILED);
    _fastRunGui->show();
    setEnabled(false);
}

void MainWindow::on_actionEvent_Viewer_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionEvent_Viewer_triggered", _tid);
#endif
    _eventViewer->show();
}

void MainWindow::on_actionExit_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionExit_triggered", _tid);
#endif
    close();
}

void MainWindow::on_actionLoad_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionLoad_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                         "Load dynamical system",
                                                         DDM::ModelFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    _fileName = file_name;
    DDM::SetModelFilesDir(_fileName);
    LoadModel(_fileName);
}

void MainWindow::on_actionLog_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionLog_triggered", _tid);
#endif
    _logGui->SetFileName(_fileName);
    _logGui->show();
}

void MainWindow::on_actionMEX_file_with_measure_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionMEX_file_with_measure_triggered", _tid);
#endif
    std::string objective_fun = QFileDialog::getOpenFileName(nullptr,
                                                         "Select objective function (measure)",
                                                         DDM::CudaFilesDir().c_str()).toStdString();
    if (objective_fun.empty()) return;
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Select MEX file name",
                                                         DDM::MEXFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    try
    {
        DDM::SetMEXFilesDir(file_name);
        MEXFile mex_file(file_name);
        mex_file.Make();
        mex_file.MakeMFiles();
        MEXFileWithMeasure mfwm(file_name, objective_fun);
        mfwm.Make();
        mfwm.MakeMFiles();
        _log->AddMesg("MEX file " + mfwm.Name() + " and associated m-files created.");
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::on_actionCreate_MEX_file_triggered: " + std::string(e.what()));
    }
}

void MainWindow::on_actionNotes_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionNotes_triggered", _tid);
#endif
    _notesGui->show();
}

void MainWindow::on_actionParameters_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionParameters_triggered", _tid);
#endif
    SaveModel(ds::TEMP_MODEL_FILE);
    SetButtonsEnabled(false);
    SetParamsEnabled(false);
    _paramEditor->SetFileName(_fileName);
    _paramEditor->show();
}

void MainWindow::on_actionReload_Current_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionReload_Current_triggered", _tid);
#endif
    if (_fileName.empty()) InitDefaultModel();
    else LoadModel(_fileName);
}

void MainWindow::on_actionRun_Offline_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionRun_Offline_triggered", _tid);
#endif
    _fastRunGui->SetMethod(FastRunGui::FAST_RUN);
    _fastRunGui->show();
    setEnabled(false);
}

void MainWindow::on_actionSave_Data_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Data_triggered", _tid);
#endif
    if ( !QFile(ds::TEMP_FILE.c_str()).exists() ) return;
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save generated data",
                                                         DDM::SaveDataDir().c_str()).toStdString();
    if (file_name.empty()) return;
    SaveData(file_name);
}

void MainWindow::on_actionSave_Model_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Model_triggered", _tid);
#endif
    if (_fileName.empty()) return;
    SaveModel(_fileName);
}

void MainWindow::on_actionSave_Model_As_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Model_As_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save dynamical system",
                                                         DDM::ModelFilesDir().c_str()).toStdString();
    if (file_name.empty()) return;
    size_t pos = file_name.find_last_of('.');
    _fileName = (pos!=std::string::npos && file_name.substr(pos)==".txt")
            ? file_name
            : file_name + ".txt";
    DDM::SetModelFilesDir(_fileName);

    SaveModel(file_name);
    setWindowTitle(("DynaSys " + ds::VERSION_STR + " - " + file_name).c_str());
}

void MainWindow::on_actionSave_Phase_Plot_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Phase_Plot_triggered", _tid);
#endif
    SaveFigure(ui->qwtPhasePlot, "phase plot", QSizeF(100, 100));
}

void MainWindow::on_actionSave_Time_Plot_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Time_Plot_triggered", _tid);
#endif
    SaveFigure(ui->qwtTimePlot, "time plot", QSizeF(200, 75));
}

void MainWindow::on_actionSave_Vector_Field_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Vector_Field_triggered", _tid);
#endif
    SaveFigure(ui->qwtPhasePlot, "vector field", QSizeF(100, 100));
}
<<<<<<< HEAD
void MainWindow::on_actionSelector_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSelector_triggered", _tid);
#endif
    _paramSelector->show();
}
=======

>>>>>>> master
void MainWindow::on_actionSet_Init_to_Current_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSet_Init_to_Current_triggered", _tid);
#endif
    const DrawBase* pp = _drawMgr->GetObject(DrawBase::SINGLE);
    if (pp)
    {
        const ParserMgr& parser_mgr = pp->GetParserMgr(0);
        const size_t num_pars = _modelMgr->Model(ds::DIFF)->NumPars();
        for (size_t i=0; i<num_pars; ++i)
            _modelMgr->SetValue( ds::INIT, i,
                                 std::to_string(parser_mgr.ConstData(ds::DIFF)[i]) );
        ui->tblInitConds->update();
    }
}

void MainWindow::on_actionSet_Input_Home_Dir_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSet_Input_Home_Dir_triggered", _tid);
#endif
    std::string dir_name = QFileDialog::getExistingDirectory(nullptr,
                                                         "Select Input $HOME directory",
                                                         DDM::InputFilesDir().c_str()).toStdString();
    if (dir_name.empty()) return;
    DDM::SetInputFilesDir(dir_name);
}

void MainWindow::on_btnAddCondition_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddCondition_clicked", _tid);
#endif
    std::string cond = QInputDialog::getText(this, "New Condition",
                                                 "Condition (evaluates to true/false):",
                                                 QLineEdit::Normal).toStdString();
    if (!cond.empty())
        _modelMgr->AddParameter(ds::COND, cond);
}
void MainWindow::on_btnPulse_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnPulse_clicked", _tid);
#endif
    _pulseParIdx = ui->cmbPulsePars->currentIndex();
    if (_pulseParIdx==-1) _pulseParIdx = 0;
    _pulseResetValue = _modelMgr->Value(ds::INP, _pulseParIdx);
    _pulseStepsRemaining = (int)( ui->edPulseDuration->text().toDouble() / _modelMgr->ModelStep() );
    _drawMgr->GetObject(DrawBase::SINGLE)->SetSpec(
                "pulse_steps_remaining", _pulseStepsRemaining);
    double pulse = ui->edPulseValue->text().toDouble(),
            val = std::stod(_modelMgr->Value(ds::INP,_pulseParIdx));
    _modelMgr->SetValue(ds::INP, (int)_pulseParIdx, std::to_string(val+pulse) );
}
void MainWindow::on_btnAddDiff_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddDiff_clicked", _tid);
#endif
    std::string diff = QInputDialog::getText(this, "New Differential",
                                                 "Differential Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!diff.empty())
    {
        _modelMgr->AddParameter(ds::DIFF, diff + "'", ParamModelBase::Param::DEFAULT_VAL);
        _modelMgr->AddParameter(ds::INIT, diff + "(0)", ParamModelBase::Param::DEFAULT_VAL);
        UpdateTimePlotTable();
    }
}
void MainWindow::on_btnAddExpression_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddExpression_clicked", _tid);
#endif
    QModelIndex index = ui->lsConditions->currentIndex();
    if (index.parent().isValid() || index.row()==-1) return;

    std::string expr = QInputDialog::getText(this, "New Condition Result",
                                                 "Statement evaluated when condition satisfied",
                                                 QLineEdit::Normal).toStdString();
    if (expr.empty()) return;

    _modelMgr->AddCondResult(index.row(), expr);
    ResetResultsList(index.row());
}
void MainWindow::on_btnAddParameter_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddParameter_clicked", _tid);
#endif
    std::string par = QInputDialog::getText(this, "New Parameter",
                                                 "Parameter Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!par.empty())
        _modelMgr->AddParameter(ds::INP, par, ParamModelBase::Param::DEFAULT_VAL);
    UpdatePulseVList();
    UpdateSliderPList();
}
void MainWindow::on_btnAddVariable_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddVariable_clicked", _tid);
#endif
    std::string var = QInputDialog::getText(this, "New Variable",
                                                 "Variable Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!var.empty())
    {
        _modelMgr->AddParameter(ds::VAR, var, ParamModelBase::Param::DEFAULT_VAL);
        UpdateTimePlotTable();
    }
}
void MainWindow::on_btnJumpToN_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnJumpToN_clicked", _tid);
#endif
    bool ok;
    const int n = ui->edJumpToN->text().toInt(&ok);
    if (ok && n>=0)
    {
        InputMgr::Instance()->JumpToSample(n);
        DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
        const int time_now = (tp->Spec_toi("past_samps_ct")+tp->Spec_toi("dv_end"))
                * _modelMgr->ModelStep() + 0.5;
        tp->SetSpec("time_offset", n - time_now);
    }
}
void MainWindow::on_btnRemoveCondition_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveCondition_clicked", _tid);
#endif
    int row = ui->lsConditions->selectionModel()->currentIndex().row();
    if (row==-1) return;
    ui->lsConditions->model()->removeRows(row, 1, QModelIndex());
    ResetResultsList( ui->lsConditions->selectionModel()->currentIndex().row() );
}
void MainWindow::on_btnRemoveDiff_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveDiff_clicked", _tid);
#endif
    QModelIndexList rows = ui->tblDifferentials->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblDifferentials->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    ui->tblInitConds->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdateTimePlotTable();
}
void MainWindow::on_btnRemoveExpression_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveExpression_clicked", _tid);
#endif
    QModelIndexList rows = ui->lsResults->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsResults->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());

    int row = ui->lsConditions->currentIndex().row();
    if (row == -1) row = 0;
    UpdateResultsModel(row);
}
void MainWindow::on_btnRemoveParameter_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveParameter_clicked", _tid);
#endif
    QModelIndexList rows = ui->tblParameters->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblParameters->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdatePulseVList();
    UpdateSliderPList();
}
void MainWindow::on_btnRemoveVariable_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveVariable_clicked", _tid);
#endif
    QModelIndexList rows = ui->tblVariables->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblVariables->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdateTimePlotTable();
}
void MainWindow::on_btnStart_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnStart_clicked", _tid);
#endif
    switch (_drawMgr->DrawState())
    {
        case DrawBase::DRAWING:
        {
            _drawMgr->Stop();
            ui->btnStart->setText("Start");
            SetButtonsEnabled(true);            
            _paramEditor->setEnabled(true);
            _timer.stop();
            StopEventViewer();
            break;
        }
        case DrawBase::PAUSED:
            _drawMgr->Resume();
            ui->btnStart->setText("Stop");
            SetButtonsEnabled(false);
            _paramEditor->setEnabled(true);
            _timer.start(PLOT_REFRESH);
            break;
        case DrawBase::STOPPED:
        {
            SetButtonsEnabled(false);
            ui->btnStart->setText("Stop");
            _paramEditor->setEnabled(false);
            size_t num_objects = _drawMgr->NumDrawObjects();
            for (size_t i=0; i<num_objects; ++i)
            {
                DrawBase* obj = _drawMgr->GetObject(i);
                UpdateDOSpecs(obj->Type());
            }
            _drawMgr->Start();
            _timer.start(PLOT_REFRESH);
            StartEventViewer();
            break;
        }
    }
}
void MainWindow::on_cboxFitView_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cboxFitView_clicked", _tid);
#endif
    DrawBase* pp = _drawMgr->GetObject(DrawBase::SINGLE);
    if (pp)
    {
        static int xmin, xmax, ymin, ymax, xidx, yidx;
        if (state==Qt::Checked)
        {
            xidx = pp->Spec_toi("xidx");
            yidx = pp->Spec_toi("yidx");
            xmin = _modelMgr->Minimum(ds::INIT, xidx);
            xmax = _modelMgr->Maximum(ds::INIT, xidx);
            ymin = _modelMgr->Minimum(ds::INIT, yidx);
            ymax = _modelMgr->Maximum(ds::INIT, yidx);
        }
        else
        {
            _modelMgr->SetMinimum(ds::INIT, xidx, xmin);
            _modelMgr->SetMaximum(ds::INIT, xidx, xmax);
            _modelMgr->SetMinimum(ds::INIT, yidx, ymin);
            _modelMgr->SetMaximum(ds::INIT, yidx, ymax);
        }
    }
}
void MainWindow::on_cboxNullclines_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cboxNullclines_stateChanged", _tid);
#endif
    if (state==Qt::Checked)
    {
        CreateObject(DrawBase::NULL_CLINE);
        UpdateDOSpecs(DrawBase::NULL_CLINE);
        if (_drawMgr->DrawState()==DrawBase::DRAWING)
            _drawMgr->Start(DrawBase::NULL_CLINE);
        else
            _drawMgr->Start(DrawBase::NULL_CLINE, 1);
    }
    else
        _drawMgr->StopAndRemove(DrawBase::NULL_CLINE);
}
void MainWindow::on_cboxPlotZ_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cboxPlotZ_stateChanged", _tid);
#endif
    DrawBase* vv = _drawMgr->GetObject(DrawBase::VARIABLE_VIEW);
    if (vv)
        vv->SetSpec("use_z", state==Qt::Checked);
}
void MainWindow::on_cboxVectorField_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cboxVectorField_stateChanged", _tid);
#endif
    if (state==Qt::Checked)
    {
        CreateObject(DrawBase::VECTOR_FIELD);
        UpdateDOSpecs(DrawBase::VECTOR_FIELD);
        _drawMgr->Start(DrawBase::VECTOR_FIELD);
    }
    else
        _drawMgr->StopAndRemove(DrawBase::VECTOR_FIELD);
}

void MainWindow::on_cmbDiffMethod_currentIndexChanged(const QString& text)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbDiffMethod_currentIndexChanged", _tid);
#endif
    if (text=="Euler")
        _modelMgr->SetDiffMethod(ModelMgr::EULER);
    else if (text=="Euler2")
        _modelMgr->SetDiffMethod(ModelMgr::EULER2);
    else
        _modelMgr->SetDiffMethod(ModelMgr::RUNGE_KUTTA);
}
void MainWindow::on_cmbPlotX_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbPlotX_currentIndexChanged", _tid);
#endif
    _drawMgr->SetGlobalSpec("xidx", index);
    _drawMgr->Resume(DrawBase::SINGLE, 1);
}
void MainWindow::on_cmbPlotY_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbPlotY_currentIndexChanged", _tid);
#endif
    _drawMgr->SetGlobalSpec("yidx", index);
    _drawMgr->Resume(DrawBase::SINGLE, 1);
}
void MainWindow::on_cmbPlotZ_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbPlotZ_currentIndexChanged", _tid);
#endif
    _drawMgr->SetGlobalSpec("zidx", index);
    _drawMgr->Resume(DrawBase::SINGLE, 1);
}
void MainWindow::on_cmbPlotMode_currentIndexChanged(const QString& text)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbPlotMode_currentIndexChanged", _tid);
#endif
    if (text=="Single")
        _plotMode = SINGLE;
    else if (text=="Vector field")
        _plotMode = VECTOR_FIELD;
    else if (text=="Variable View")
        _plotMode = VARIABLE_VIEW;

    _drawMgr->ClearObjects();

    switch (_plotMode)
    {
        case SINGLE:
        {
            SetTPShown(true);
            SetActionBtnsEnabled(true);
            SetZPlotShown(false);
            ui->lblTailLength->setText("tail length:");
            ui->spnStepsPerSec->setValue(_singleStepsSec);
            ui->spnStepsPerSec->setMinimum(-1);
            ui->spnTailLength->setValue(_singleTailLen);
            _vfTailLen = 1;

            CreateObject(DrawBase::SINGLE);
            CreateObject(DrawBase::TIME_PLOT);
            break;
        }
        case VARIABLE_VIEW:
        {
            SetTPShown(false);
            SetActionBtnsEnabled(false);
            SetZPlotShown(true);
            ui->lblTailLength->setText("# model evals:");
            ui->spnStepsPerSec->setValue(_singleStepsSec);
            ui->spnStepsPerSec->setMinimum(1);
            ui->spnTailLength->setValue(_singleTailLen);
            _vfTailLen = 1;

            CreateObject(DrawBase::VARIABLE_VIEW);
            break;
        }
        case VECTOR_FIELD:
        {
            SetTPShown(false);
            SetActionBtnsEnabled(false);
            ui->cboxNullclines->setEnabled(true);
            SetZPlotShown(false);
            ui->lblTailLength->setText("tail length:");
            ui->spnStepsPerSec->setValue(_vfStepsSec);
            ui->spnStepsPerSec->setMinimum(1);
            ui->spnTailLength->setValue(_vfTailLen); //This updates the vector field

            CreateObject(DrawBase::VECTOR_FIELD);
            break;
        }
    }

    ResetPhasePlotAxes();
}
void MainWindow::on_cmbSlidePars_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbSlidePars_currentIndexChanged", _tid);
#endif
    if (index==-1) return;
    UpdateSlider(index);
}

void MainWindow::on_edJumpToN_returnPressed()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_edJumpToN_editingFinished", _tid);
#endif
    on_btnJumpToN_clicked();
}
void MainWindow::on_edModelStep_editingFinished()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_edModelStep_editingFinished", _tid);
#endif
    std::string text = ui->edModelStep->text().toStdString();
    size_t pos;
    double step = std::stod(text, &pos);
    if (pos>0)
    {
        _modelMgr->SetModelStep(step);
        _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _modelMgr->ModelStep() );
    }
}
void MainWindow::on_edNumTPSamples_editingFinished()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_edNumTPSamples_editingFinished", _tid);
#endif
    _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _modelMgr->ModelStep() );
    if (DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT))
        tp->SetSpec("num_samples", _numTPSamples);
}

void MainWindow::on_lsConditions_clicked(const QModelIndex& index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_lsConditions_clicked", _tid);
#endif
    const int row = index.row();
    ResetResultsList(row);
}

void MainWindow::on_sldParameter_sliderMoved(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_sldParameter_sliderMoved", _tid);
#endif
    const int index = ui->cmbSlidePars->currentIndex();
    const double range = _modelMgr->Range(ds::INP, index);
    const double pct = (double)value / (double)SLIDER_INT_LIM,
            dval = pct*range + _modelMgr->Minimum(ds::INP, index);
    _modelMgr->SetValue(ds::INP, index, std::to_string(dval));
    ui->tblParameters->update();
}
void MainWindow::on_spnStepsPerSec_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_spnStepsPerSec_valueChanged", _tid);
#endif
    _drawMgr->SetGlobalSpec("steps_per_sec", value);
    switch (_plotMode)
    {
        case SINGLE:
        case VARIABLE_VIEW:
            _singleStepsSec = value;
            break;
        case VECTOR_FIELD:
            _vfStepsSec = value;
            break;
    }
}
void MainWindow::on_spnTailLength_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_spnTailLength_valueChanged", _tid);
#endif
    _drawMgr->SetGlobalSpec("tail_length", value);
    switch (_plotMode)
    {
        case SINGLE:
        case VARIABLE_VIEW:
            _singleTailLen = value;
            break;
        case VECTOR_FIELD:
            _vfTailLen = value;
            break;
    }
}
void MainWindow::on_spnVFResolution_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_spnVFResolution_valueChanged", _tid);
#endif
    _drawMgr->SetGlobalSpec("resolution", value);
}

Executable* MainWindow::CreateExecutable(const std::string& name) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::CreateExecutable", _tid);
#endif
    Executable* exe = new Executable(name);
    exe->Compile();

    VecStr values;
    values.push_back( std::to_string(_numSimSteps) );
    values.push_back( std::to_string(_saveModN) );
    values.push_back(name + ".dsdat");
    for (size_t i=0; i<_modelMgr->Model(ds::INP)->NumPars(); ++i)
        values.push_back( _modelMgr->Model(ds::INP)->Value(i) );
    for (size_t i=0; i<_modelMgr->Model(ds::INIT)->NumPars(); ++i)
    {
        std::string val = _modelMgr->Model(ds::INIT)->Value(i);
        int idx = _modelMgr->Model(ds::INP)->KeyIndex(val);
        values.push_back( idx==-1 ? val : _modelMgr->Model(ds::INP)->Value(val) );
    }
    exe->SetArguments(values);

    return exe;
}
DrawBase* MainWindow::CreateObject(DrawBase::DRAW_TYPE draw_type)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::CreateObject", _tid);
#endif
    DSPlot* plot(nullptr);
    switch (draw_type)
    {
        case DrawBase::NULL_CLINE:
        case DrawBase::SINGLE:
        case DrawBase::VARIABLE_VIEW:
        case DrawBase::VECTOR_FIELD:
            plot = ui->qwtPhasePlot;
            break;
        case DrawBase::TIME_PLOT:
            plot = ui->qwtTimePlot;
            break;
    }
    DrawBase* draw_object = DrawBase::Create(draw_type, plot);

    switch (draw_type)
    {
        case DrawBase::NULL_CLINE:
            break;
        case DrawBase::SINGLE:
            connect(draw_object, SIGNAL(Flag1()), this, SLOT(UpdatePulseParam()), Qt::QueuedConnection);
            connect(draw_object, SIGNAL(Flag2()), this, SLOT(UpdateTPData()), Qt::BlockingQueuedConnection);
            break;
        case DrawBase::TIME_PLOT:
            break;
        case DrawBase::VARIABLE_VIEW:
            break;
        case DrawBase::VECTOR_FIELD:
            break;
    }
    connect(draw_object, SIGNAL(Error()), this, SLOT(Error()));

    _drawMgr->AddObject(draw_object);

    return draw_object;
}
void MainWindow::ConnectModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ConnectModels", _tid);
#endif
    connect(const_cast<ParamModelBase*>(_modelMgr->Model(ds::INP)), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(const_cast<ParamModelBase*>(_modelMgr->Model(ds::VAR)), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(const_cast<ParamModelBase*>(_modelMgr->Model(ds::DIFF)), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(const_cast<ParamModelBase*>(_modelMgr->Model(ds::INIT)), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(const_cast<ParamModelBase*>(_modelMgr->Model(ds::COND)), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    if (_modelMgr->Model(ds::COND)->NumPars()==0)
        delete ui->lsResults->model();
    else
        ResetResultsList(0);
}
void MainWindow::DoFastRun()
{
    ds::AddThread(std::this_thread::get_id());
#ifdef DEBUG_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    ScopeTracker st("MainWindow::DoFastRun", std::this_thread::get_id());
#endif

    DrawBase* pp = CreateObject(DrawBase::SINGLE);
    UpdateDOSpecs(DrawBase::SINGLE);

    pp->SetSpec("make_plots", false);
    pp->SetSpec("is_recording", true);
    disconnect(pp, SIGNAL(Flag1()), this, SLOT(UpdatePulseParam()));
    disconnect(pp, SIGNAL(Flag2()), this, SLOT(UpdateTPData()));

    StartEventViewer();

    const int num_iters = _numSimSteps / _modelMgr->ModelStep() + 0.5;
    _drawMgr->Start(DrawBase::SINGLE, num_iters);
}

void MainWindow::InitDefaultModel()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitDefaultModel", _tid);
#endif
    _modelMgr->CreateModels();
    InitViews();

    _modelMgr->AddParameter(ds::INP, "a", "4");
    _modelMgr->AddParameter(ds::INP, "b", "10");

    _modelMgr->AddParameter(ds::VAR, "q", "a*b"); //\"../../dcn_input_sync2.dsin\""); //Input::UNI_RAND_STR);
    _modelMgr->AddParameter(ds::VAR, "r", "u*v");

    _modelMgr->AddParameter(ds::DIFF, "v'", "(u + r + a)/b");
    _modelMgr->AddParameter(ds::DIFF, "u'", "q*(b - v)");

    _modelMgr->AddParameter(ds::INIT, "v(0)", "1");
    _modelMgr->AddParameter(ds::INIT, "u(0)", "0");
    _modelMgr->SetRange(ds::INIT, 0, -40, 40);
    _modelMgr->SetRange(ds::INIT, 1, -40, 40);

    VecStr vs;
    vs.push_back("v = 1"); vs.push_back("u = 2");
    _modelMgr->AddCondParameter("v>40", vs);
    ui->lsConditions->setModelColumn(ConditionModel::TEST);
    ResetResultsList(0);

    _modelMgr->SetModelStep(ds::DEFAULT_MODEL_STEP);
    _modelMgr->SetNotes("This is the default model.  For more interesting models look in the ParameterFiles folder.");

    _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _modelMgr->ModelStep() );

    UpdateLists();

    SaveModel(ds::TEMP_MODEL_FILE);
}

void MainWindow::InitViews()
{
#ifdef DEBUG_MW_FUNC
    ScopeTracker st("MainWindow::InitViews", std::this_thread::get_id());
#endif
    _modelMgr->SetView(ui->tblParameters, ds::INP);
    ui->tblParameters->setColumnHidden(ParamModelBase::FREEZE,true);
    ui->tblParameters->horizontalHeader()->setStretchLastSection(true);

    _modelMgr->SetView(ui->tblVariables, ds::VAR);
    ui->tblVariables->setColumnHidden(NumericModelBase::MIN,true);
    ui->tblVariables->setColumnHidden(NumericModelBase::MAX,true);
    ui->tblVariables->setColumnWidth(ParamModelBase::FREEZE,25);
    ui->tblVariables->horizontalHeader()->setStretchLastSection(true);
    CheckBoxDelegate* cbbd_v = new CheckBoxDelegate(std::vector<QColor>(), this);
    ui->tblVariables->setItemDelegateForColumn(ParamModelBase::FREEZE, cbbd_v);
    VecStr vstr;
    vstr.push_back(Input::INPUT_FILE_STR);
    vstr.push_back(Input::GAMMA_RAND_STR);
    vstr.push_back(Input::NORM_RAND_STR);
    vstr.push_back(Input::UNI_RAND_STR);
    ComboBoxDelegate* cmbd = new ComboBoxDelegate(vstr);
    ui->tblVariables->setItemDelegateForColumn(ParamModelBase::VALUE, cmbd);
    connect(cmbd, SIGNAL(ComboBoxChanged(size_t)), this, SLOT(ComboBoxChanged(size_t)));

    _modelMgr->SetView(ui->tblDifferentials, ds::DIFF);
    ui->tblDifferentials->horizontalHeader()->setStretchLastSection(true);
    ui->tblDifferentials->setColumnHidden(NumericModelBase::MIN,true);
    ui->tblDifferentials->setColumnHidden(NumericModelBase::MAX,true);
    ui->tblDifferentials->setColumnWidth(ParamModelBase::FREEZE,25);
    CheckBoxDelegate* cbbd_d = new CheckBoxDelegate(std::vector<QColor>(), this);
    ui->tblDifferentials->setItemDelegateForColumn(ParamModelBase::FREEZE, cbbd_d);

    _modelMgr->SetView(ui->tblInitConds, ds::INIT);
    ui->tblInitConds->setColumnHidden(ParamModelBase::FREEZE,true);
    ui->tblInitConds->horizontalHeader()->setStretchLastSection(true);

    _modelMgr->SetView(ui->lsConditions, ds::COND);
    ui->lsConditions->setModelColumn(ConditionModel::TEST);
    ResetResultsList(-1);
}

void MainWindow::ComboBoxChanged(size_t row) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ComboBoxChanged", _tid);
#endif
    try
    {
        std::string text = _modelMgr->Value(ds::VAR, row);
        if (text==Input::INPUT_FILE_STR)
        {
            std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                                  "Select input file",
                                                                  DDM::InputFilesDir().c_str(),
                                                                  "Input files (*.txt *.dsin)"
                                                                 ).toStdString();
            if (!file_name.empty())
            {
                const size_t pos = file_name.find( DDM::InputFilesDir() );
                if (pos!=std::string::npos)
                    file_name.replace(pos, DDM::InputFilesDir().length(), "$HOME");
                text = "\"" + file_name + "\"";
            }
            else
                text = "0";
            _modelMgr->SetValue(ds::VAR, (size_t)row, text);
        }
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::ComboBoxChanged: " + std::string(e.what()));
    }
}
void MainWindow::ExprnChanged(QModelIndex, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ExprnChanged", _tid);
#endif
    _drawMgr->SetNeedRecompute();
}
void MainWindow::ParamChanged(QModelIndex topLeft, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    assert(std::this_thread::get_id()==_tid && "Thread error: MainWindow::ParamChanged");
#endif
    int idx = topLeft.row();
    std::string exprn = _modelMgr->Model(ds::INP)->Expression(idx);
//std::cerr << exprn << std::endl;
    if (_modelMgr->AreModelsInitialized())
        try
        {
            _drawMgr->QuickEval(exprn);
        }
        catch (std::exception& e)
        {
            _log->AddMesg("MainWindow::ParamChanged: " + std::string(e.what()));
        }
    UpdateSlider( ui->cmbSlidePars->currentIndex() );
}
void MainWindow::ResultsChanged(QModelIndex, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResultsChanged", _tid);
#endif
    int cond_row = ui->lsConditions->currentIndex().row();
    if (cond_row==-1) return;
    UpdateResultsModel(cond_row);
}
void MainWindow::Replot()
{
#ifdef DEBUG_FUNC
    assert(std::this_thread::get_id()==_tid && "Thread error: MainWindow::Replot");
#endif
    try
    {
        DrawBase* pp(nullptr);
        switch (_plotMode)
        {
            case SINGLE:
                pp = _drawMgr->GetObject(DrawBase::SINGLE);
                break;
            case VARIABLE_VIEW:
                pp = _drawMgr->GetObject(DrawBase::VARIABLE_VIEW);
                break;
            case VECTOR_FIELD:
                pp = _drawMgr->GetObject(DrawBase::VECTOR_FIELD);
                break;
        }
        if (!pp) return;

        if ( pp->Spec_tob("make_plots") )
        {
            if (pp->IterCt()==0) return;
            _drawMgr->MakePlotItems();

            ViewRect pp_lims;
            switch (_plotMode)
            {
                case SINGLE:
                    pp_lims = PhasePlotLimits();
                    break;
                case VARIABLE_VIEW:
                case VECTOR_FIELD:
                {
                    pp_lims = ViewRect(pp->Spec_tod("xmin"),
                                       pp->Spec_tod("xmax"),
                                       pp->Spec_tod("ymin"),
                                       pp->Spec_tod("ymax"));
                    break;
                }
            }
            ui->qwtPhasePlot->setAxisScale( QwtPlot::xBottom, pp_lims.xmin, pp_lims.xmax );
            ui->qwtPhasePlot->setAxisScale( QwtPlot::yLeft, pp_lims.ymin, pp_lims.ymax );

            if (_plotMode==SINGLE)
            {
                DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
                const int dv_start = tp->Spec_toi("dv_start"),
                        dv_end = tp->Spec_toi("dv_end"),
                        y_tp_min = tp->Spec_toi("y_tp_min"),
                        y_tp_max = tp->Spec_toi("y_tp_max"),
                        past_samps_ct = tp->Spec_toi("past_samps_ct"),
                        time_offset = tp->Spec_toi("time_offset");
                ViewRect tp_lims( (past_samps_ct+dv_start)*_modelMgr->ModelStep()+time_offset,
                                  (past_samps_ct+dv_end)*_modelMgr->ModelStep()+time_offset,
                                  y_tp_min,
                                  y_tp_max );
                ui->qwtTimePlot->setAxisScale( QwtPlot::xBottom, tp_lims.xmin, tp_lims.xmax );
                ui->qwtTimePlot->setAxisScale( QwtPlot::yLeft, tp_lims.ymin, tp_lims.ymax );

                ui->qwtTimePlot->replot();
            }

            ui->qwtPhasePlot->replot();
        }
        else
        {
            int num_steps = pp->IterCt()*_modelMgr->ModelStep();
            if (num_steps<_numSimSteps)
                UpdateSimPBar(num_steps);
            else //Simulation finished
            {
                UpdateSimPBar(-1);
                _timer.stop();
                connect(pp, SIGNAL(Flag1()), this, SLOT(UpdatePulseParam()), Qt::QueuedConnection);
                connect(pp, SIGNAL(Flag2()), this, SLOT(UpdateTPData()), Qt::BlockingQueuedConnection);
                SaveData(_fastRunGui->FullFileName());
            }
        }
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::Replot: " + std::string(e.what()));
        if (_drawMgr->DrawState()==DrawBase::DRAWING) on_btnStart_clicked();
    }
}

void MainWindow::UpdatePulseParam() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdatePulseParam", _tid);
#endif
    _modelMgr->SetValue(ds::INP, (int)_pulseParIdx, _pulseResetValue);
    _pulseStepsRemaining = -1;
    ui->tblParameters->update();
}
void MainWindow::LoadModel(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::LoadModel", _tid);
#endif
    try
    {
        _drawMgr->ClearObjects();

        SysFileIn in(file_name);
        in.Load();
        InitViews();
        ConnectModels();

        ui->cmbPlotMode->setCurrentIndex(0);
        if (!_drawMgr->GetObject(DrawBase::SINGLE))
        {
            CreateObject(DrawBase::SINGLE);
            CreateObject(DrawBase::TIME_PLOT);
        }
        ui->edModelStep->setText( std::to_string(_modelMgr->ModelStep()).c_str() ); //This does *not* call editingFinished
        _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _modelMgr->ModelStep() );

        ResetPhasePlotAxes();
        if (file_name != ds::TEMP_MODEL_FILE) UpdateLists();
        UpdateNotes();
        UpdateParamEditor();

        setWindowTitle(("DynaSys " + ds::
                        VERSION_STR + " - " + _fileName).c_str());
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::LoadModel: " + std::string(e.what()));
    }
}
MainWindow::ViewRect MainWindow::PhasePlotLimits() const
{
    ViewRect pp_lims;
    DrawBase* pp = _drawMgr->GetObject(DrawBase::SINGLE);
    if (pp)
    {
        const int xidx = pp->Spec_toi("xidx"),
                yidx = pp->Spec_toi("yidx");
        double xmin, xmax, ymin, ymax;
        if (ui->cboxFitView->isChecked())
        {
            _modelMgr->SetMinimum(ds::INIT, xidx, pp->Spec_tod("xmin"));
            _modelMgr->SetMaximum(ds::INIT, xidx, pp->Spec_tod("xmax"));
            _modelMgr->SetMinimum(ds::INIT, yidx, pp->Spec_tod("ymin"));
            _modelMgr->SetMaximum(ds::INIT, yidx, pp->Spec_tod("ymax"));
        }
        if (ui->cboxFitLimits->isChecked() || !pp->IsSpec("xmin"))
        {
            xmin = _modelMgr->Minimum(ds::INIT, xidx);
            xmax = _modelMgr->Maximum(ds::INIT, xidx);
            ymin = _modelMgr->Minimum(ds::INIT, yidx);
            ymax = _modelMgr->Maximum(ds::INIT, yidx);
        }
        else
        {
            xmin = pp->Spec_tod("xmin");
            xmax = pp->Spec_tod("xmax");
            ymin = pp->Spec_tod("ymin");
            ymax = pp->Spec_tod("ymax");
        }
        pp_lims = ViewRect(xmin, xmax, ymin, ymax);
    }
    return pp_lims;
}
void MainWindow::ResetPhasePlotAxes()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResetPhasePlotAxes", _tid);
#endif
    QStringList xlist, ylist, zlist;

    ui->cmbPlotX->clear();
    ui->cmbPlotX->insertItems(0, xlist);
    ui->cmbPlotX->setCurrentIndex(0);

    int yidx(0), zidx(0);
    switch (_plotMode)
    {
        case SINGLE:
        case VECTOR_FIELD:
            xlist = ylist = ds::VecStrToQSList( _modelMgr->Model(ds::DIFF)->ShortKeys() );
            yidx = _modelMgr->Model(ds::DIFF)->NumPars() > 1 ? 1 : 0;
            break;
        case VARIABLE_VIEW:
            xlist = ds::VecStrToQSList( _modelMgr->Model(ds::INP)->Keys() );
            zlist = xlist += ds::VecStrToQSList( _modelMgr->Model(ds::DIFF)->ShortKeys() );
            ylist = ds::VecStrToQSList( _modelMgr->Model(ds::VAR)->Keys() );
            yidx = 0;
            zidx = zlist.size()>1 ? 1 : 0;
            break;
    }

    ui->cmbPlotX->clear();
    ui->cmbPlotX->insertItems(0, xlist);
    ui->cmbPlotX->setCurrentIndex(0);

    ui->cmbPlotY->clear();
    ui->cmbPlotY->insertItems(0, ylist);
    ui->cmbPlotY->setCurrentIndex(yidx);

    ui->cmbPlotZ->clear();
    ui->cmbPlotZ->insertItems(0, zlist);
    ui->cmbPlotZ->setCurrentIndex(zidx);
}
void MainWindow::ResetResultsList(int cond_row)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResetResultsList", _tid);
#endif
    delete ui->lsResults->model();
    if (cond_row==-1)
    {
        ui->lsResults->setModel( new QStringListModel() );
        return;
    }
    QStringList qexprns = ds::VecStrToQSList( _modelMgr->CondResults(cond_row) );
    QStringListModel* model = new QStringListModel(qexprns);
    ui->lsResults->setModel(model);
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ResultsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
}
void MainWindow::SaveFigure(QwtPlot* fig, const QString& name, const QSizeF& size) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SaveFigure", _tid);
#endif
    QString suff = name;
    suff.replace(' ', '_');
    std::string base_name = _fileName.substr(0, _fileName.find_last_of('.'));
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                     "Save " + name + " figure",
                                                     QString(base_name.c_str())
                                                         + "_" + suff + ".pdf").toStdString();
    if (file_name.empty()) return;
    QwtPlotRenderer renderer;
    renderer.renderDocument(fig, file_name.c_str(), "pdf", size);
}
void MainWindow::SaveData(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SaveData", _tid);
#endif
    size_t sep = file_name.find_last_of('/');
    std::string path = (sep==std::string::npos) ? "" : file_name.substr(0, sep);
    DDM::SetSaveDataDir(path);
    QFile old(file_name.c_str());
    if (old.exists()) old.remove();
    if ( !QFile::rename(ds::TEMP_FILE.c_str(), file_name.c_str()) )
        _log->AddExcept(" : Save of \"" + file_name + "\" failed.");
}
void MainWindow::SaveModel(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SaveModel", _tid);
#endif
    SysFileOut out(file_name);
    out.Save();
}
void MainWindow::SetActionBtnsEnabled(bool is_enabled)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetActionBtnsEnabled", _tid);
#endif
    ui->btnPulse->setEnabled(is_enabled);
    ui->cboxNullclines->setEnabled(is_enabled);
    ui->cboxRecord->setEnabled(is_enabled);
    ui->cboxVectorField->setEnabled(is_enabled);
}
void MainWindow::SetButtonsEnabled(bool is_enabled)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetButtonsEnabled", _tid);
#endif
    ui->btnAddCondition->setEnabled(is_enabled);
    ui->btnAddDiff->setEnabled(is_enabled);
    ui->btnAddExpression->setEnabled(is_enabled);
    ui->btnAddParameter->setEnabled(is_enabled);
    ui->btnAddVariable->setEnabled(is_enabled);
    ui->btnRemoveCondition->setEnabled(is_enabled);
    ui->btnRemoveDiff->setEnabled(is_enabled);
    ui->btnRemoveExpression->setEnabled(is_enabled);
    ui->btnRemoveParameter->setEnabled(is_enabled);
    ui->btnRemoveVariable->setEnabled(is_enabled);

    ui->actionClear->setEnabled(is_enabled);
    ui->actionLoad->setEnabled(is_enabled);
    ui->actionParameters->setEnabled(is_enabled);
    ui->actionReload_Current->setEnabled(is_enabled);
    ui->actionRun_Offline->setEnabled(is_enabled);
    SetSaveActionsEnabled(is_enabled);

    ui->cmbPlotMode->setEnabled(is_enabled);
    ui->spnVFResolution->setEnabled(is_enabled); //Would like to work this in, too hard currently
}
void MainWindow::SetParamsEnabled(bool is_enabled)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetParamsEnabled", _tid);
#endif
    ui->tblDifferentials->setEnabled(is_enabled);
    ui->tblInitConds->setEnabled(is_enabled);
    ui->tblParameters->setEnabled(is_enabled);
    ui->tblVariables->setEnabled(is_enabled);
    ui->lsConditions->setEnabled(is_enabled);
    ui->lsResults->setEnabled(is_enabled);
}
void MainWindow::SetSaveActionsEnabled(bool is_enabled)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetSaveActionsEnabled", _tid);
#endif
    ui->actionSave_Data->setEnabled(is_enabled);
    ui->actionSave_Model_As->setEnabled(is_enabled);
    ui->actionSave_Phase_Plot->setEnabled(is_enabled);
    ui->actionSave_Time_Plot->setEnabled(is_enabled);
    ui->actionSave_Vector_Field->setEnabled(is_enabled);
}
void MainWindow::SetTPShown(bool is_shown)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetTPShown", _tid);
#endif
    if (is_shown)
    {
        ui->qwtTimePlot->show();
        ui->tblTimePlot->show();
        ui->lblTimePlotN->show();
        ui->edNumTPSamples->show();
    }
    else
    {
        ui->qwtTimePlot->hide();
        ui->tblTimePlot->hide();
        ui->lblTimePlotN->hide();
        ui->edNumTPSamples->hide();
    }
}
void MainWindow::SetZPlotShown(bool is_shown)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetZPlotShown", _tid);
#endif
    if (is_shown)
    {
        ui->cboxPlotZ->show();
        ui->cmbPlotZ->show();
        ui->lblPlotZ->show();
    }
    else
    {
        ui->cboxPlotZ->hide();
        ui->cmbPlotZ->hide();
        ui->lblPlotZ->hide();
    }
}
void MainWindow::StartEventViewer()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::StartEventViewer", _tid);
#endif
    DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
    if (!tp) return;
    tp->SetSpec("event_index", _eventViewer->VarIndex());
    connect(tp, SIGNAL(Flag_d(double)), _eventViewer, SLOT(TimePoints(double)));
    connect(tp, SIGNAL(Flag_i(int)), _eventViewer, SLOT(Event(int)));
    _eventViewer->Start(0);
}
void MainWindow::StopEventViewer()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::StopEventViewer", _tid);
#endif
    DrawBase* tp = _drawMgr->GetObject(DrawBase::TIME_PLOT);
    if (!tp) return;
    disconnect(tp, SIGNAL(Flag_d(double)), _eventViewer, SLOT(TimePoints(double)));
    disconnect(tp, SIGNAL(Flag_i(int)), _eventViewer, SLOT(Event(int)));
    _eventViewer->Stop();
}
void MainWindow::UpdateLists()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateList", _tid);
#endif
    UpdatePulseVList();
    UpdateSliderPList();
    UpdateTimePlotTable();
    _eventViewer->SetList( _modelMgr->DiffVarList() );
    if (_modelMgr->Model(ds::COND)->NumPars()>0)
        ui->lsConditions->setCurrentIndex( ui->lsConditions->model()->index(0,0) );
}
void MainWindow::UpdateNotes()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateNotes", _tid);
#endif
    _notesGui->SetFileName(_fileName);
    _notesGui->UpdateNotes();
}
void MainWindow::UpdateParamEditor()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateParamEditor", _tid);
#endif
    SaveModel(ds::TEMP_MODEL_FILE);
    _paramEditor->SetFileName(_fileName);
    _paramEditor->UpdateParameters();
}
void MainWindow::UpdatePulseVList()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdatePulseVList", _tid);
#endif
    ui->cmbPulsePars->clear();
    VecStr keys = _modelMgr->Model(ds::INP)->Keys();
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbPulsePars->insertItem((int)i, keys.at(i).c_str());
}
void MainWindow::UpdateSlider(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateSlider", _tid);
#endif
    if (index==-1) return;
    const double val = std::stod( _modelMgr->Value(ds::INP, index) ),
            min = _modelMgr->Minimum(ds::INP, index),
            range = _modelMgr->Range(ds::INP, index);
    const int scaled_val = ((val-min)/range) * SLIDER_INT_LIM + 0.5;
    ui->sldParameter->setValue( qBound(0, scaled_val, SLIDER_INT_LIM) );
}
void MainWindow::UpdateSliderPList()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateSliderPList", _tid);
#endif
    ui->cmbSlidePars->clear();
    VecStr keys = _modelMgr->Model(ds::INP)->Keys();
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbSlidePars->insertItem((int)i, keys.at(i).c_str());
}
void MainWindow::UpdateResultsModel(int cond_row)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateResultsModel", _tid);
#endif
    QStringList items = qobject_cast<QStringListModel*>(ui->lsResults->model())->stringList();
    VecStr exprns;
    for (const auto& it : items)
        exprns.push_back(it.toStdString());
    _modelMgr->SetCondValue(cond_row, exprns);
}
void MainWindow::UpdateDOSpecs(DrawBase::DRAW_TYPE draw_type)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateDOSpecs", _tid);
#endif
    DrawBase* draw_object = _drawMgr->GetObject(draw_type);
    draw_object->SetSpec("steps_per_sec", ui->spnStepsPerSec->value());
    switch (draw_type)
    {
        case DrawBase::NULL_CLINE:
            draw_object->SetSpec("xidx", ui->cmbPlotX->currentIndex());
            draw_object->SetSpec("yidx", ui->cmbPlotY->currentIndex());
            draw_object->SetSpec("resolution", ui->spnVFResolution->value());
            draw_object->SetOpaqueSpec("colors", &_tpColors);
            break;
        case DrawBase::SINGLE:
            draw_object->SetSpec("is_recording", ui->cboxRecord->isChecked());
            draw_object->SetSpec("pulse_steps_remaining", _pulseStepsRemaining);
            draw_object->SetSpec("xidx", ui->cmbPlotX->currentIndex());
            draw_object->SetSpec("yidx", ui->cmbPlotY->currentIndex());
            draw_object->SetSpec("tail_length", ui->spnTailLength->text().toStdString());
            draw_object->SetSpec("make_plots", true);
            break;
        case DrawBase::TIME_PLOT:
            draw_object->SetSpec("event_index", -1);
            draw_object->SetSpec("event_thresh", _eventViewer->Threshold());
            draw_object->SetSpec("thresh_above", _eventViewer->IsThreshAbove());
            draw_object->SetSpec("num_samples", _numTPSamples);
            draw_object->SetSpec("time_offset", 0);
            draw_object->SetOpaqueSpec("colors", &_tpColors);
            break;
        case DrawBase::VARIABLE_VIEW:
            draw_object->SetSpec("xidx", ui->cmbPlotX->currentIndex());
            draw_object->SetSpec("yidx", ui->cmbPlotY->currentIndex());
            draw_object->SetSpec("zidx", ui->cmbPlotZ->currentIndex());
            draw_object->SetSpec("tail_length", ui->spnTailLength->value());
            draw_object->SetSpec("use_z", ui->cboxPlotZ->checkState()==Qt::Checked);
            draw_object->SetSpec("make_plots", true);
            break;
        case DrawBase::VECTOR_FIELD:
            draw_object->SetSpec("xidx", ui->cmbPlotX->currentIndex());
            draw_object->SetSpec("yidx", ui->cmbPlotY->currentIndex());
            draw_object->SetSpec("resolution", ui->spnVFResolution->value());
            draw_object->SetSpec("tail_length", ui->spnTailLength->value());
            draw_object->SetSpec("grow_tail", _plotMode==VECTOR_FIELD);
            draw_object->SetSpec("make_plots", true);
            break;
    }
}
void MainWindow::UpdateTimePlotTable()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateTimePlotTable", _tid);
#endif
    delete ui->tblTimePlot->model();
    VecStr vs = _modelMgr->DiffVarList();

    TPVTableModel* model = new TPVTableModel(vs, this);
    _modelMgr->SetTPVModel(model);
    ui->tblTimePlot->setModel(model);

    CheckBoxDelegate* cbd = new CheckBoxDelegate(_tpColors, this);
    connect(cbd, SIGNAL(MouseReleased()), this, SLOT(UpdateTimePlot()), Qt::DirectConnection);
    ui->tblTimePlot->setItemDelegateForColumn(0, cbd);
    ui->tblTimePlot->setColumnWidth(TPVTableModel::SHOW,50);
    model->setHeaderData(TPVTableModel::SHOW,Qt::Horizontal,"Show",Qt::DisplayRole);

    DSpinBoxDelegate* dsbd = new DSpinBoxDelegate(this);
    connect(dsbd, SIGNAL(DataChanged()), this, SLOT(UpdateTimePlot()), Qt::DirectConnection);
    ui->tblTimePlot->setItemDelegateForColumn(1, dsbd);
    ui->tblTimePlot->setColumnWidth(TPVTableModel::SCALE,50);
    model->setHeaderData(TPVTableModel::SCALE,Qt::Horizontal,"Scale",Qt::DisplayRole);
}
//2533
