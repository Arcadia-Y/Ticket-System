// a class to handle date and time
#ifndef DATE_HPP
#define DATE_HPP

#include <iostream>
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

    inline void operator++()
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

    inline void operator+=(int x)
    {
        d += x;
        if (d < 31) return;
        if (d == 31)
        {
            if (m == 6)
            {
                ++m;
                d -= 30;
            }
            return;
        }
        ++m;
        d -= 31;
    }

    inline Date operator+(int x) const
    {
        Date res = *this;
        res += x;
        return res;
    }

    inline void operator--()
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

    inline void operator-=(int x)
    {
        if (d > x)
        {
            d -= x;
            return;
        }
        --m;
        d += 31;
        if (m == 6)  --d;
        d -= x;
    }

    inline Date operator-(int x) const
    {
        Date res = *this;
        res -= x;
        return res;
    }

    inline friend bool operator<(Date a, Date b)
    {
        if (a.m != b.m) return a.m < b.m;
        return a.d < b.d;
    }

    inline friend bool operator==(Date a, Date b)
    {
        return a.m == b.m && a.d == b.d;
    }

    friend std::ostream& operator<<(std::ostream& out, Date x)
    {
        out << x.m / 10 << x.m % 10 << '-' << x.d / 10 << x.d % 10;
        return out;
    }

    friend int operator-(Date a, Date b)
    {
        int res = 0;
        while (a.m > b.m)
        {
            res += a.d;
            a.d = 1;
            --a;
        }
        return res + a.d - b.d;
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
        res.h = h;
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

    int operator-(Time other) const
    {
        return (h - other.h) * 60 + m - other.m;
    }

    friend std::ostream& operator<<(std::ostream& out, Time x)
    {
        out << x.h / 10 << x.h % 10 << ':' << x.m / 10 << x.m % 10;
        return out;
    }

    friend bool operator<(Time a, Time b)
    {
        if (a.h != b.h) return a.h < b.h;
        return a.m < b.m;
    }

};

void adjust_date(Date& d, Time& t)
{
    while (t.h >= 24)
    {
        ++d;
        t.h -= 24;
    }
}

// (d1, t1) should be earlier than (d2, t2)
int time_between(Date d1, Time t1, Date d2, Time t2)
{
    if (t2 < t1)
    {
        t2.h += 24;
        --d2;
    }
    return (d2 - d1) * 1440 + (t2 - t1);
}

} // namespace sjtu

#endif
