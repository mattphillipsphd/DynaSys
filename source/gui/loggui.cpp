#include "loggui.h"
#include "ui_loggui.h"

LogGui::LogGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogGui), _log(Log::Instance())
{
    ui->setupUi(this);
    setWindowTitle("Log");
}

LogGui::~LogGui()
{
    delete ui;
}

void LogGui::closeEvent(QCloseEvent* event)
{
    disconnect(_log, SIGNAL(SendMesg(const char*, QColor)),
               this, SLOT(Append(const char*, QColor)));
    QWidget::closeEvent(event);
}
void LogGui::showEvent(QShowEvent* event)
{
    ui->txtLog->clear();
    connect(_log, SIGNAL(SendMesg(const char*, QColor)), this,
            SLOT(Append(const char*, QColor)), Qt::QueuedConnection);
    _log->SendBuffer();
    QWidget::showEvent(event);
}

void LogGui::on_btnClear_clicked()
{
    ui->txtLog->clear();
    _log->Clear();
}
void LogGui::on_btnShowParser_clicked()
{
    emit ShowParser();
}
void LogGui::on_btnWrite_clicked()
{
    std::string name = _fileName.substr(0, _fileName.find_last_of('.'));
    auto tm = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    name += "_log_" + std::string( std::ctime(&tm) );

    std::ofstream out;
    out.open(name);
    _log->Write(out);
    out.close();
}

void LogGui::Append(const char* text, QColor color)
{
    std::lock_guard<std::mutex> lock (_mutex);
    ui->txtLog->setTextColor(color);
    ui->txtLog->append(text);
}
