#ifndef SYSFILEOUT_H
#define SYSFILEOUT_H

#include <string>
#include <fstream>

#include "parammodel.h"

class SysFileOut
{
    public:
        SysFileOut(const std::string& name, const std::vector<const ParamModel*>& models);

        void Save();

    private:
        const std::vector<const ParamModel*>& _models;
        const std::string _name;
        std::ofstream _out;
};

#endif // SYSFILEOUT_H
