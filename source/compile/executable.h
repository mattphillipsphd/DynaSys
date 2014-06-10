#ifndef EXECUTABLE_H
#define EXECUTABLE_H

#include "cfile.h"
#include "compilable.h"

class Executable : public Compilable
{
    Q_OBJECT

    public:
        Executable(const std::string& name);

        int Launch();
        int Launch(const VecStr& inputs);
        void WaitComplete();

        void SetArguments(const VecStr& args) {_arguments = args; }

    signals:
        void Finished(int id, bool is_normal);

    private slots:
        void ReadProcError();
        void ReadProcOutput();
        void StateChanged(QProcess::ProcessState state);

    private:
        static int _jobCt;

        VecStr _arguments;
        int _jobId;
        QProcess _proc;
        QProcess::ProcessState _state;
};

#endif // EXECUTABLE_H
