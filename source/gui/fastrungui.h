#ifndef FASTRUNGUI_H
#define FASTRUNGUI_H

#include <QWidget>

namespace Ui {
class FastRunGui;
}

class FastRunGui : public QWidget
{
    Q_OBJECT

    public:
        explicit FastRunGui(QWidget *parent = 0);
        ~FastRunGui();

    public slots:
        void UpdatePBar(int n);

    protected:
        virtual void closeEvent(QCloseEvent*) override;
        virtual void showEvent(QShowEvent*) override;

    signals:
        void Start(int duration, int save_mod_n);
        void Finished(bool do_save);

    private slots:
        void on_btnStartFast_clicked();

    private:
        Ui::FastRunGui *ui;

        static const int DEFAULT_DURATION,
                        DEFAULT_MODN;
};

#endif // FASTRUNGUI_H
