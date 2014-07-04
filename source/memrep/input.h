#ifndef INPUT_H
#define INPUT_H

#include <cstring>

#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>

#include "../globals/log.h"
#include "../globals/scopetracker.h"

class Input
{
    public:
        static const size_t INPUT_EXP,
                            INPUT_SIZE,
                            INPUT_MASK;

        enum TYPE
        {
            UNKNOWN = -1,
            INPUT_FILE,
            GAMMA_RAND,
            NORM_RAND,
            UNI_RAND,
            USER
        };
        static TYPE Type(const std::string& text);
        static const std::string INPUT_FILE_STR,
                                GAMMA_RAND_STR,
                                NORM_RAND_STR,
                                UNI_RAND_STR;

        Input(double* const value);
        Input(const Input& other);
        ~Input();

        void GenerateInput(TYPE type);
        void LoadInput(const std::string& file_name);
        void NextInput(int n = 1);

        int SamplesPerStep() const { return _samplesPerStep; }
        TYPE Type() const { return _type; }

    private:
#ifdef __GNUG__
        Input& operator=(const Input&) = delete;
#endif

        void DeepCopy(const Input& other);
        template<typename T>
        void GenerateRandInput(T& distribution);
        void LoadBinInput(const std::string& file_name);
        void LoadTextInput(const std::string& file_name);
        void ResetInput();

        size_t _ct;
        double* _input;
        Log* const _log;
        int _samplesPerStep; //I.e. per unit time
        TYPE _type;
        double* const _value;
};

#endif // INPUT_H
