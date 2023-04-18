# ifndef MYSTRING_HPP
# define MYSTRING_HPP

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
    Mystring(char *s)
    {
        strcpy(string, s);
    }
    friend bool operator<(const Mystring &a, const Mystring &b)
    {
        return strcmp(a.string, b.string) < 0;
    }
    friend bool operator==(const Mystring &a, const Mystring &b)
    {
        return strcmp(a.string, b.string) == 0;
    }
};

} // namespace sjtu

# endif
