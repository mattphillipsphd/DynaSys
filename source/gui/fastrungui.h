#ifndef FASTRUNGUI_H
#define FASTRUNGUI_H

#include <QFileDialog>
#include <QWidget>

#include "../file/defaultdirmgr.h"
#include "../globals/globals.h"

namespace Ui {
class FastRunGui;
}

class FastRunGui : public QWidget
{
    Q_OBJECT

    public:
        enum METHOD
        {
            UNKNOWN,
            FAST_RUN,
            COMPILED
        };

        explicit FastRunGui(QWidget *parent = 0);
        ~FastRunGui();

        void SetMethod(METHOD method) { _method = method; }

        std::string FileName() const { return _fileName; }
        std::string FullFileName() const;

    public slots:
        void UpdatePBar(int n);

    protected:
        virtual void closeEvent(QCloseEvent*) override;
        virtual void showEvent(QShowEvent*) override;

    signals:
        void StartCompiled(int duration, int save_mod_n);
        void StartFastRun(int duration, int save_mod_n);
        void Finished();

    private slots:
        void on_btnFileName_clicked();
        void on_btnStartFast_clicked();
        void on_edFullFileName_editingFinished();

    private:
        Ui::FastRunGui *ui;

        static const int DEFAULT_DURATION,
                        DEFAULT_MODN;

        std::string _fileName;
        METHOD _method;
};

#endif // FASTRUNGUI_H
