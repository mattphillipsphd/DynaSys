#include "fit.h"

Fit::Fit(Executable* exe, const std::string& transform, const std::string& target)
    : _error(std::numeric_limits<double>::max()), _exe(exe), _isRunning(false),
      _log(Log::Instance()), _target(target), _transform(transform)
{
//    connect(_exe, SIGNAL(Finished(int,bool)), this, SLOT(ExecutableFinished(int,bool)),
//            Qt::QueuedConnection);
    connect(&_procTrans, SIGNAL(readyReadStandardError()), this, SLOT(ReadProcError()));
    connect(&_procTrans, SIGNAL(readyReadStandardOutput()), this, SLOT(ReadProcOutput()));
    connect(&_procTrans, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(StateChanged(QProcess::ProcessState)));
}

void Fit::Cancel()
{
    _isRunning = false;
}
void Fit::Launch()
{
    FDatFileIn data_file("error.dsfdat"),
            targ_file(_target);
    targ_file.Read(true);
    const int N = targ_file.Length();
    const double* targ = targ_file.Data();

    _isRunning = true;
    for (int i=0; i<100; ++i) // ###
    {
        _exe->Launch();
        _exe->WaitComplete();

        LaunchTransform();
        _procTrans.waitForFinished(-1);
        data_file.Read(i==0);

#ifdef QT_DEBUG
        if (data_file.Length() != N)
            throw std::runtime_error("Fit::Launch, wrong data length");
#endif

        int old_error = _error;
        _error = 0;
        const double* dat = data_file.Data();
        for (int k=0; k<N; ++k)
            _error += (targ[k]-dat[k]) * (targ[k]-dat[k]);

        if (_error<old_error)
        {

        }

        if (!_isRunning) break;
    }
    _isRunning = false;
}

void Fit::ExecutableFinished(int id, bool is_normal)
{
/*    _log->AddMesg(std::to_string(is_normal));

    LaunchTransform();

    int len;
    double* data = LoadTarget(len);
*/}

void Fit::ReadProcError()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Fit::ReadProcError", std::this_thread::get_id());
#endif
    _log->AddMesg("Fit::ReadProcError, " + ds::StripPath(_transform) + ": "
                  + std::string(_procTrans.readAllStandardError().data()));
}

void Fit::ReadProcOutput()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Fit::ReadProcOutput", std::this_thread::get_id());
    _log->AddMesg("Fit::ReadProcOutput, " + ds::StripPath(_transform) + ": "
                  + std::string(_procTrans.readAllStandardOutput().data()));
#endif
}

void Fit::StateChanged(QProcess::ProcessState state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Fit::StateChanged", std::this_thread::get_id());
    _log->AddMesg(ds::StripPath(_transform) + " process state changed, new state " + std::to_string(state));
#endif
    QProcess::ProcessState prior_state = _state;

    _state = state;

    if (state==0)
        switch (prior_state)
        {
            case QProcess::NotRunning: //0
                break;
            case QProcess::Starting: //1
//                emit Finished(_jobId, false);
                break;
            case QProcess::Running: //2
//                emit Finished(_jobId, true);
                break;
        }
}

double* Fit::LoadTarget(int& len)
{
    FILE* fp = fopen(_target.c_str(), "rb");
    int br = fread(&len, sizeof(int), 1, fp);
    double* data = (double*)malloc(len*sizeof(double));
    br = fread(data, sizeof(double), len, fp);
    return data;
}

void Fit::LaunchTransform()
{
    QStringList args;
    args.push_back(".temp.dsdat");
    args.push_back("500");
    args.push_back("-50");
    _procTrans.start(_transform.c_str(), args);
}
