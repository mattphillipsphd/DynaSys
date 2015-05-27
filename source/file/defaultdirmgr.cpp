#include "defaultdirmgr.h"

const std::string DefaultDirMgr::DEFAULT_VCVARS = "C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\VC\\bin";
std::string DefaultDirMgr::_configFileName = ".dsconfig.txt";
std::string DefaultDirMgr::_cudaFilesDir = "";
std::string DefaultDirMgr::_inputFilesDir = "";
std::string DefaultDirMgr::_mexFilesDir = "";
std::string DefaultDirMgr::_modelFilesDir = "";
std::string DefaultDirMgr::_saveDataDir = "";
std::string DefaultDirMgr::_vcvarsDir = "";

void DefaultDirMgr::ReadConfig()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DefaultDirMgr::ReadConfig", std::this_thread::get_id());
#endif
    std::ifstream cfg;
    cfg.open(_configFileName);
    if (!cfg.is_open())
    {
        _modelFilesDir = "";
        _saveDataDir = "";
        _vcvarsDir = DEFAULT_VCVARS;
        return;
    }
    std::string line;
    std::getline(cfg, line);
    while (line.empty()) std::getline(cfg, line);
    _modelFilesDir = line;

    std::getline(cfg, line);
    _saveDataDir = line;

    std::getline(cfg, line);
    _vcvarsDir = line;

    std::getline(cfg, line);
    _mexFilesDir = line;

    std::getline(cfg, line);
    _cudaFilesDir = line;

    std::getline(cfg, line);
    _inputFilesDir = line;
    cfg.close();
}

void DefaultDirMgr::WriteConfig()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DefaultDirMgr::WriteConfig", std::this_thread::get_id());
#endif
    std::ofstream cfg;
    cfg.open(_configFileName);
    if (!cfg.is_open())
        throw std::runtime_error("DefaultDirMgr::WriteConfig: Couldn't open " + _configFileName);
    cfg << _modelFilesDir << std::endl;
    cfg << _saveDataDir << std::endl;
    cfg << _vcvarsDir << std::endl;
    cfg << _mexFilesDir << std::endl;
    cfg << _cudaFilesDir << std::endl;
    cfg << _inputFilesDir << std::endl;
    cfg.close();
}

void DefaultDirMgr::SetCudaFilesDir(const std::string& dir)
{
    _cudaFilesDir = dir;
    WriteConfig();
}
void DefaultDirMgr::SetInputFilesDir(const std::string& dir)
{
    _inputFilesDir = dir;
    WriteConfig();
}
void DefaultDirMgr::SetMEXFilesDir(const std::string& dir)
{
    _mexFilesDir = dir;
    WriteConfig();
}
void DefaultDirMgr::SetModelFilesDir(const std::string& dir)
{
    _modelFilesDir = dir;
    WriteConfig();
}
void DefaultDirMgr::SetSaveDataDir(const std::string& dir)
{
    _saveDataDir = dir;
    WriteConfig();
}
void DefaultDirMgr::SetVCVarsDir(const std::string& dir)
{
    _vcvarsDir = dir;
    WriteConfig();
}
