// a class to handle dates
#ifndef DATE_HPP
#define DATE_HPP

#include <iostream>
#include <ostream>
#include <string>

namespace sjtu
{

struct Date
{
    char m;
    char d;

    Date() {}
    Date(const std::string& s)
    {
        m = 10 * (s[0] - '0') + s[1] - '0';
        d = 10 * (s[3] - '0') + s[4] - '0';
    }

    void operator++()
    {
        if (++d < 31) return;
        if (d == 31)
        {
            if (m == 6)
            {
                ++m;
                d = 1;
                return;
            }
            return;
        } 
        ++m;
        d = 1;
    }

    void operator+=(int x)
    {
        while (x--) this->operator++();
    }

    void operator--()
    {
        if (--d > 0) return;
        --m;
        if (m == 6)
        {
            d = 30;
            return;
        }
        d = 31;
    }

    void operator-=(int x)
    {
        while (x--) this->operator--();
    }

    Date operator-(int x)
    {
        Date res = *this;
        res -= x;
        return res;
    }

    friend bool operator<(Date a, Date b)
    {
        if (a.m != b.m) return a.m < b.m;
        return a.d < b.d;
    }

    friend std::ostream& operator<<(std::ostream& out, Date x)
    {
        out << x.m / 10 << x.m % 10 << '-' << x.d / 10 << x.d % 10;
        return out;
    }
};

struct Time
{   
    char h;
    char m;

    Time() {}
    Time(const std::string& s)
    {
        h = 10 * (s[0] - '0') + s[1] - '0';
        m = 10 * (s[3] - '0') + s[4] - '0';
    }

    Time operator+(int x)
    {
        Time res;
        x += m;
        res.h += x / 60;
        res.m = x % 60;
        return res;
    }

    void operator+=(int x)
    {
        x += m;
        h += x / 60;
        m = x % 60;
    }

    int operator-(Time other)
    {
        return (h - other.h) * 60 + m - other.m;
    }

    friend std::ostream& operator<<(std::ostream& out, Time x)
    {
        out << x.h / 10 << x.h % 10 << ':' << x.m / 10 << x.m % 10;
        return out;
    }

};

void adjust_date(Date& d, Time& t)
{
    while (t.h >= 24)
    {
        ++d;
        --t.h;
    }
}

}

#endif
