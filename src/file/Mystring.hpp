# ifndef MYSTRING_HPP
# define MYSTRING_HPP

#include <iostream>
#include <string>

namespace sjtu
{

template <int size>
struct Mystring
{
    char string[size];
    Mystring()
    {}
    Mystring(const std::string &s)
    {
        strcpy(string, s.c_str());
    }
    Mystring(const char *s)
    {
        strcpy(string, s);
    }
    void operator=(Mystring other)
    {
        strcpy(string, other.string);
    }
    friend bool operator<(Mystring a, Mystring b)
    {
        return strcmp(a.string, b.string) < 0;
    }
    friend bool operator==(Mystring a, Mystring b)
    {
        return strcmp(a.string, b.string) == 0;
    }
    friend std::ostream& operator<<(std::ostream& out, Mystring s)
    {
        out << s.string;
        return out;
    }
};

} // namespace sjtu

# endif
