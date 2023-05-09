// a class for easy file I/O with LRU cache

#ifndef MYFILE_HPP
#define MYFILE_HPP

#include <fstream>
#include <cstring>
#include "../STLite/allocator.hpp"

#define MAX_CACHE 300
#define HASH_SIZE 631

namespace sjtu
{

template<typename T, typename Header>
class Basefile
{
public:
    Basefile(const std::string& _name, const Header& _header)
    {
        name = _name;
        header = _header;
        data.open(name+".db");
        if (data.good())
        {
            data.seekg(0);
            data.read(reinterpret_cast <char *> (&data_cursor), sizeof(long));
            data.read(reinterpret_cast <char *> (&pool_cursor), sizeof(long));
            data.read(reinterpret_cast <char *> (&header), sizeof(Header));
        }
        else
        {
            data.open(name+".db", std::ios::out);
            data.close();
            data.open(name+".db");
            data.write(reinterpret_cast <char *> (&data_cursor), sizeof(long));
            data.write(reinterpret_cast <char *> (&pool_cursor), sizeof(long));
            data.write(reinterpret_cast <char *> (&header), sizeof(Header));
        }
    }

    ~Basefile()
    {
        data.seekp(0);
        data.write(reinterpret_cast <char *> (&data_cursor), sizeof(long));
        data.write(reinterpret_cast <char *> (&pool_cursor), sizeof(long));
        data.write(reinterpret_cast <char *> (&header), sizeof(Header));
        data.close();
    }

    long new_space()
    {
        long address;
        if (!pool_cursor)
        {
            address = data_cursor;
            data_cursor += sizeof(T);
            return address;
        }
        address = pool_cursor;
        data.seekg(pool_cursor);
        data.read(reinterpret_cast <char *> (&pool_cursor), sizeof(long));
        return address;
    }

    void delete_space(long address)
    {
        if (address == data_cursor - sizeof(T))
        {
            data_cursor = address;
            return;
        }
        std::swap(pool_cursor, address);
        data.seekp(pool_cursor);
        data.write(reinterpret_cast <char *> (&address), sizeof(long));
    }

    inline void read(long address, T& value)
    {
        data.seekg(address);
        data.read(reinterpret_cast <char *> (&value), sizeof(T));
    }

    inline void write(long address, const T& value)
    {
        data.seekp(address);
        data.write(reinterpret_cast <const char *> (&value), sizeof(T));
    }

    inline Header& head()
    {
        return header;
    }

    void clean()
    {
        data_cursor = 2*sizeof(long) + sizeof(Header);
        pool_cursor = 0;
        data.close();
        data.open(name+".db", std::ios::out);
        data.close();
        data.open(name+".db");
    }

private:
    std::fstream data;
    long data_cursor = 2*sizeof(long) + sizeof(Header);
    long pool_cursor = 0;
    Header header;
    std::string name; 
};

template<typename T>
class Cache_List
{
public:
    struct Cache_Node
    {
        Cache_Node* pre;
        Cache_Node* next;
        long address;
        bool dirty;
        T data;
        Cache_Node(long a, const T& v, bool w, Cache_Node* p = nullptr, Cache_Node* n = nullptr):
        address(a), data(v), dirty(w), pre(p), next(n) {}
        Cache_Node() {}
    };

    Cache_List()
    {
        head = memory.new_space();
        end = memory.new_space();
        head->pre = end->next = nullptr;
        end->pre = head;
        head->next = end;
    }
    
    ~Cache_List() = default;

    Cache_Node* push_front(long address, const T& data, bool write)
    {
        Cache_Node* tmp = memory.new_space();
        tmp->address = address;
        tmp->data = data;
        tmp->dirty = write;
        tmp->pre = head;
        tmp->next = head->next;
        head->next = tmp;
        tmp->next->pre = tmp;
        Size++;
        return tmp;
    }

    void pop_back()
    {
        if (!Size) return;
        Cache_Node* topop = end->pre;
        end->pre = topop->pre;
        topop->pre->next = end;
        memory.delete_space(topop);
        Size--;
    }

    void erase(Cache_Node* toerase)
    {
        toerase->pre->next = toerase->next;
        toerase->next->pre = toerase->pre;
        memory.delete_space(toerase);
        Size--;
    }

    int size() const
    {
        return Size;
    }

    bool empty() const
    {
        return Size == 0;
    }

    void adjust_to_front(Cache_Node* p)
    {
        if (p->pre == head) return;
        p->pre->next = p->next;
        p->next->pre = p->pre;
        head->next->pre = p;
        p->next = head->next;
        p->pre = head;
        head->next = p;
    }

    Cache_Node* front()
    {
        return head->next;
    }

    Cache_Node* back()
    {
        return end->pre;
    }

    void clean()
    {
        memory.clean();
        head = memory.new_space();
        end = memory.new_space();
        head->pre = end->next = nullptr;
        end->pre = head;
        head->next = end;
    }

private:
    int Size = 0;
    Cache_Node* head;
    Cache_Node* end;
    allocator<Cache_Node, MAX_CACHE+5> memory;
};

class Hashmap
{
public:
    Hashmap() = default;
    ~Hashmap() = default;
    
    long find(long key)
    {
        Node* res = array[key % HASH_SIZE].next;
        while (res != nullptr)
        {
            if (res->key == key)
                return res->data;
            res = res->next;
        }
        return -1;
    }

    void insert(long key, long data)
    {
        Node* pre = &array[key % HASH_SIZE];
        while (pre->next != nullptr)
            pre = pre->next;
        pre->next = memory.new_space();
        pre->next->key = key;
        pre->next->data = data;
        pre->next->next = nullptr;
    }

    void erase(long key)
    {
        Node* pre = &array[key % HASH_SIZE];
        Node* toerase = pre->next;
        while (toerase != nullptr)
        {
            if (toerase->key == key)
            {
                pre->next = toerase->next;
                memory.delete_space(toerase);
                return;
            }
            pre = toerase;
            toerase = toerase->next;
        }
    }

    void clean()
    {
        memory.clean();
        for (int i = 0; i < HASH_SIZE; i++)
            array[i].next = nullptr;
    }

private:
    struct Node
    {
        long key;
        long data;
        Node* next;
        Node(): next(nullptr) {}
        Node(long k, long d): key(k), data(d), next(nullptr) {}
    };
    Node array[HASH_SIZE];
    allocator<Node, MAX_CACHE+5> memory;
};

template<typename T, typename Header>
class Myfile
{
public:
    Myfile(const std::string& name, const Header& _header): file(name, _header) {}
    ~Myfile()
    {
        auto tmp = list.front();
        while (tmp->next != nullptr)
        {
            if (tmp->dirty)
                file.write(tmp->address, tmp->data);
            tmp = tmp->next;
        }
    }

    inline Header& head()
    {
        return file.head();
    }

    const T* readonly(long address)
    {
        long found = node_map.find(address);
        if (found != -1)
        {
            auto tmp = reinterpret_cast<typename Cache_List<T>::Cache_Node*> (found);
            list.adjust_to_front(tmp);
            return &(tmp->data);
        }
        T value;
        file.read(address, value);
        auto ptr = list.push_front(address, value, false);
        node_map.insert(address, reinterpret_cast<long>(ptr));
        if (list.size() > MAX_CACHE) oversize();
        return &(ptr->data);
    }

    T* readwrite(long address)
    {
        long found = node_map.find(address);
        if (found != -1)
        {
            auto tmp = reinterpret_cast<typename Cache_List<T>::Cache_Node*> (found);
            list.adjust_to_front(tmp);
            tmp->dirty = true;
            return &(tmp->data);
        }
        T value;
        file.read(address, value);
        auto ptr = list.push_front(address, value, true);
        node_map.insert(address, reinterpret_cast<long>(ptr));
        if (list.size() > MAX_CACHE) oversize();
        return &(ptr->data);
    }

    void write(long address, const T& value)
    {
        node_map.insert(address, reinterpret_cast<long> (list.push_front(address, value, true)));
        if (list.size() > MAX_CACHE) oversize();
    }

    long new_space()
    {
        return file.new_space();
    }

    void delete_space(long address)
    {
        file.delete_space(address);
        long found = node_map.find(address);
        if (found != -1)
        {
             auto tmp = reinterpret_cast<typename Cache_List<T>::Cache_Node*> (found);
             list.erase(tmp);
             node_map.erase(address);
        }
    }

    void clean()
    {
        file.clean();
        list.clean();
        node_map.clean();
    }

private:
    Basefile<T, Header> file;
    Cache_List<T> list;
    Hashmap node_map;

    void oversize()
    {
        auto tmp = list.back();
        if (tmp->dirty)
            file.write(tmp->address, tmp->data);
        node_map.erase(tmp->address);
        list.pop_back();
    }
};

} // namespace sjtu

#endif
