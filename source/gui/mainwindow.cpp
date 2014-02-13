#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::MAX_BUF_SIZE = 1024 * 1024;
const int MainWindow::SLEEP_MS = 50;
const int MainWindow::IP_SAMPLES_SHOWN = 10000;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), _isDrawing(false), _needInitialize(true),
    _pulseResetValue(std::numeric_limits<double>::min), _pulseStepsRemaining(0),
    _thread(nullptr)
{
    ui->setupUi(this);

    setWindowTitle("DynaSys " + QString(ds::VERSION_STR.c_str()) + " - default model");

    _parameters = new ParamModel(this, "Parameters");
    _parameters->AddParameter("a", "4");
    _parameters->AddParameter("b", "10");
    ui->tblParameters->setModel(_parameters);
    ui->tblParameters->horizontalHeader()->setStretchLastSection(true);
    connect(_parameters, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_parameters);

    _variables = new VariableModel(this, "Variables");
    _variables->AddParameter("q", Input::NORM_RAND_STR);
    _variables->AddParameter("r", "u*v");
    ui->tblVariables->setModel(_variables);
    AddVarDelegate(0);
    AddVarDelegate(1);
    ui->tblVariables->horizontalHeader()->setStretchLastSection(true);
    connect(_variables, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_variables);
    UpdatePulseVList();


    _differentials = new DifferentialModel(this, "Differentials");
    _differentials->AddParameter("v'", "0.1*(u + a)/b");
    _differentials->AddParameter("u'", "0.1*(b - v)");
    ui->tblDifferentials->setModel(_differentials);
    ui->tblDifferentials->horizontalHeader()->setStretchLastSection(true);
    connect(_differentials, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_differentials);

    _initConds = new InitialCondModel(this, "InitialConds");
    _initConds->AddParameter("v(0)", "1");
    _initConds->AddParameter("u(0)", "0");
    ui->tblInitConds->setModel(_initConds);
    ui->tblInitConds->horizontalHeader()->setStretchLastSection(true);
    connect(_initConds, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_initConds);

    _conditions = new ConditionModel(this);
    VecStr vs;
    vs.push_back("v = 1"); vs.push_back("u = 2");
    _conditions->AddCondition("v>30", vs);
    ui->lsConditions->setModel(_conditions);
    ui->lsConditions->setModelColumn(0);
    QStringList exprns;
    exprns << vs.at(0).c_str() << vs.at(1).c_str();
    ui->lsResults->setModel( new QStringListModel(exprns) );
//    ui->lsResults->setModelColumn(0);
    qDebug() << _conditions->columnCount();
    connect(_conditions, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.SetCondModel(_conditions);

    connect(this, SIGNAL(DoReplot()), this, SLOT(Replot()), Qt::QueuedConnection);

    _aboutGui = new AboutGui();
    _aboutGui->setWindowModality(Qt::ApplicationModal);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (_thread) _thread->detach();
    QFile temp_file(ds::TEMP_FILE.c_str());
    if (temp_file.exists()) temp_file.remove();
}

void MainWindow::on_actionAbout_triggered()
{
    _aboutGui->show();
}

void MainWindow::on_actionLoad_triggered()
{
    try
    {
        std::lock_guard<std::mutex> lock(_mutex);

        std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                             "Load dynamical system",
                                                             "../../DynaSysFiles").toStdString();
        if (file_name.empty()) return;

        std::vector<ParamModel*> models;
        ConditionModel* conditions = new ConditionModel(this);
        SysFileIn in(file_name, models, conditions);
        in.Load();

        //Have to do this before models are deleted!
        _parserMgr.ClearModels();

        delete _parameters;     _parameters = models[0];    ui->tblParameters->setModel(_parameters);
        delete _variables;      _variables = models[1];     ui->tblVariables->setModel(_variables);
        delete _differentials;  _differentials = models[2]; ui->tblDifferentials->setModel(_differentials);
        delete _initConds;      _initConds = models[3];     ui->tblInitConds->setModel(_initConds);
        delete _conditions;     _conditions = conditions;   ui->lsConditions->setModel(_conditions);

        // ### Push the models all at once
        delete ui->lsResults->model();
        _parserMgr.AddModel(_parameters);
        _parserMgr.AddModel(_variables);
        _parserMgr.AddModel(_differentials);
        _parserMgr.AddModel(_initConds);
        _parserMgr.SetCondModel(_conditions);

        for (auto it : _cmbDelegates)
            delete it;
        _cmbDelegates.clear();
        const size_t num_vars = _variables->NumPars();
        for (size_t i=0; i<num_vars; ++i)
            AddVarDelegate((int)i, _variables->Value(i));

        setWindowTitle(("DynaSys " + ds::VERSION_STR + " - " + file_name).c_str());
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
void MainWindow::on_actionSave_Data_triggered()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if ( !QFile(ds::TEMP_FILE.c_str()).exists() ) return;
    QString file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save generated data",
                                                         "");
    if (file_name.isEmpty()) return;
    QFile old(file_name);
    if (old.exists()) old.remove();
    QFile::rename(ds::TEMP_FILE.c_str(), file_name);
}
void MainWindow::on_actionSave_Model_triggered()
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save dynamical system",
                                                         "").toStdString();
    if (file_name.empty()) return;

    std::vector<const ParamModel*> models;
    models.push_back(_parameters);
    models.push_back(_variables);
    models.push_back(_differentials);
    models.push_back(_initConds);
    SysFileOut out(file_name, models, _conditions);
    out.Save();
    setWindowTitle(("DynaSys " + ds::VERSION_STR + " - " + file_name).c_str());
}

void MainWindow::on_btnAddCondition_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string cond = QInputDialog::getText(this, "New Condition",
                                                 "Condition (evaluates to true/false):",
                                                 QLineEdit::Normal).toStdString();
    if (!cond.empty())
    {
        _conditions->AddCondition(cond, VecStr());
        _parserMgr.SetConditions();
    }
}
void MainWindow::on_btnPulse_clicked()
{
//    ui->btnPulse->setEnabled(false);
    size_t idx = ui->cmbVariables->currentIndex();
    _pulseResetValue = _variables->Value(idx);
    _pulseStepsRemaining = ui->edPulseDuration->text().toInt();
    double val = ui->edPulseValue->text().toDouble();
    // ### Need to do this by hijacking the parameter entry process, so to speak
    //Really, need to eliminate MainWindow::ParamsChanged, change code so that individual
    //parameters can be manipulated without re-initializing the whole setup.  When this
    //happens it will be easy to also add a *ramp*, which would be very useful
}
void MainWindow::on_btnAddDiff_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string diff = QInputDialog::getText(this, "New Differential",
                                                 "Differential Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!diff.empty())
    {
        _differentials->AddParameter(diff + "'", "0");
        _initConds->AddParameter(diff + "(0)", "0");
    }
}
void MainWindow::on_btnAddExpression_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);

    QModelIndex index = ui->lsConditions->currentIndex();
    if (index.parent().isValid() || index.row()==-1) return;

    std::string expr = QInputDialog::getText(this, "New Condition Result",
                                                 "Statement evaluated when condition satisfied",
                                                 QLineEdit::Normal).toStdString();
    if (expr.empty()) return;

    _conditions->AddExpression(index.row(), expr);
    VecStr exprns = _conditions->Expressions(index.row());
    QStringList qexprns;
    for (const auto& it : exprns)
        qexprns << it.c_str();
    qobject_cast<QStringListModel*>(ui->lsResults->model())->setStringList(qexprns);
}
void MainWindow::on_btnAddParameter_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string par = QInputDialog::getText(this, "New Parameter",
                                                 "Parameter Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!par.empty())
        _parameters->AddParameter(par, "0");
}
void MainWindow::on_btnAddVariable_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string var = QInputDialog::getText(this, "New Variable",
                                                 "Variable Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!var.empty())
    {
        _variables->AddParameter(var, "0");
        AddVarDelegate((int)_variables->NumPars()-1);
    }
    UpdatePulseVList();
}
void MainWindow::on_btnRemoveCondition_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->lsConditions->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsConditions->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    _parserMgr.SetConditions();
}
void MainWindow::on_btnRemoveDiff_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblDifferentials->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblDifferentials->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    ui->tblInitConds->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnRemoveExpression_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->lsResults->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsResults->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());

    int row = ui->lsConditions->currentIndex().row();
    if (row == -1) row = 0;
    QStringList items = qobject_cast<QStringListModel*>(ui->lsResults->model())->stringList();
    VecStr exprns;
    for (const auto& it : items)
        exprns.push_back(it.toStdString());
    _conditions->SetExpressions(row, exprns);
}
void MainWindow::on_btnRemoveParameter_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblParameters->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblParameters->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    for (auto it : rows)
    {
        delete _cmbDelegates[ it.row() ];
        _cmbDelegates[ it.row() ] = nullptr;
    }
    std::remove(_cmbDelegates.begin(), _cmbDelegates.end(), nullptr);
}
void MainWindow::on_btnRemoveVariable_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblVariables->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblVariables->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdatePulseVList();
}
void MainWindow::on_btnStart_clicked()
{
    if (ui->btnStart->text()=="Start")
    {
        _isDrawing = _needInitialize = true;
        ui->btnAddDiff->setEnabled(false);
        ui->btnRemoveDiff->setEnabled(false);
        _thread = new std::thread( std::bind(&MainWindow::Draw, this) );
        ui->btnStart->setText("Stop");
    }
    else // "Stop"
    {
        if (_thread)
        {
            _isDrawing = false;
            _thread->detach();
            _thread = nullptr;
            ui->btnAddDiff->setEnabled(true);
            ui->btnRemoveDiff->setEnabled(true);
        }
        ui->btnStart->setText("Start");
    }
}
void MainWindow::on_lsConditions_clicked(const QModelIndex& index)
{
    const int row = index.row();
    VecStr exprns = _conditions->Expressions(row);
    QStringList qexprns;
    for (const auto& it : exprns) qexprns << it.c_str();
    delete ui->lsResults->model();
    ui->lsResults->setModel( new QStringListModel(qexprns) );
}

void MainWindow::AddVarDelegate(int row, const std::string& type)
{
    AddVarDelegate(row);//, Input::Type(type));
}
void MainWindow::AddVarDelegate(int row)
{
    VecStr vstr;// = {"unirand", "normrand"};
    vstr.push_back(Input::GAMMA_RAND_STR);
    vstr.push_back(Input::NORM_RAND_STR);
    vstr.push_back(Input::UNI_RAND_STR);
    ComboBoxDelegate* cbd = new ComboBoxDelegate(vstr);
    ui->tblVariables->setItemDelegateForRow(row, cbd);
    connect(cbd, SIGNAL(ComboBoxChanged(const QString&)), this, SLOT(ComboBoxChanged(const QString&)));
    _cmbDelegates.push_back(cbd);
}
void MainWindow::Draw()
{
    ui->qwtPlot->detachItems();
    ui->qwtInnerProduct->detachItems();

    //The point indicating current value
    QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
        QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setSymbol(symbol);
    marker->attach(ui->qwtPlot);

    //The 'tail' of the plot
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setPen( Qt::black, 1 );
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve->attach(ui->qwtPlot);

    //The inner-product plot
    QwtPlotCurve* curve_ip = new QwtPlotCurve();
    curve_ip->setPen( Qt::black, 1 );
    curve_ip->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve_ip->attach(ui->qwtInnerProduct);

    //Get all of the information from the parameter fields, introducing new variables as needed.
    const int num_diffs = (int)_differentials->NumPars(),
            num_vars = (int)_variables->NumPars();
    const double* diffs = _parserMgr.ConstData(_differentials),
            * vars = _parserMgr.ConstData(_variables);
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values
    std::vector< std::deque<double> > pts(num_diffs);
    std::deque<double> ip;

    QFile temp(ds::TEMP_FILE.c_str());
    std::string output;
    bool is_recording = ui->cboxRecord->isChecked();
    if (is_recording)
    {
        temp.open(QFile::WriteOnly | QFile::Text);
        for (size_t i=0; i<(size_t)num_diffs; ++i)
            output += _differentials->ShortKey(i) + "\t";
        for (size_t i=0; i<(size_t)num_vars; ++i)
            output += _variables->ShortKey(i)+ "\t";
        output += "\n";
        temp.write(output.c_str());
        temp.flush();
    }

    while (_isDrawing)
    {
        static auto last_step = std::chrono::system_clock::now();
        auto step_diff = std::chrono::system_clock::now() - last_step;
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_diff);
        bool need_new_step = diff_ms.count() > 1000.0 / (double)ui->spnStepsPerSec->value();
        if (!need_new_step)
            goto label;
        last_step = std::chrono::system_clock::now();
        {
        std::lock_guard<std::mutex> lock(_mutex);

        //Update the state vector with the value of the differentials.
            //Number of iterations to calculate in this refresh
        int num_steps = (double)ui->spnStepsPerSec->value() / (double)SLEEP_MS + 0.5;
        if (num_steps==0) num_steps = 1;

            //Shrink the buffer if need be
        while (pts.at(0).size() + num_steps > MAX_BUF_SIZE)
            for (int i=0; i<num_diffs; ++i)
                pts[i].pop_front();
        while (ip.size() > IP_SAMPLES_SHOWN)
            ip.pop_front();

            //Go through each expression and evaluate them
        try
        {
            if (_needInitialize)
            {
                _parserMgr.InitVars();
                _parserMgr.InitParsers();
                _parserMgr.SetExpressions();
                _parserMgr.SetConditions();
                _needInitialize = false;
            }

            for (int k=0; k<num_steps; ++k)
            {
                _parserMgr.ParserEval();
#ifdef QT_DEBUG
                std::string sd;
                for (int i=0; i<num_diffs; ++i)
                    sd += std::to_string(diffs[i]) + ", ";
                qDebug() << "Diffs: " << sd.c_str();

                std::string sv;
                for (int i=0; i<num_vars; ++i)
                    sv += std::to_string(vars[i]) + ", ";
                qDebug() << "Vars: " << sv.c_str();
#endif
                _parserMgr.ParserCondEval();

                //Record updated variables for 2d graph, inner product, and output file
                double ip_k = 0;
                output.clear();
                for (int i=0; i<num_diffs; ++i)
                {
                    pts[i].push_back(diffs[i]);
                    if (is_recording)
                        output += std::to_string(diffs[i]) + "\t";

                    ip_k += diffs[i] * diffs[i];
                }
                if (is_recording)
                {
                    for (int i=0; i<num_vars; ++i)
                        output += std::to_string(vars[i]) + "\t";
                    output += "\n";
                    temp.write(output.c_str());
                    temp.flush();
                }
                ip.push_back(ip_k);
            }

        }
        catch (mu::ParserError& e)
        {
            std::cout << e.GetMsg() << std::endl;
            qDebug() << e.GetMsg().c_str();
            break;
        }

        //A blowup will crash QwtPlot
//        if (std::isinf(diffs[0]) || std::isinf(diffs[1]) || std::isnan(diffs[0]) || std::isnan(diffs[1]))
        const double DMAX = std::numeric_limits<double>::max();
        if (diffs[0]>DMAX || diffs[1]>DMAX)
        {
            on_btnStart_clicked();
            return;
        }

        //Plot the current state vector
        marker->setValue(diffs[0], diffs[1]);
#ifdef QT_DEBUG
        if (num_steps<100)
        {
//            std::cout << diffs[0] << ", " << diffs[1] << ", " << ip.back() << std::endl;
//            qDebug() << diffs[0] << ", " << diffs[1] << ", " << ip.back();
        }
#endif
        const int num_saved_pts = (int)pts[0].size();
        int tail_len = std::min(num_saved_pts, ui->spnTailLength->text().toInt());
        if (tail_len==-1) tail_len = num_saved_pts;
        QPolygonF points(tail_len);
        int ct_begin = std::max(0,num_saved_pts-tail_len);
        for (int k=0, ct=ct_begin; k<tail_len; ++k, ++ct)
            points[k] = QPointF(pts[0][ct], pts[1][ct]);
        curve->setSamples(points);

        //Plot points for the inner-product graph
        const int NUM_IP_POINTS = (int)ip.size();
        QPolygonF points_ip(NUM_IP_POINTS);
        for (int k=0; k<NUM_IP_POINTS; ++k)
            points_ip[k] = QPointF(k, ip[k]);
        curve_ip->setSamples(points_ip);

            //Get axis limits for inner-product plot
        auto xlims_ip = std::make_pair((const double)0, (const double)(ip.size()-1));
        auto ylims_ip = std::minmax_element(ip.cbegin(), ip.cend());
        ui->qwtInnerProduct->setAxisScale( QwtPlot::xBottom, xlims_ip.first, xlims_ip.second );
        ui->qwtInnerProduct->setAxisScale( QwtPlot::yLeft, *ylims_ip.first, *ylims_ip.second );

            //Get axis limits
        auto xlims = std::minmax_element(pts[0].cbegin(), pts[0].cend()),
                ylims = std::minmax_element(pts[1].cbegin(), pts[1].cend());
        ui->qwtPlot->setAxisScale( QwtPlot::xBottom, *xlims.first, *xlims.second );
        ui->qwtPlot->setAxisScale( QwtPlot::yLeft, *ylims.first, *ylims.second );

        emit DoReplot();
        }
        label:
        std::this_thread::sleep_for( std::chrono::milliseconds(SLEEP_MS) );
    }

    temp.close();
    _isDrawing = false;
}

void MainWindow::ComboBoxChanged(const QString& text)
{
    ComboBoxDelegate* cbd = qobject_cast<ComboBoxDelegate*>(sender());
    size_t row = std::find(_cmbDelegates.cbegin(), _cmbDelegates.cend(), cbd) - _cmbDelegates.cbegin();
    _parserMgr.AssignInput(_variables, row, text.toStdString());
}
void MainWindow::ParamsChanged(QModelIndex, QModelIndex) //slot
{
    _needInitialize = true;
}
void MainWindow::Replot() //slot
{
    ui->qwtPlot->replot();
    ui->qwtInnerProduct->replot();
}
void MainWindow::UpdatePulseVList()
{
    ui->cmbVariables->clear();
    VecStr keys = _variables->Keys();
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbVariables->insertItem(i, keys.at(i).c_str());
}
