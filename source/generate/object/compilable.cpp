#include "compilable.h"

Compilable::Compilable(const std::string& name)
    : CompilableBase(name, new CFile(name))
{
}

void Compilable::Compile()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Compilable::Compile", std::this_thread::get_id());
#endif
    GetCFile()->Make();

#ifdef Q_OS_WIN
    std::string setup("\"" + DDM::VCVarsDir() + "\\vcvarsx86_amd64.bat\"");
    int setup_code = system(setup.c_str());
    _log->AddMesg("Executed " + setup + ", returning code "
                  + std::to_string(setup_code));
    std::string win_name = "\"" + Name() + "\"",
            win_name_exe = "\"" + NameExe() + "\"";
//    std::replace(win_name.begin(), win_name.end(), '/', '\\');
//    std::replace(win_name_exe.begin(), win_name_exe.end(), '/', '\\');
//    std::string cmd("\"" + DDM::VCVarsDir() + "\\cl\" /O2 " + win_name + " /Fe" + win_name_exe);
//    std::string cl_cmd("\"" + DDM::VCVarsDir() + "\\cl\" /O2 " + win_name + " /Fe" + win_name_exe);
//    std::string cmd = setup + " && " + cl_cmd;
    //C:\Windows\System32\cmd.exe /A /Q /K C:\Qt\Qt-5.2.1\5.2.1\mingw48_32\bin\qtenv2.bat
/*    QProcess proc;
    QStringList args;
    args << "/O2"
         << _name.c_str()
        << "/Fe " + QString(_nameExe.c_str());
    proc.start( "cl.exe", args );
*/
    std::string cmd = "set PATH=%PATH;C:/MinGW/bin";
    QProcess proc;
    QStringList args;
    args << "-O3"
         << "-std=c++11"
            << "-lm"
         << Name().c_str()
        << "-o " + QString(NameExe().c_str());

//    std::string cmd = "C:\\MinGW\\bin\\mingw32-g++.exe -O3 -std=c++11 " + Name() + " -lm -o " + NameExe();
//    std::string cmd = "set PATH=%PATH;C:\\MinGW\\bin && C:\\MinGW\\bin\\mingw32-g++.exe -O3 -std=c++11 " + Name() + " -lm -o " + NameExe();
#else
    std::string cmd = "gcc -O3 -std=c11 " + Name() + " -lm -o " + NameExe();
#endif
//    _log->AddMesg("Compiling " + ds::StripPath(NameExe()) + " with " + cmd);
    auto comp_start = std::chrono::system_clock::now();
//#ifdef Q_OS_WIN
//    int code = -1;
//#else
    int code = system(cmd.c_str());
//#endif
#ifdef Q_OS_WIN
    proc.start( "C:/MinGW/bin/mingw32-g++.exe", args );
#endif
    auto dur = std::chrono::system_clock::now() - comp_start;
    int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    _log->AddMesg(ds::StripPath(NameExe()) + " compilation ended with code " + std::to_string(code)
            + ", with a duration of " + std::to_string(dur_ms) + "ms.");
}
