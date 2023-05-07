// a fixed-sized allocator
# ifndef SJTU_ALLOCATOR_HPP
# define SJTU_ALLOCATOR_HPP

#include <iostream>

namespace sjtu
{

template<typename T, int size>
class allocator
{
public:
    allocator()
    {
        space = (T*) malloc(size * sizeof(T));
    }
    ~allocator()
    {
        free(space);
    }

    T* new_space()
    {
        if (pool_cursor == -1)
            return space + data_cursor++;
        int* tmp = reinterpret_cast<int*>(space + pool_cursor);
        pool_cursor = *tmp;
        return  reinterpret_cast<T*>(tmp);
    }

    void delete_space(T* x)
    {
        int* tmp = reinterpret_cast<int*>(x);
        *tmp = pool_cursor;
        pool_cursor = x - space;
    }

    void clean()
    {
        data_cursor = 0;
        pool_cursor = -1;
    }

private:
    T* space;
    int data_cursor = 0;
    int pool_cursor = -1;
};

} // namespace sjtu

#endif
