#ifndef MFILEBASE_H
#define MFILEBASE_H

#include "../../../globals/globals.h"
#include "../../../globals/log.h"
#include "../../../globals/scopetracker.h"
#include "../../../memrep/modelmgr.h"
#include "../../../models/variablemodel.h"

class MFileBase
{
    public:
        static const int NUM_AUTO_ARGS;

        MFileBase(const std::string& name);
        void Make() const;
        void SetSuffix(const std::string& suffix) { _suffix = suffix; }

    protected:
        virtual void Make(std::ofstream& out) const = 0;

        virtual std::string Name() const = 0;
        std::string NameDefs() const;
        std::string NameExec() const;
        std::string NameRun() const;

        Log* const _log;
        ModelMgr* const _modelMgr;

    private:
        std::string MakeName(const std::string& name) const;

        const std::string _name;
        std::string _suffix;
};

#endif // MFILEBASE_H
