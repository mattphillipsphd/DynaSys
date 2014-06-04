#include "compilable.h"

Compilable::Compilable(const std::string& name)
    : CompilableBase(name, new CFile(name))
{
}

void Compilable::Compile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Compilable::Compile", std::this_thread::get_id());
#endif
    GetCFile()->Make(parser_mgr);

#ifdef Q_OS_WIN
    std::string setup("\"" + DDM::VCVarsDir() + "\\vcvars32.bat\"");
    int setup_code = system(setup.c_str());
    _log->AddMesg("Executed " + setup + ", returning code "
                  + std::to_string(setup_code));
    std::string win_name = "\"" + _name + "\"",
            win_name_exe = "\"" + _nameExe + "\"";
    std::replace(win_name.begin(), win_name.end(), '/', '\\');
    std::replace(win_name_exe.begin(), win_name_exe.end(), '/', '\\');
    std::string cmd("\"" + DDM::VCVarsDir() + "\\cl\" /O2 " + win_name + " /Fe" + win_name_exe);
/*    std::string cl_cmd("\"" + DDM::VCVarsDir() + "\\cl\" /O2 " + win_name + " /Fe" + win_name_exe);
    std::string cmd = setup + " && " + cl_cmd;
    QProcess proc;
    QStringList args;
    args << "/O2"
         << _name.c_str()
        << "/Fe" + QString(_nameExe.c_str());
    proc.start( "cl.exe", args );
*/
#else
    std::string cmd = "gcc -O3 -std=c11 " + Name() + " -lm -o " + NameExe();
#endif
    _log->AddMesg("Compiling " + ds::StripPath(NameExe()) + " with " + cmd);
    auto comp_start = std::chrono::system_clock::now();
//#ifdef Q_OS_WIN
//    int code = -1;
//#else
    int code = system(cmd.c_str());
//#endif
    auto dur = std::chrono::system_clock::now() - comp_start;
    int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    _log->AddMesg(ds::StripPath(NameExe()) + " compilation ended with code " + std::to_string(code)
            + ", with a duration of " + std::to_string(dur_ms) + "ms.");
}
