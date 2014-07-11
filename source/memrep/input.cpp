#include "input.h"

const size_t Input::INPUT_EXP = 20;
const size_t Input::INPUT_SIZE = 16 * 1024 * 1024;
const size_t Input::INPUT_MASK = INPUT_SIZE-1;
const std::string Input::INPUT_FILE_STR = "input file";
const std::string Input::GAMMA_RAND_STR = "gamma rand";
const std::string Input::NORM_RAND_STR = "normal rand";
const std::string Input::UNI_RAND_STR = "uniform rand";

Input::TYPE Input::Type(const std::string& text)
{
    if (text.empty()) return USER;
    if (text.at(0)=='"')
        return INPUT_FILE;
    if (text==GAMMA_RAND_STR)
        return GAMMA_RAND;
    if (text==NORM_RAND_STR)
        return NORM_RAND;
    if (text==UNI_RAND_STR)
        return UNI_RAND;

    return USER;
}

Input::Input(double* const value)
    : _ct(0), _input(nullptr), _log(Log::Instance()),
      _samplesPerStep(1), _type(UNKNOWN), _value(value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::Input", std::this_thread::get_id());
#endif
}
Input::Input(const Input& other)
    : _ct(other._ct), _input(nullptr), _log(Log::Instance()),
      _samplesPerStep(other._samplesPerStep), _type(other._type), _value(other._value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::Input(const Input& other)", std::this_thread::get_id());
#endif
    DeepCopy(other);
}

Input::~Input()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::~Input", std::this_thread::get_id());
#endif
    if (_input) delete[] _input;
}

void Input::GenerateInput(TYPE type)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::GenerateInput", std::this_thread::get_id());
#endif
//    std::lock_guard<std::mutex> lock(_mutex);
    ResetInput();
    _input = new double[INPUT_SIZE];

    switch (type)
    {
        case UNKNOWN:
            throw std::runtime_error("Input::GenerateInput: Unknown Variable Type");
        case UNI_RAND:
        {
            std::uniform_real_distribution<double> uni_rand;
            GenerateRandInput(uni_rand);
            break;
        }
        case GAMMA_RAND:
        {
            std::gamma_distribution<double> gamma_rand;
            GenerateRandInput(gamma_rand);
            break;
        }
        case NORM_RAND:
        {
            std::normal_distribution<double> norm_rand;
            GenerateRandInput(norm_rand);
            break;
        }
        case INPUT_FILE:
        case USER:
            throw std::runtime_error("Input::GenerateInput: type not defined for string " + std::to_string(type));
    }

    _type = type;
    *_value = _input[_ct];
}
void Input::LoadInput(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::LoadInput", std::this_thread::get_id());
#endif
    ResetInput();
    try
    {
        std::string suffix = file_name.substr(file_name.find_last_of('.')+1);
        if (suffix=="txt")
            LoadTextInput(file_name);
        else if (suffix=="dsin")
            LoadBinInput(file_name);
        else
            throw std::runtime_error("Input::LoadInput: Bad file extension.");

        _type = INPUT_FILE;
        *_value = _input[_ct];
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::InitInput: " + std::string(e.what()));
        throw(e);
    }
}
void Input::NextInput(int n)
{
    if (!_input)
        throw std::runtime_error("Input::NextInput: _input is null");
    _ct+=n;
    _ct &= INPUT_MASK;
    *_value = _input[_ct];
}

void Input::DeepCopy(const Input& other)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::DeepCopy", std::this_thread::get_id());
#endif
    if (!other._input) return;
    _input = new double[INPUT_SIZE];
    memcpy(_input,  other._input, INPUT_SIZE*sizeof(_input[0]));
}
template<typename T>
void Input::GenerateRandInput(T& distribution)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::GenerateRandInput", std::this_thread::get_id());
#endif
    if (!_input)
        throw std::runtime_error("Input::GenerateRandInput: _input is null");
    std::mt19937_64 mte;
    for (int k=0; k<INPUT_SIZE; ++k)
        _input[k] = distribution(mte);
}
void Input::LoadBinInput(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::LoadBinInput", std::this_thread::get_id());
#endif
    FILE* fp = fopen(file_name.c_str(), "rb");
    if (!fp) throw std::runtime_error("Input::LoadTextInput: File "
                                                  + file_name + " failed to open.");

    int vnum;
    size_t br = fread(&vnum, sizeof(int), 1, fp);

    br = fread(&_samplesPerStep, sizeof(int), 1, fp);

    int num_elts;
    br = fread(&num_elts, sizeof(int), 1, fp);

    _input = new double[INPUT_SIZE];
    br = fread(_input, sizeof(double), std::min((int)INPUT_SIZE, num_elts), fp);
    _log->AddMesg(std::to_string(br) + " bytes read from " + file_name);
}
void Input::LoadTextInput(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::LoadTextInput", std::this_thread::get_id());
#endif
    std::ifstream file;
    file.open(file_name);
    if (!file.is_open()) throw std::runtime_error("Input::LoadTextInput: File "
                                                  + file_name + " failed to open.");

    std::string vstr;
    std::getline(file, vstr);

    std::string line;
    std::getline(file, line);
    _samplesPerStep = std::stoi(line);

    std::getline(file, line);
    const int num_elts = std::stoi(line);

    _input = new double[INPUT_SIZE];
    size_t input_ct = 0;
    for (int i=0; i<num_elts; ++i)
    {
        std::getline(file, line);
        _input[input_ct++] = std::stof(line);
    }
    while (input_ct<INPUT_SIZE)
    {
        _input[input_ct] = _input[input_ct%num_elts];
        ++input_ct;
    }
}
void Input::ResetInput()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Input::ResetInput", std::this_thread::get_id());
#endif
    _ct = 0;
    if (_input)
    {
        delete[] _input;
        _input = nullptr;
    }
    _type = UNKNOWN;
}
