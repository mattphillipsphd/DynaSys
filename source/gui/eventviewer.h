#ifndef EVENTVIEWER_H
#define EVENTVIEWER_H

#include <QTimer>
#include <QWidget>

#include "../globals/globals.h"
#include "../globals/log.h"

#include <qwt_scale_div.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_renderer.h>
#include <qwt_symbol.h>

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
        void Start(int);
        void Stop();

        bool IsThreshAbove() const;
        double Threshold() const;
        int VarIndex() const;

    public slots:
        void Event(int ev_time);
        void TimePoints(double point_ct);

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

        void on_edBinWidth_editingFinished();
        void on_edRowLen_editingFinished();
        void on_edThresh_editingFinished();

        void RevertLabel();
        void UpdateCount();

    private:
        Ui::EventViewer *ui;

        enum BUTTON_ID
        {
            ABOVE = 0,
            BELOW
        };

        static const int MAX_BINS = 1024 * 1024;

        void ResetAll();
        void ResetData();

        int _binWidth, _ct;
        double _cv2;
        QwtPlotCurve* _sdfCurve;
        const std::vector<QColor> _colors;
        double _currentTime, _startTime;
        int _eventCounts[MAX_BINS];
        std::vector<int> _evTimes;
        QTimer _evTimer, _updateTimer;
        std::mutex _mutex;
        int _numRows, _rowLength;
};

#endif // EVENTVIEWER_H
