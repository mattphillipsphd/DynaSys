#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::MAX_BUF_SIZE = 1024 * 1024;
const int MainWindow::SLEEP_MS = 10;
const int MainWindow::IP_SAMPLES_SHOWN = 100;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), _isDrawing(false), _needGetParams(true), _thread(nullptr)
{
    ui->setupUi(this);

    setWindowTitle("DynaSys");

    _parameters = new ParamModel(this, "Parameters");
    _parameters->AddParameter("a", "4");
    _parameters->AddParameter("b", "10");
    ui->tblParameters->setModel(_parameters);
    ui->tblParameters->horizontalHeader()->setStretchLastSection(true);
    connect(_parameters, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);

    _variables = new ParamModel(this, "Variables");
    ui->tblVariables->setModel(_variables);
    ui->tblVariables->horizontalHeader()->setStretchLastSection(true);
    connect(_variables, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);

    _differentials = new ParamModel(this, "Differentials");
    _differentials->AddParameter("v'", "0.1*(u + a)/b");
    _differentials->AddParameter("u'", "0.1*(b - v)");
    ui->tblDifferentials->setModel(_differentials);
    ui->tblDifferentials->horizontalHeader()->setStretchLastSection(true);
    connect(_differentials, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);

    _initConds = new ParamModel(this, "InitialConds");
    _initConds->AddParameter("v(0)", "1");
    _initConds->AddParameter("u(0)", "0");
    ui->tblInitConds->setModel(_initConds);
    ui->tblInitConds->horizontalHeader()->setStretchLastSection(true);
    connect(_initConds, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);

    _conditions = new ConditionModel(this);
    VecStr vs;
    vs.push_back("v = 1"); vs.push_back("u = 2");
    _conditions->AddCondition("v>30", vs);
//    _conditions->AddCondition("v>30", {"v = 1", "u = 2"});
//    _conditions->AddCondition("u>30", {"v = 100", "u = 200"});
    ui->clmConditions->setModel(_conditions);
    connect(_conditions, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);

    connect(this, SIGNAL(DoReplot()), this, SLOT(Replot()), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (_thread) _thread->detach();
}

void MainWindow::on_actionLoad_triggered()
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                         "Load dynamical system",
                                                         "").toStdString();
    if (file_name.empty()) return;

    std::vector<ParamModel*> models;
    SysFileIn in(file_name, models);
    in.Load();

    delete _parameters;     _parameters = models[0];    ui->tblParameters->setModel(_parameters);
    delete _variables;      _variables = models[1];     ui->tblVariables->setModel(_variables);
    delete _differentials;  _differentials = models[2]; ui->tblDifferentials->setModel(_differentials);
    delete _initConds;      _initConds = models[3];     ui->tblInitConds->setModel(_initConds);
}
void MainWindow::on_actionSave_triggered()
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
    SysFileOut out(file_name, models);
    out.Save();
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
        _variables->AddParameter(var, "0");
}
void MainWindow::on_btnRemoveDiff_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblDifferentials->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblDifferentials->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    ui->tblInitConds->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnRemoveParameter_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblParameters->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblParameters->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnRemoveVariable_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblVariables->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblVariables->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnStart_clicked()
{
    if (ui->btnStart->text()=="Start")
    {
        _isDrawing = _needGetParams = true;
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
void MainWindow::Draw()
{
    ui->qwtPlot->detachItems();

    //The point indicating current value
    QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
        QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setSymbol(symbol);
    marker->attach(ui->qwtPlot);

    //The 'tail' of the plot
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setPen( Qt::black, 1 ),
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve->attach(ui->qwtPlot);

    //The inner-product plot
    QwtPlotCurve* curve_ip = new QwtPlotCurve();
    curve_ip->setPen( Qt::black, 1 ),
    curve_ip->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve_ip->attach(ui->qwtInnerProduct);

    //Create input vector(s)
    std::vector< std::vector<double> > inputs(1);
    inputs[0] = std::vector<double>(1024);
    for (int i=0; i<1024; ++i)
        inputs[0][i] = (double)rand() / (double)RAND_MAX;

    //Get all of the information from the parameter fields, introducing new variables as needed.
    int num_pars = _parameters->NumPars(),
            num_vars = _variables->NumPars(),
            num_diffs = _differentials->NumPars(),
            num_conds = _conditions->rowCount();
    for (int i=0; i<num_conds; ++i)
        _parserConds.push_back( mu::Parser() );
    std::unique_ptr<double[]> pars( new double[num_pars] ),
                                vars( new double[num_vars] ), //This is purely for muParser
                                diffs( new double[num_diffs] );
    std::vector<std::string> expressions, initializations;
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values
    std::vector< std::deque<double> > pts(num_diffs);
    std::deque<double> ip;
    bool is_initialized = false;
    while (_isDrawing)
    {
        static auto last_step = std::chrono::system_clock::now();
        auto diff = std::chrono::system_clock::now() - last_step;
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        bool need_new_step = diff_ms.count() > 1000.0 / (double)ui->spnStepsPerSec->value();
        if (!need_new_step)
            goto label;
        last_step = std::chrono::system_clock::now();
        {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_needGetParams)
        {
            //Initialize
            num_pars = _parameters->NumPars();
            num_vars = _variables->NumPars();
            num_diffs = _differentials->NumPars();
            pars.reset( new double[num_pars] );
            vars.reset( new double[num_vars] );
            diffs.reset( new double[num_diffs] );

            try
            {
                for (int i=0; i<num_pars; ++i)
                {
                    const std::string& key = _parameters->Key(i),
                            & value = _parameters->Value(i);
                    pars[i] = atof(value.c_str());
                    _parser.DefineVar(key, &pars[i]);
                    for (auto& it : _parserConds)
                        it.DefineVar(key, &pars[i]);
                }

                for (int i=0; i<num_vars; ++i)
                {
                    const std::string& key = _variables->Key(i),
                            & value = _variables->Value(i);
                    _parser.DefineVar(key, &vars[i]); //So the expression which assigns a value
                        //to this variable must get called before the variable is used!
                    for (auto& it : _parserConds)
                        it.DefineVar(key, &vars[i]);
                    expressions.push_back(key + " = " + value);
                }

                expressions.clear();
                for (int i=0; i<num_diffs; ++i)
                {
                    std::string key = _differentials->Key(i).substr(0,1),
                            init_value = _initConds->Value(i);
                    _parser.DefineVar(key, &diffs[i]); //So the expression which assigns a value
                        //to this variable must get called before the variable is used!
                    for (auto& it : _parserConds)
                        it.DefineVar(key, &diffs[i]);

                    if (!is_initialized) initializations.push_back(key + " = " + init_value);
                    expressions.push_back(key + " = " + key + " + " +_differentials->Value(i));
                }
            }
            catch (mu::ParserError& e)
            {
                std::cout << e.GetMsg() << std::endl;
            }
        }

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
            if (!is_initialized)
            {
                for (const auto& it : initializations)
                {
                    _parser.SetExpr(it);
                    _parser.Eval();
                }
                is_initialized = true;
            }

            if (_needGetParams)
            {
                std::string expr = expressions.front();
                std::for_each(expressions.cbegin()+1, expressions.cend(),
                              [&](const std::string& it)
                {
                    expr += ", " + it;
                });
                _parser.SetExpr(expr);

                for (int k=0; k<num_conds; ++k)
                {
                    expr = _conditions->Condition(k);
                    _parserConds[k].SetExpr(expr);
                }
            }
            _needGetParams = false;

            for (int k=0; k<num_steps; ++k)
            {
                vars[0] = inputs.at(0).at(k);

                _parser.Eval();
                double ip_k = 0;
                for (int i=0; i<num_diffs; ++i)
                {
                    pts[i].push_back(diffs[i]);
                    ip_k += diffs[i] * diffs[i];
                }
                ip.push_back(ip_k);
            }

            for (int k=0; k<num_conds; ++k)
            {
                if (_parserConds.at(k).Eval())
                {
                    VecStr cond_exprns = _conditions->Expressions(k);
                    for (const auto& it : cond_exprns)
                    {
                        _parserConds[k].SetExpr(it);
                        _parserConds[k].Eval();
                    }
                    _parserConds[k].SetExpr( _conditions->Condition(k) );
                }
            }
        }
        catch (mu::ParserError& e)
        {
            std::cout << e.GetMsg() << std::endl;
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
        std::cout << diffs[0] << ", " << diffs[1] << ", " << ip.back() << std::endl;
        int num_saved_pts = (int)pts[0].size(),
            tail_len = std::min(num_saved_pts, ui->spnTailLength->text().toInt());
        if (tail_len==-1) tail_len = num_saved_pts;
        QPolygonF points(tail_len);
        int ct_begin = std::max(0,num_saved_pts-tail_len);
        for (int k=0, ct=ct_begin; k<tail_len; ++k, ++ct)
            points[k] = QPointF(pts[0][ct], pts[1][ct]);
        curve->setSamples(points);

        //Plot points for the inner-product graph
        const int NUM_IP_POINTS = ip.size();
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

    _isDrawing = false;
}

void MainWindow::ParamsChanged(QModelIndex, QModelIndex) //slot
{
    _needGetParams = true;
}
void MainWindow::Replot() //slot
{
    ui->qwtPlot->replot();
    ui->qwtInnerProduct->replot();
}
