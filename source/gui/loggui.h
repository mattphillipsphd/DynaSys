#ifndef LOGGUI_H
#define LOGGUI_H

#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>

#include <QWidget>

#include "../globals/globals.h"
#include "../globals/log.h"

namespace Ui {
class LogGui;
}

class LogGui : public QWidget
{
    Q_OBJECT

    public:
        explicit LogGui(QWidget *parent = 0);
        ~LogGui();

        void SetFileName(const std::string& file_name) { _fileName = file_name; }

    protected:
        virtual void closeEvent(QCloseEvent* event) override;
        virtual void showEvent(QShowEvent* event) override;

    signals:
        void ShowParser();

    private slots:
        void on_btnClear_clicked();
        void on_btnShowParser_clicked();
        void on_btnWrite_clicked();

        void Append(const char* text, QColor color);

    private:
        Ui::LogGui *ui;

        std::string _fileName;
        Log* _log;
        std::mutex _mutex;
};

#endif // LOGGUI_H
