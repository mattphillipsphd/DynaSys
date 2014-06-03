#include "executable.h"

int Executable::_jobCt = 0;

Executable::Executable(const std::string& name) : CFileBase(name),
#ifdef Q_OS_WIN
    _nameExe(name + ".exe"),
#else
    _nameExe(name + ".out"),
#endif
    _state(QProcess::NotRunning)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Executable", std::this_thread::get_id());
#endif
    connect(&_proc, SIGNAL(readyReadStandardError()), this, SLOT(ReadProcError()));
    connect(&_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(ReadProcOutput()));
    connect(&_proc, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(StateChanged(QProcess::ProcessState)));
}

void Executable::Compile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Compile", std::this_thread::get_id());
#endif
    MakeCFile(parser_mgr);

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
    _compileCmd = "gcc -O3 -std=c11 " + Name() + " -lm -o " + _nameExe;
#endif
    _log->AddMesg("Compiling " + ds::StripPath(_nameExe) + " with " + _compileCmd);
    auto comp_start = std::chrono::system_clock::now();
//#ifdef Q_OS_WIN
//    int code = -1;
//#else
    int code = system(_compileCmd.c_str());
//#endif
    auto dur = std::chrono::system_clock::now() - comp_start;
    int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    _log->AddMesg(ds::StripPath(_nameExe) + " compilation ended with code " + std::to_string(code)
            + ", with a duration of " + std::to_string(dur_ms) + "ms.");
}
int Executable::Launch()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Launch", std::this_thread::get_id());
#endif
    QStringList args;
    for (const auto& it : _arguments)
        args.push_back(it.c_str());

    _proc.start(_nameExe.c_str(), args);

    _jobId = ++_jobCt;
    return _jobId;
}
int Executable::Launch(const VecStr& inputs)
{
    SetArguments(inputs);
    return Launch();
}
void Executable::WaitComplete()
{
    _proc.waitForFinished(-1);
}

void Executable::ReadProcError()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::ReadProcError", std::this_thread::get_id());
#endif
    _log->AddMesg("Executable::ReadProcError, " + ds::StripPath(_nameExe) + ": "
                  + std::string(_proc.readAllStandardError().data()));
}

void Executable::ReadProcOutput()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::ReadProcOutput", std::this_thread::get_id());
    _log->AddMesg("Executable::ReadProcOutput, " + ds::StripPath(_nameExe) + ": "
                  + std::string(_proc.readAllStandardOutput().data()));
#endif
}

void Executable::StateChanged(QProcess::ProcessState state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::StateChanged", std::this_thread::get_id());
    _log->AddMesg(ds::StripPath(_nameExe) + " process state changed, new state " + std::to_string(state));
#endif
    QProcess::ProcessState prior_state = _state;

    _state = state;

    if (state==0)
        switch (prior_state)
        {
            case QProcess::NotRunning: //0
                break;
            case QProcess::Starting: //1
                emit Finished(_jobId, false);
                break;
            case QProcess::Running: //2
                emit Finished(_jobId, true);
                break;
        }
}

void Executable::WriteDataOut(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin WriteDataOut";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            fwrite(&" + model->ShortKey(i) + ", sizeof(double), 1, fp);\n";
    out << "//End WriteDataOut";
    out << "    \n";
}
void Executable::WriteIncludes(std::ofstream& out)
{
    out << "//Compile command: " + _compileCmd + "\n\n";
    out <<
#ifdef Q_OS_WIN
           "#include <cstdio>\n"
           "#include <cstdlib>\n"
           "#include <cmath>\n"
           "#include <algorithm>\n"
#else
           "#include \"stdio.h\"\n"
           "#include \"stdlib.h\"\n"
           "#include \"math.h\"\n"
#endif
           "\n";
}
void Executable::WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                   const ParamModelBase* init_conds)
{
    out << "//Begin WriteInitArgs\n";
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( atof(argv[1]) / tau ),\n"
           "        save_mod_n = atoi(argv[2]);\n"
           "\n";

    //argv[3] is the file name
    const int NUM_AUTO_ARGS = 4;
    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = atof(argv["
               + std::to_string(i+NUM_AUTO_ARGS) + "]);\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = atof(argv["
               + std::to_string(i+num_inputs+NUM_AUTO_ARGS) + "]);\n";
    out << "//End WriteInitArgs\n";
    out << "\n";
}
void Executable::WriteMainBegin(std::ofstream& out)
{
    out <<
           "int main(int argc, char* argv[])\n"
           "{\n";
}
void Executable::WriteMainEnd(std::ofstream& out)
{
    out <<
           "    fclose(fp);\n"
           "    fprintf(stderr, \"" + ds::StripPath(_nameExe) + " exited without crashing.\\n\");\n"
           "    return 0;\n"
           "}\n";
}
void Executable::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
    out << "//Begin WriteOutputHeader\n";
    out <<
           "    FILE* fp;\n"
           "    fp = fopen(argv[3], \"wb\");\n"
           "    if (fp==0) return 1;"
           "    \n"
           "    const int vnum = " + std::to_string(ds::VersionNum()) + ";\n"
           "    fwrite(&vnum, sizeof(int), 1, fp);\n"
           "    \n"
           "    const int num_fields = "
                    + std::to_string(variables->NumPars() + diffs->NumPars()) + ";\n"
           "    fwrite(&num_fields, sizeof(int), 1, fp);\n"
           "    \n";

    out << "    int len;\n";
    WriteVarsOut(out, variables);
    WriteVarsOut(out, diffs);
    out <<
           "    fwrite(&save_mod_n, sizeof(int), 1, fp);\n"
           "    \n"
           "    const int num_records = num_iters / save_mod_n;\n"
           "    fwrite(&num_records, sizeof(int), 1, fp);\n";
    out << "//End WriteOutputHeader\n";
    out << "\n";
}
void Executable::WriteVarsOut(std::ofstream& out, const ParamModelBase* variables)
{
    out << "//Begin WriteVarsOut";
    const size_t num_pars = variables->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        out << "    len = " + std::to_string(variables->ShortKey(i).length()) + ";\n";
        out << "    fwrite(&len, sizeof(int), 1, fp);\n";
        out << "    fputs(\"" + variables->ShortKey(i) + "\", fp);\n";
    }
    out << "//End WriteVarsOut";
    out << "    \n";
}
