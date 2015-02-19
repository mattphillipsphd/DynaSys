#ifndef EVENTVIEWER_H
#define EVENTVIEWER_H

#include <QTimer>
#include <QWidget>

#include "../globals/globals.h"
#include "../globals/log.h"

namespace Ui {
class EventViewer;
}

class EventViewer : public QWidget
{
    Q_OBJECT

    public:
        explicit EventViewer(QWidget *parent = 0);
        ~EventViewer();

        void SetList(const VecStr& vs);
        void Start(int start_time);

    public slots:
        void Event(int ev_time);

    protected:
        virtual void closeEvent(QCloseEvent*) override;
        virtual void showEvent(QShowEvent*) override;

    signals:
        void ListSelection(int i);
        void Threshold(double thresh);
        void IsThreshAbove(bool b);

    private slots:
        void on_btnReset_clicked();

        void on_buttonGroup_buttonClicked(int id);

        void on_cmbDiffVars_currentIndexChanged(int index);

        void on_edThresh_editingFinished();

        void UpdateCount();

    private:
        Ui::EventViewer *ui;

        enum BUTTON_ID
        {
            ABOVE = 0,
            BELOW
        };

        int _ct;
        int _currentTime, _startTime;
        QTimer _timer;
};

#endif // EVENTVIEWER_H
