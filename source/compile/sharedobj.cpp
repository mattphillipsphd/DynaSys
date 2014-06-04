#include "sharedobj.h"

SharedObj::SharedObj(const std::string& name)
    : CompilableBase(name, new CFileSO(name)), _nameO(NameRaw() + ".o")
{
}

void SharedObj::Compile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Compilable::Compile", std::this_thread::get_id());
#endif
    GetCFile()->Make(parser_mgr);

    const std::string so_name = "lib" + NameRaw() + ".so";
    const std::string cmd1 = "gcc -O3 -std=c11 -c -fPIC " + Name() + " -lm -o " + _nameO,
            cmd2 = "gcc -O3 -std=c11 -shared -Wl,-soname," + so_name + " -o "
                    + so_name + " " + _nameO;

    _log->AddMesg("Compiling " + ds::StripPath(NameExe()) + " with " + cmd1 + ", " + cmd2);
    auto comp_start = std::chrono::system_clock::now();

    int code1 = system(cmd1.c_str());
    int code2 = system(cmd2.c_str());

    auto dur = std::chrono::system_clock::now() - comp_start;
    int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    _log->AddMesg(so_name + " compilation ended with codes " + std::to_string(code1)
            + ", " + std::to_string(code2) + " with a duration of " + std::to_string(dur_ms) + "ms.");
}
