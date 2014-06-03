#ifndef FIT_H
#define FIT_H

#include <QObject>
#include <QProcess>

#include "../compile/executable.h"
#include "../file/fdatfilein.h"
#include "../globals/globals.h"
#include "../globals/log.h"

class Fit : public QObject
{
    Q_OBJECT

    public:
        Fit(Executable* exe, const std::string& transform, const std::string& target);

        void Cancel();
        void Launch();

    public slots:
        void ExecutableFinished(int id, bool is_normal);

    private slots:
        void ReadProcError();
        void ReadProcOutput();
        void StateChanged(QProcess::ProcessState state);

    private:
        void LaunchTransform();
        double* LoadTarget(int& size);
        double* LoadTransData(int& size);

        double _error;
        Executable* _exe;
        volatile bool _isRunning;
        Log* _log;
        QProcess _procTrans;
        QProcess::ProcessState _state;
        const std::string _target, _transform;
};

#endif // FIT_H
