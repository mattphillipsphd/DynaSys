#include "input.h"

const size_t Input::INPUT_EXP = 20;
const size_t Input::INPUT_SIZE = 1<<20;
const size_t Input::INPUT_MASK = INPUT_SIZE-1;
const std::string Input::GAMMA_RAND_STR = "gamma rand";
const std::string Input::NORM_RAND_STR = "normal rand";
const std::string Input::UNI_RAND_STR = "uniform rand";

Input::TYPE Input::Type(const std::string& text)
{
    if (text.empty()) return USER;
    if (text.at(0)=='"')
        return TXT_FILE;
    if (text==GAMMA_RAND_STR)
        return GAMMA_RAND;
    if (text==NORM_RAND_STR)
        return NORM_RAND;
    if (text==UNI_RAND_STR)
        return UNI_RAND;

    return USER;
}

Input::Input(double* const value)
    : _ct(0), _input(nullptr), _value(value), _type(UNKNOWN)
{
}
Input::Input(const Input& other) : _ct(other._ct), _value(other._value), _type(other._type)
{
    DeepCopy(other);
}
/*Input::Input& Input::operator=(const Input& other)
{
    if (&other != this)
    {
        size_t _ct;
        double* const _value,
                * _input;
//        std::mutex _mutex;
        TYPE _type;
    }
}*/
Input::~Input()
{
    if (_input) delete[] _input;
}

void Input::GenerateInput(TYPE type)
{
//    std::lock_guard<std::mutex> lock(_mutex);
    ResetInput();
    _input = new double[INPUT_SIZE];

    switch (type)
    {
        case UNKNOWN:
            throw("Unknown Variable Type");
        case UNI_RAND:
        {
            std::uniform_real_distribution<double> uni_rand;
            GenerateRandInput(uni_rand);
//            std::cerr << "UNI_RAND" << std::endl;
            break;
        }
        case GAMMA_RAND:
        {
            std::gamma_distribution<double> gamma_rand;
            GenerateRandInput(gamma_rand);
//            std::cerr << "GAMMA_RAND" << std::endl;
            break;
        }
        case NORM_RAND:
        {
            std::normal_distribution<double> norm_rand;
            GenerateRandInput(norm_rand);
//            std::cerr << "NORM_RAND" << std::endl;
            break;
        }
        case TXT_FILE:
        case USER:
            throw("Input::GenerateInput not defined for type " + std::to_string(type));
    }

    _type = type;
    *_value = _input[_ct];
}
void Input::LoadInput(const std::string& file_name)
{
    ResetInput();
    try
    {
        std::ifstream file;
        file.open(file_name);

        std::string line;
        std::getline(file, line);

        const int num_elts = std::stoi(line);
        _input = new double[INPUT_SIZE];
        size_t input_ct = 0;
        for (int i=0; i<num_elts; ++i)
        {
            std::getline(file, line);
            _input[input_ct++] = std::stoi(line);
        }
        while (input_ct<INPUT_SIZE)
        {
            _input[input_ct] = _input[input_ct-num_elts];
            ++input_ct;
        }

        _type = TXT_FILE;
        *_value = _input[_ct];
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
void Input::NextInput(int n)
{
    if (!_input)
        throw("Input::NextInput: _input is null");
    _ct+=n;
    _ct &= INPUT_MASK;
    *_value = _input[_ct];
}

void Input::DeepCopy(const Input& other)
{
    _input = new double[INPUT_SIZE];
    memcpy(_input,  other._input, INPUT_SIZE*sizeof(_input[0]));
}
template<typename T>
void Input::GenerateRandInput(T& distribution)
{
    if (!_input)
        throw("Input::GenerateRandInput: _input is null");
    std::mt19937_64 mte;
    for (int k=0; k<INPUT_SIZE; ++k)
        _input[k] = distribution(mte);
}

void Input::ResetInput()
{
    _ct = 0;
    if (_input)
    {
        delete[] _input;
        _input = nullptr;
    }
    _type = UNKNOWN;
}
