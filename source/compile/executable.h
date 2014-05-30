#ifndef EXECUTABLE_H
#define EXECUTABLE_H

#include <fstream>

#include <QFile>
#include <QProcess>
#include <QStringList>

#include "../file/defaultdirmgr.h"
#include "../globals/log.h"
#include "../globals/scopetracker.h"
#include "../memrep/parsermgr.h"

class Executable : public QObject
{
    Q_OBJECT

    public:
        Executable(const std::string& name);

        void Compile(const ParserMgr& parser_mgr);
        int Launch(const VecStr& inputs);

    signals:
        void Finished(int id, bool is_normal);

    private slots:
        void ReadProcError();
        void ReadProcOutput();
        void StateChanged(QProcess::ProcessState state);

    private:
        void MakeCFile(const ParserMgr& parser_mgr);
        std::string PreprocessExprn(const std::string& exprn) const;
        void WriteConditions(std::ofstream& out, const ConditionModel* conditions);
        void WriteDataToFile(std::ofstream& out, const ParamModelBase* model);
        void WriteFuncs(std::ofstream& out, const ParamModelBase* model);
        void WriteLoadInput(std::ofstream& out, const ParamModelBase* variables);
        void WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                           const ParamModelBase* diffs);
        void WriteVarDecls(std::ofstream& out, const ParserMgr& parser_mgr);
        void WriteVarsToFile(std::ofstream& out, const ParamModelBase* variables);

        static int _jobCt;

        int _jobId;
        Log* const _log;
        const std::string _name, _nameExe;
        QProcess _proc;
        QProcess::ProcessState _state;
};

#endif // EXECUTABLE_H
