#ifndef FITGUI_H
#define FITGUI_H

#include <thread>

#include <QMainWindow>
#include <QWidget>

#include "../generate/object/executable.h"
#include "../generate/fit.h"
#include "../globals/globals.h"
#include "../globals/scopetracker.h"

namespace Ui {
class FitGui;
}

class FitGui : public QWidget
{
    Q_OBJECT

    public:
        explicit FitGui(QWidget *parent = 0);
        ~FitGui();

    signals:
        void StartFit();
        void Finished();

    private slots:
        void on_btnStart_clicked();

    private:
        Ui::FitGui *ui;

        struct Job
        {
            Job(int id_, Fit* fit_) : id(id_), fit(fit_) {}
            int id;
            Fit* fit;
        };

        void Start();

        static int _jobCt;

        Executable* _exe;
        std::vector<Job> _jobs;
        const QMainWindow* _mainWin;
};

#endif // FITGUI_H
