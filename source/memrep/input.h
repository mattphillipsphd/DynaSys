#ifndef INPUT_H
#define INPUT_H

#include <cstring>

#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>

#include <QDebug>

#include "../globals/log.h"

class Input
{
    public:
        enum TYPE
        {
            UNKNOWN = -1,
            TXT_FILE,
            GAMMA_RAND,
            NORM_RAND,
            UNI_RAND,
            USER
        };
        static TYPE Type(const std::string& text);
        static const std::string GAMMA_RAND_STR,
                                NORM_RAND_STR,
                                UNI_RAND_STR;

        Input(double* const value);
        Input(const Input& other);
        Input& operator=(const Input& other);
        ~Input();

        void GenerateInput(TYPE type);
        void LoadInput(const std::string& file_name);
        void NextInput(int n = 1);

        TYPE Type() const { return _type; }

    private:
        static const size_t INPUT_EXP,
                            INPUT_SIZE,
                            INPUT_MASK;

        void DeepCopy(const Input& other);
        template<typename T>
        void GenerateRandInput(T& distribution);
        void ResetInput();

        size_t _ct;
        double* _input;
        Log* _log;
        double* const _value;
//        std::mutex _mutex;
        TYPE _type;
};

#endif // INPUT_H
