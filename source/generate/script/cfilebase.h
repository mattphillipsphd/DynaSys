#ifndef CFILEBASE_H
#define CFILEBASE_H

#include <fstream>

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include "../../file/defaultdirmgr.h"
#include "../../globals/log.h"
#include "../../globals/scopetracker.h"
#include "../../models/variablemodel.h"
#include "../../memrep/parsermgr.h"

class CFileBase : public QObject
{
    public:
        CFileBase(const std::string& name, const std::string& ext);

        void Make();

        const std::string Name() const;

    protected:
        virtual std::string FuncArgs(ds::PMODEL, size_t) const { return ""; }
        std::string PreprocessExprn(const std::string& exprn) const;
        virtual void MakeHFile() = 0;
        virtual std::string Suffix() const = 0;

        void WriteConditions(std::ofstream& out);
        virtual void WriteDataOut(std::ofstream& out, ds::PMODEL mi) = 0;
        virtual void WriteExecVarsDiffs(std::ofstream& out);
        virtual void WriteExtraFuncs(std::ofstream&) {}
        virtual void WriteFuncs(std::ofstream& out, ds::PMODEL model);
        virtual void WriteGlobalConst(std::ofstream& out);
        virtual void WriteIncludes(std::ofstream& out) = 0;
        virtual void WriteInitArgs(std::ofstream& out) = 0;
        virtual void WriteInitVarsDiffs(std::ofstream& out);
        virtual void WriteMainBegin(std::ofstream& out) = 0;
        virtual void WriteMainEnd(std::ofstream& out) = 0;
        virtual void WriteModelLoopBegin(std::ofstream& out);
        virtual void WriteModelLoopEnd(std::ofstream& out);
        virtual void WriteLoadInput(std::ofstream& out);
        virtual void WriteOutputHeader(std::ofstream& out) = 0;
        virtual void WriteSave(std::ofstream&);
        virtual void WriteSaveBlockBegin(std::ofstream&) {}
        virtual void WriteSaveBlockEnd(std::ofstream&) {}
        virtual void WriteVarDecls(std::ofstream& out);

        Log* const _log;
        ModelMgr* const _modelMgr;

    private:
        std::string MakeName(const std::string& name) const;

        const std::string _nameBase, _nameExtension;
};

#endif // CFILEBASE_H
