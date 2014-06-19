#include "fitgui.h"
#include "ui_fitgui.h"

FitGui::FitGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FitGui), _mainWin(qobject_cast<QMainWindow*>(parent))
{
    ui->setupUi(this);
}

FitGui::~FitGui()
{
    delete ui;
}

void FitGui::on_btnStart_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("FitGui::on_btnStart_clicked(", std::this_thread::get_id());
#endif
    emit StartFit();

//    std::thread t( std::bind(&FitGui::Start, this) );
//    t.detach();
//    close();
}

void FitGui::Start()  // ### Do all this in MainWindow, or some other class
{
/*    Fit* fit = new Fit( new Executable("error.dsfdat") );
    int job_id = ++_jobCt;

    _jobs.push_back( Job(job_id, fit) );

    fit->Launch();
    emit Finished();
    delete fit;
*/}
