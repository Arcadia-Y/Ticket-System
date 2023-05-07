// a class for storing data in file
#ifndef DATAFILE_HPP
#define DATAFILE_HPP

#include "Myfile.hpp"

namespace sjtu
{

template<typename V>
class Datafile
{
public:
    Datafile(const std::string& name): file(name, pos)
    {
        pos = file.head();
        if (!pos)
        {
            Block tmp;
            pos = file.new_space();
            file.write(pos, tmp);
        }
    }
    ~Datafile()
    {
        file.head() = pos;
    }
    
    long new_space()
    {
        Block* tmp = file.readwrite(pos);
        if (tmp->size < MAXSIZE)
            return pos + (tmp->size++) * sizeof(V);
        Block new_block;
        new_block.size++;
        pos = file.new_space();
        file.write(pos, new_block);
        return pos;
    }

    void delete_space(long address)
    {
        long offset = (address - 3 * sizeof(long)) % sizeof(Block);
        long block_address = address - offset;
        Block* block = file.readwrite(block_address);
        if (--block->size) return;
        if (block_address != pos)
            file.delete_space(block_address);
    }

    void write(long address, const V& value)
    {
        long offset = (address - 3 * sizeof(long)) % sizeof(Block);
        Block* block = file.readwrite(address - offset);
        block->data[offset / sizeof(V)] = value;
    }

    const V* readonly(long address)
    {
        long offset = (address - 3 * sizeof(long)) % sizeof(Block);
        const Block* block = file.readonly(address - offset);
        return (block->data + offset / sizeof(V));
    }

    V* readwrite(long address)
    {
        long offset = (address - 3 * sizeof(long)) % sizeof(Block);
        Block* block = file.readwrite(address - offset);
        return (block->data + offset / sizeof(V));
    }

    void clean()
    {
        file.clean();
        pos = 0;
    }

private:
    constexpr static int MAXSIZE = std::max(4000/sizeof(V), 1UL);
    struct Block
    {
        V data[MAXSIZE];
        int size = 0;
    };
    long pos = 0;
    Myfile<Block, long> file;
};

}// namespace sjtu

#endif
