// implementation for frequently used algorithms
# ifndef SJTU_ALGORITHM_HPP
# define SJTU_ALGORITHM_HPP

#include <iostream>

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

template<typename T, typename Compare>
void sort(T* begin, T* end, Compare comp)
{
    T* space = new T[end - begin + 1];
    innersort(begin, end, comp, space);
    delete []space;
}

template<typename T, typename Compare>
void innersort(T* first, T* last, Compare comp, T* tmp)
{
    int dist = last - first;
    if (dist <= 1) return;
	if (dist == 2)
	{
		if (comp(*(last-1), *first))
			std::swap(first[0], first[1]);
		return;
	}
	T* mid = first + dist/2;
	innersort(first, mid, comp, tmp);
	innersort(mid, last, comp, tmp);
	T* ptr1 = first;
	T* ptr2 = mid;
	int count = 0;
	while (ptr1 != mid && ptr2 != last)
	{
		if (comp(*ptr2, *ptr1))
		{
			tmp[count] = *ptr2;
			ptr2++;
			
		}
		else
		{
			tmp[count] = *ptr1;
			ptr1++;
		}
		count++;
	}
	while (ptr1 != mid)
	{
		tmp[count] = *ptr1;
		ptr1++;
		count++;
	}
	while (ptr2 != last)
	{
		tmp[count] = *ptr2;
		ptr2++;
		count++;
	}
	for (int i = 0; i < count; i++)
		first[i] = tmp[i];
}

} // namespace sjtu

# endif
