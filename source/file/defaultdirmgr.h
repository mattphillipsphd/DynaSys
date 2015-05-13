#ifndef DEFAULTDIRMGR_H
#define DEFAULTDIRMGR_H

#include <fstream>

#include "../globals/globals.h"
#include "../globals/scopetracker.h"

/*
 Line 1: Model Save directory
 Line 2: Save Data directory
 Line 3: vcvars.bat command directory
 */
class DefaultDirMgr
{
    public: 
        static void ReadConfig();
        static void WriteConfig();

        static void SetCudaFilesDir(const std::string& dir);
        static void SetInputFilesDir(const std::string& dir);
        static void SetMEXFilesDir(const std::string& dir);
        static void SetModelFilesDir(const std::string& dir);
        static void SetSaveDataDir(const std::string& dir);
        static void SetVCVarsDir(const std::string& dir);
        static std::string CudaFilesDir() { return _cudaFilesDir; }
        static std::string InputFilesDir() { return _inputFilesDir; }
        static std::string MEXFilesDir() { return _mexFilesDir; }
        static std::string ModelFilesDir() { return _modelFilesDir; }
        static std::string SaveDataDir() { return _saveDataDir; }
        static std::string VCVarsDir() { return _vcvarsDir; }

    private:
        DefaultDirMgr();

        static const std::string DEFAULT_VCVARS;
        static std::string _configFileName,
                    _cudaFilesDir, _inputFilesDir,
                    _mexFilesDir, _modelFilesDir, _saveDataDir, _vcvarsDir;
};
typedef DefaultDirMgr DDM;

#endif // DEFAULTDIRMGR_H
