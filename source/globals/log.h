#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>

#include <QColor>
#include <QDebug>

#include "../globals/globals.h"

class Log : public QObject
{
    Q_OBJECT

    public:
        inline static Log* Instance()
        {
            static Log* instance = new Log();
            return instance;
        }

        void AddExcept(const std::string& except);
        void AddMesg(const std::string& mesg);
        void Clear();
        void SendBuffer();
        void Write(std::ofstream& out) const;

    signals:
        void SendMesg(const char*, QColor);
        void OpenGui();

    private:
        Log();
#ifndef Q_OS_WIN
        Log(const Log&) = delete;
        Log& operator=(const Log&) = delete;
#endif
        inline void Add(const std::string& mesg);

        static const int MAXLEN;

        std::vector< std::pair<std::string, QColor> > _buffer;
        mutable std::mutex _mutex;
};

#endif // LOG_H
