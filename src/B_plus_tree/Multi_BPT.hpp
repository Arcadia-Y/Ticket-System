// implementation for Multi-B+ tree
#ifndef MULTI_BPT_HPP
#define MULTI_BPT_HPP

#include "../file/Myfile.hpp"
#include "../STLite/vector.hpp"
#include "../STLite/algorithm.hpp"

namespace sjtu
{

template<typename K, typename V, class Comp_K = std::less<K>, class Comp_V = std::less<V>>
class Multi_BPT
{
public:
    Multi_BPT(const std::string& name): file(name, head)
    {
        head = file.head();
    }
    ~Multi_BPT()
    {
        file.head() = head;
    }

    void find(const K& key, vector<V>& res)
    {
        if (!head) return;
        const Node* tmp;
        long tofind = find_Node(key);
        tmp = file.readonly(tofind);
        const KVpair* begin = lower_bound(tmp->data, tmp->data+tmp->size, key, comp);
        int locat = begin - tmp->data;
        for (int i = locat; i < tmp->size; i++)
        {
            if (tmp->data[i].key == key)
                res.push_back(tmp->data[i].value);
            else return;
        }
        long next = tmp->ptr[1];
        while (next)
        {
            tmp = file.readonly(next);
            for (int i = 0; i < tmp->size; i++)
            {
                if (tmp->data[i].key == key)
                    res.push_back(tmp->data[i].value);
                else if (tmp->data[i].key < key) continue;
                else return;
            }
            next = tmp->ptr[1];
        }
    }

    void insert(const K& key, const V& value)
    {
        if (!head)
        {
            Node tmp;
            head = file.new_space();
            tmp.parent = 0;
            tmp.size = 1;
            tmp.data[0].key = key;
            tmp.data[0].value = value;
            tmp.ptr[0]  = tmp.ptr[1] = 0;
            file.write(head, tmp);
            return;
        }
        insert_leaf(find_Node(key, value), key, value);
    }

    void erase(const K& key, const V& value)
    {
        if (!head) return;
        erase_leaf(find_Node(key, value), key, value);
    }

    void clean()
    {
        file.clean();
        head = 0;
    }

private:
    constexpr static int DEGREE = 4000 / (sizeof(long) + sizeof(K) + sizeof(V));
    struct KVpair
    {
        K key;
        V value;
        KVpair() {}
        KVpair(const K& k, const V& v): key(k), value(v) {}
        friend bool operator==(const KVpair& a, const KVpair& b)
        {
            return a.key == b.key && a.value == b.value;
        }
    };
    struct Node
    {
        int size;
        long parent;
        KVpair data[DEGREE];
        long ptr[DEGREE+1]; // ptr[0] == 0 means leaf, whose ptr[1] points to next leaf 
    };
    struct Comp
    {
        Comp_K comp_k;
        Comp_V comp_v;
        bool operator()(const KVpair& x, const K& key)
        {
            return comp_k(x.key, key);
        }
        bool operator()(const K& key, const KVpair& x)
        {
            return comp_k(key, x.key);
        }
        bool operator()(const KVpair& a, const KVpair& b)
        {
            if (!(a.key == b.key)) return comp_k(a.key, b.key);
            return comp_v(a.value, b.value);
        }
    } comp;
    long head = 0;
    Myfile<Node, long> file;

    long find_Node(const K& key, const V& value)
    {
        long res = head;
        const Node* tmp = file.readonly(head);
        KVpair tofind(key, value);
        while (tmp->ptr[0])
        {
            const KVpair* found = upper_bound(tmp->data, tmp->data+tmp->size, tofind, comp);
            res = tmp->ptr[found - tmp->data];
            tmp = file.readonly(res);
        }
        return res;
    }

    long find_Node(const K& key)
    {
        long res = head;
        const Node* tmp = file.readonly(head);
        while (tmp->ptr[0])
        {
            const KVpair* found = lower_bound(tmp->data, tmp->data+tmp->size, key, comp);
            res = tmp->ptr[found - tmp->data];
            tmp = file.readonly(res);
        }
        return res;
    }

    void insert_leaf(long address, const K& key, const V& value)
    {
        Node& tmp = *file.readwrite(address);
        KVpair toinsert(key, value);
        KVpair* found = lower_bound(tmp.data, tmp.data+tmp.size, toinsert, comp);
        if (found != tmp.data+tmp.size && *found == toinsert) return; // remember to check out_of_bound!
        int locat = found - tmp.data;
        for (int i = tmp.size; i > locat; i--)
            tmp.data[i] = tmp.data[i-1];
        tmp.data[locat] = toinsert;
        tmp.size++;
        if (tmp.size < DEGREE)
            return;
        int carry = DEGREE / 2;
        long new_address = file.new_space();
        Node new_leaf;
        new_leaf.size = tmp.size - carry;
        tmp.size = carry;
        new_leaf.parent = tmp.parent;
        new_leaf.ptr[0] = 0;
        new_leaf.ptr[1] = tmp.ptr[1];
        tmp.ptr[1] = new_address;
        for (int i = 0; i < new_leaf.size; i++)
            new_leaf.data[i] = tmp.data[carry+i];
        file.write(new_address, new_leaf);
        insert_internal(tmp.parent, new_address, tmp.data[carry]);
    }

    void insert_internal(long this_address, long right_address, const KVpair& toinsert)
    {
        if (!this_address)
        {
            Node new_node;
            long new_head = file.new_space();
            new_node.size = 1;
            new_node.parent = 0;
            new_node.data[0] = toinsert;
            new_node.ptr[0] = head;
            new_node.ptr[1] = right_address;
            Node* tmp = file.readwrite(head);
            tmp->parent = new_head;
            tmp = file.readwrite(right_address);
            tmp->parent = new_head;
            head = new_head;
            file.write(new_head, new_node);
            return;
        }
        Node& this_node = *file.readwrite(this_address);
        KVpair* found = lower_bound(this_node.data, this_node.data+this_node.size, toinsert, comp);
        int locat = found - this_node.data;
        if (this_node.size < DEGREE)
        {
            for (int i = this_node.size - 1; i >= locat; i--)
            {
                this_node.data[i+1] = this_node.data[i];
                this_node.ptr[i+2] = this_node.ptr[i+1];
            }
            this_node.data[locat] = toinsert;
            this_node.ptr[locat+1] = right_address;
            this_node.size++;
            return;
        }
        // split
        int carry = DEGREE / 2;
        KVpair tocarry = this_node.data[carry];
        long new_address = file.new_space();
        Node new_node;
        new_node.size = this_node.size - carry - 1;
        this_node.size = carry;
        new_node.parent = this_node.parent;
        for (int i = 0; i < new_node.size; i++)
        {
            new_node.data[i] = this_node.data[carry+1+i];
            new_node.ptr[i] = this_node.ptr[carry+1+i];
        }
        new_node.ptr[new_node.size] = this_node.ptr[DEGREE];
        // insert
        if (locat <= carry)
        {
            for (int i = this_node.size - 1; i >= locat; i--)
            {
                this_node.data[i+1] = this_node.data[i];
                this_node.ptr[i+2] = this_node.ptr[i+1];
            }
            this_node.data[locat] = toinsert;
            this_node.ptr[locat+1] = right_address;
            this_node.size++;
        }
        else 
        {
            locat -= carry + 1;
            for (int i = new_node.size - 1; i >= locat; i--)
            {
                new_node.data[i+1] = new_node.data[i];
                new_node.ptr[i+2] = new_node.ptr[i+1]; 
            }
            new_node.data[locat] = toinsert;
            new_node.ptr[locat+1] = right_address;
            new_node.size++;
        }
        for (int i = 0; i <= new_node.size; i++)
        {
            Node* tmp = file.readwrite(new_node.ptr[i]);
            tmp->parent = new_address;
        }
        file.write(new_address, new_node);
        insert_internal(this_node.parent, new_address, tocarry);
    }

    void erase_leaf(long address, const K& key, const V& value)
    {
        
        KVpair toerase(key, value);
        Node &tmp = *file.readwrite(address);
        KVpair* found = lower_bound(tmp.data, tmp.data+tmp.size, toerase, comp);
        if (!(*found == toerase)) return;
        int locat = found - tmp.data;
        if (locat == tmp.size) return;
        for (int i = locat; i < tmp.size-1; i++)
            tmp.data[i] = tmp.data[i+1];
        tmp.size--;
        if (tmp.size >= DEGREE/2) return;
        erase_leaf_rebalance(address, tmp);
    }

    void erase_leaf_rebalance(long address, Node& this_node)
    {
        if (!this_node.parent)
        {
            if (this_node.size) return;
            file.delete_space(address);
            head = 0;
            return;
        }
        Node &parent_node = *file.readwrite(this_node.parent);
        KVpair* this_key = upper_bound(parent_node.data, parent_node.data+parent_node.size, this_node.data[0], comp) - 1;
        int locat = this_key - parent_node.data;
        // borrow from right sibling
        // ptr[1] IS NOT ALWAYS RIGHT SIBLING!
        long right;
        if (locat == parent_node.size - 1)
            right = 0;
        else
            right = parent_node.ptr[locat+2];
        Node* right_node;
        if (right)
        {
            right_node = file.readwrite(right);
            if (right_node->size > DEGREE / 2)
            {
                this_node.data[this_node.size] = right_node->data[0];
                this_node.size++;
                for (int i = 1; i < right_node->size; i++)
                    right_node->data[i-1] = right_node->data[i];
                right_node->size--;
                *(this_key+1) = right_node->data[0];
                return;
            }
        }
        // borrow from left sibling
        Node* left_node;
        long left;
        if (locat >= 0)
        {
            left = parent_node.ptr[locat];
            left_node = file.readwrite(left);
            if (left_node->size > DEGREE / 2)
            {
                for (int i = this_node.size; i > 0; i--)
                    this_node.data[i] = this_node.data[i-1];
                left_node->size--;
                this_node.data[0] = left_node->data[left_node->size];
                this_node.size++;
                *this_key = this_node.data[0];
                return;
            }
        }
        // merge with siblings
        if (right)
        {
            for (int i = 0; i < right_node->size; i++)
                this_node.data[this_node.size+i] = right_node->data[i];
            this_node.size += right_node->size;
            this_node.ptr[1] = right_node->ptr[1];
            for (int i = locat+1; i < parent_node.size-1; i++)
            {
                parent_node.data[i] = parent_node.data[i+1];
                parent_node.ptr[i+1] = parent_node.ptr[i+2];
            }
            parent_node.size--;
            file.delete_space(right);
        }
        else
        {
            for (int i = 0; i < this_node.size; i++)
                left_node->data[left_node->size+i] = this_node.data[i];
            left_node->size += this_node.size;
            left_node->ptr[1] = this_node.ptr[1];
            for (int i = locat; i < parent_node.size-1; i++)
            {
                parent_node.data[i] = parent_node.data[i+1];
                parent_node.ptr[i+1] = parent_node.ptr[i+2];
            }
            parent_node.size--;
            file.delete_space(address);
        }
        if (parent_node.size >= DEGREE / 2) return;
        erase_internal_rebalance(this_node.parent, parent_node);
    }

    void erase_internal_rebalance(long address, Node& this_node)
    {
        Node* son;
        if (!this_node.parent)
        {
            if (this_node.size)
                return; 
            son = file.readwrite(this_node.ptr[0]);
            son->parent = 0;
            head = this_node.ptr[0];
            file.delete_space(address);
            return;
        }
        Node &parent_node = *file.readwrite(this_node.parent);
        KVpair* this_key = upper_bound(parent_node.data, parent_node.data+parent_node.size, this_node.data[0], comp) - 1;
        int locat = this_key - parent_node.data;
        long right;
        if (locat == parent_node.size - 1)
            right = 0;
        else
            right = parent_node.ptr[locat+2];
        // borrow from right sibling
        Node* right_node;
        if (right)
        {
            right_node = file.readwrite(right);
            if (right_node->size > DEGREE / 2)
            {
                this_node.size++;
                son = file.readwrite(right_node->ptr[0]);
                this_node.ptr[this_node.size] = right_node->ptr[0];
                son->parent = address;
                 this_node.data[this_node.size-1] = *(this_key+1); // NOT this_node.data[this_node.size-1] = son->data[0]!!!
                *(this_key+1) = right_node->data[0]; // THIS LINE SHOULD BE DONE BEFORE MOVING!
                for (int i = 1; i < right_node->size; i++)
                {
                    right_node->data[i-1] = right_node->data[i];
                    right_node->ptr[i-1] = right_node->ptr[i];
                }
                right_node->ptr[right_node->size-1] = right_node->ptr[right_node->size];
                right_node->size--;
                return;
            }
        }
        // borrow from left sibling
        Node* left_node;
        long left;
        if (locat >= 0)
        {
            left = parent_node.ptr[locat];
            left_node = file.readwrite(left);
            if (left_node->size > DEGREE / 2)
            {
                this_node.ptr[this_node.size+1] = this_node.ptr[this_node.size];
                for (int i = this_node.size; i > 0; i--)
                {
                    this_node.data[i] = this_node.data[i-1];
                    this_node.ptr[i] = this_node.ptr[i-1];
                }
                son = file.readwrite(left_node->ptr[left_node->size]);
                this_node.ptr[0] = left_node->ptr[left_node->size];
                son->parent = address;
                this_node.size++;
                left_node->size--;
                this_node.data[0] = *this_key; // NOT this_node.data[0] = son->data[0]!
                *this_key = left_node->data[left_node->size]; // SIGNIFICANT STEP!
                return;
            }
        }
        // merge with siblings
        if (right)
        {
            this_node.ptr[this_node.size+1] = right_node->ptr[0];
            son = file.readwrite(right_node->ptr[0]);
            son->parent = address;
            for (int i = 0; i < right_node->size; i++)
            {
                this_node.data[this_node.size+1+i] = right_node->data[i];
                this_node.ptr[this_node.size+i+2] = right_node->ptr[i+1]; 
                son = file.readwrite(right_node->ptr[i+1]);
                son->parent = address;
            }
            this_node.data[this_node.size] = parent_node.data[locat+1];
            this_node.size += right_node->size + 1;
            for (int i = locat + 1; i < parent_node.size - 1; i++)
            {
                parent_node.data[i] = parent_node.data[i+1];
                parent_node.ptr[i+1] = parent_node.ptr[i+2];
            }
            parent_node.size--;
            file.delete_space(right);
        }
        else
        {
            left_node->ptr[left_node->size+1] = this_node.ptr[0];
            son = file.readwrite(this_node.ptr[0]);
            son->parent = left;
            for (int i = 0; i < this_node.size; i++)
            {
                left_node->data[left_node->size+1+i] = this_node.data[i];
                left_node->ptr[left_node->size+i+2] = this_node.ptr[i+1]; 
                son = file.readwrite(this_node.ptr[i+1]);
                son->parent = left;
            }
            left_node->data[left_node->size] = parent_node.data[locat];
            left_node->size += this_node.size + 1;
            for (int i = locat; i < parent_node.size - 1; i++)
            {
                parent_node.data[i] = parent_node.data[i+1];
                parent_node.ptr[i+1] = parent_node.ptr[i+2];
            }
            parent_node.size--;
            file.delete_space(address);
        }
        if (parent_node.size >= DEGREE / 2) return;
        erase_internal_rebalance(this_node.parent, parent_node);
    }
};

} // namespace sjtu

#endif
