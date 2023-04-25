// implementation for frequently used algorithms
# ifndef SJTU_ALGORITHM_HPP
# define SJTU_ALGORITHM_HPP

namespace sjtu
{

template<typename T1, typename T2, typename Compare>
T1 lower_bound(T1 begin, T1 end, const T2& tofind, Compare comp)
{
    int dist = end - begin;
    while (dist > 3)
    {
        T1 mid = begin + (dist >> 1);
        if (comp(*mid, tofind))
            begin = mid + 1;
        else 
            end = mid + 1;
        dist = end - begin;
    }
    while (begin != end && comp(*begin, tofind)) begin++;
    return begin;
}

template<typename T1, typename T2, typename Compare>
T1 upper_bound(T1 begin, T1 end, const T2& tofind, Compare comp)
{
    int dist = end - begin;
    while (dist > 3)
    {
        T1 mid = begin + (dist >> 1);
        if (comp(tofind, *mid))
            end = mid + 1;
        else 
            begin = mid + 1;
        dist = end - begin;
    }
    while (begin != end && !comp(tofind, *begin)) begin++;
    return begin;
}

} // namespace sjtu

# endif
