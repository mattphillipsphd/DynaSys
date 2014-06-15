#include "notes.h"

Notes::Notes()
{
}

void Notes::Read(std::istream& in)
{
    _text.clear();
    std::string line;
    while (line.empty() && !in.eof())
        std::getline(in, line);
    _text += line + "\n";
    if (in.eof()) return;
    while (!in.eof())
    {
        std::getline(in, line);
        _text += line + "\n";
    }
}

void Notes::Write(std::ostream& out) const
{
    out << _text;
}
