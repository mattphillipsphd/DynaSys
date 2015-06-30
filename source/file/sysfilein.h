#ifndef SYSFILEIN_H
#define SYSFILEIN_H

#include <string>
#include <fstream>

#include "../globals/scopetracker.h"
#include "../memrep/modelmgr.h"
#include "../memrep/notes.h"
#include "../models/parammodelbase.h"

class SysFileIn
{
    public:
        static ModelMgr::ParVariant* ReadParVariant(std::ifstream& in);

        SysFileIn(const std::string& name);

        void Load();
        void Load(VecStr& vmodels);

    private:
        std::vector<ParamModelBase*> ReadModels(int num_models);
        double ReadModelStep();
        Notes* ReadNotes();
        int ReadNumModels();
        std::vector<ModelMgr::ParVariant*> ReadParVariants();
        int ReadVersion();

        const std::string _name;
        std::ifstream _in;
};


#endif // SYSFILEIN_H
