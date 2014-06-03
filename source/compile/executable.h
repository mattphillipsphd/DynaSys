#ifndef EXECUTABLE_H
#define EXECUTABLE_H

#include "cfilebase.h"

class Executable : public CFileBase
{
    Q_OBJECT

    public:
        Executable(const std::string& name);

        void Compile(const ParserMgr& parser_mgr);
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

    protected:
        virtual void WriteDataOut(std::ofstream& out, const ParamModelBase* model) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                           const ParamModelBase* init_conds) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                           const ParamModelBase* diffs) override;

    private:
        void WriteVarsOut(std::ofstream& out, const ParamModelBase* variables);

        static int _jobCt;

        VecStr _arguments;
        std::string _compileCmd;
        int _jobId;
        const std::string _nameExe;
        QProcess _proc;
        QProcess::ProcessState _state;
};

#endif // EXECUTABLE_H
