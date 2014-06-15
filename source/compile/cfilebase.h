#ifndef CFILEBASE_H
#define CFILEBASE_H

#include <fstream>

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include "../file/defaultdirmgr.h"
#include "../globals/log.h"
#include "../globals/scopetracker.h"
#include "../models/variablemodel.h"
#include "../memrep/parsermgr.h"

class CFileBase : public QObject
{
    public:
        CFileBase(const std::string& name);

        void Make(const ParserMgr& parser_mgr);

        const std::string& Name() const { return _name; }

    protected:
        std::string PreprocessExprn(const std::string& exprn) const;
        virtual void MakeHFile() = 0;

        void WriteConditions(std::ofstream& out, const ConditionModel* conditions);
        virtual void WriteDataOut(std::ofstream& out, const ParamModelBase* model) = 0;
        void WriteExecVarsDiffs(std::ofstream& out, const ParamModelBase* variables,
                                const ParamModelBase* diffs);
        virtual void WriteFuncs(std::ofstream& out, const ParamModelBase* model);
        virtual void WriteGlobalConst(std::ofstream& out, const ParserMgr& parser_mgr);
        virtual void WriteIncludes(std::ofstream& out) = 0;
        virtual void WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                           const ParamModelBase* init_conds) = 0;
        void WriteInitVarsDiffs(std::ofstream& out, const ParamModelBase* variables,
                                const ParamModelBase* diffs);
        virtual void WriteMainBegin(std::ofstream& out) = 0;
        virtual void WriteMainEnd(std::ofstream& out) = 0;
        virtual void WriteLoadInput(std::ofstream& out, const ParamModelBase* variables);
        virtual void WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                           const ParamModelBase* diffs) = 0;
        virtual void WriteSaveBlockBegin(std::ofstream&) {}
        virtual void WriteSaveBlockEnd(std::ofstream&) {}
        virtual void WriteVarDecls(std::ofstream& out, const ParserMgr& parser_mgr);

        Log* const _log;

    protected:
        void ResetNameSuffix(const std::string& new_suffix);

    private:
        virtual std::string MakeName(const std::string& name) const;

        std::string _name; // ### Making this non-const so CudaKernel can revise it to .cu
};

#endif // CFILEBASE_H
