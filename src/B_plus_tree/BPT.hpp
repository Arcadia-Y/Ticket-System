// implementation for B+ tree
#ifndef BPT_HPP
#define BPT_HPP

#include "../file/Myfile.hpp"
#include "../file/Datafile.hpp"
#include "../STLite/algorithm.hpp"

namespace sjtu
{

template<typename K, typename V, class Comp = std::less<K>>
class BPT
{
public:
    BPT(const std::string& name): file(name + "_index", head), data(name + "_data")
    {
        head = file.head();
    }
    ~BPT()
    {
        file.head() = head;
    }

    const V* readonly(const K& key)
    {
        if (!head) return nullptr;
        const Node* tmp;
        long tofind = find_Node(key);
        tmp = file.readonly(tofind);
        const K* found = lower_bound(tmp->key, tmp->key+tmp->size, key, comp);
        int locat = found - tmp->key;
        if (locat == tmp->size || !(*found == key)) return nullptr;
        return data.readonly(tmp->ptr[locat]);
    }

    V* readwrite(const K& key)
    {
        if (!head) return nullptr;
        const Node* tmp;
        long tofind = find_Node(key);
        tmp = file.readonly(tofind);
        const K* found = lower_bound(tmp->key, tmp->key+tmp->size, key, comp);
        int locat = found - tmp->key;
        if (locat == tmp->size || !(*found == key)) return nullptr;
        return data.readwrite(tmp->ptr[locat]);
    }

    void insert(const K& key, const V& value)
    {
        if (!head)
        {
            Node tmp;
            head = file.new_space();
            tmp.parent = 0;
            tmp.isleaf = true;
            tmp.size = 1;
            tmp.key[0] = key;
            tmp.ptr[0] = data.new_space();
            data.write(tmp.ptr[0], value);
            file.write(head, tmp);
            return;
        }
        insert_leaf(find_Node(key), key, value);
    }

    void erase(const K& key)
    {
        if (!head) return;
        erase_leaf(find_Node(key), key);
    }

    bool empty() const
    {
        if (!head) return true;
        return false;
    }

    void clean()
    {
        file.clean();
        data.clean();
        head = 0;
    }

private:
    constexpr static int DEGREE = 4000 / (sizeof(long) + sizeof(K));
    struct Node
    {
        int size;
        bool isleaf;
        long parent;
        K key[DEGREE];
        long ptr[DEGREE+1]; // leaf's ptr[DEGREE] points to next leaf
    };
    long head = 0;
    Comp comp;
    Myfile<Node, long> file;
    Datafile<V> data;

    long find_Node(const K& key)
    {
        long res = head;
        const Node* tmp = file.readonly(head);
        while (!tmp->isleaf)
        {
            const K* found = upper_bound(tmp->key, tmp->key+tmp->size, key, comp);
            res = tmp->ptr[found - tmp->key];
            tmp = file.readonly(res);
        }
        return res;
    }

    void insert_leaf(long address, const K& key, const V& value)
    {
        Node& tmp = *file.readwrite(address);
        K* found = lower_bound(tmp.key, tmp.key+tmp.size, key, comp);
        if (found != tmp.key+tmp.size && *found == key) return; // remember to check out_of_bound!
        int locat = found - tmp.key;
        for (int i = tmp.size; i > locat; i--)
        {
            tmp.key[i] = tmp.key[i-1];
            tmp.ptr[i] = tmp.ptr[i-1];
        }
        tmp.key[locat] = key;
        tmp.ptr[locat] = data.new_space();
        data.write(tmp.ptr[locat], value);
        tmp.size++;
        if (tmp.size < DEGREE)
            return;
        int carry = DEGREE / 2;
        long new_address = file.new_space();
        Node new_leaf;
        new_leaf.isleaf = true;
        new_leaf.size = tmp.size - carry;
        tmp.size = carry;
        new_leaf.parent = tmp.parent;
        new_leaf.ptr[DEGREE] = tmp.ptr[DEGREE];
        tmp.ptr[DEGREE] = new_address;
        for (int i = 0; i < new_leaf.size; i++)
        {
            new_leaf.key[i] = tmp.key[carry+i];
            new_leaf.ptr[i] = tmp.ptr[carry+i];
        }
        file.write(new_address, new_leaf);
        insert_internal(tmp.parent, new_address, tmp.key[carry]);
    }

    void insert_internal(long this_address, long right_address, const K& toinsert)
    {
        if (!this_address)
        {
            Node new_node;
            long new_head = file.new_space();
            new_node.isleaf = false;
            new_node.size = 1;
            new_node.parent = 0;
            new_node.key[0] = toinsert;
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
        K* found = lower_bound(this_node.key, this_node.key+this_node.size, toinsert, comp);
        int locat = found - this_node.key;
        if (this_node.size < DEGREE)
        {
            for (int i = this_node.size - 1; i >= locat; i--)
            {
                this_node.key[i+1] = this_node.key[i];
                this_node.ptr[i+2] = this_node.ptr[i+1];
            }
            this_node.key[locat] = toinsert;
            this_node.ptr[locat+1] = right_address;
            this_node.size++;
            return;
        }
        // split
        int carry = DEGREE / 2;
        K tocarry = this_node.key[carry];
        long new_address = file.new_space();
        Node new_node;
        new_node.isleaf = false;
        new_node.size = this_node.size - carry - 1;
        this_node.size = carry;
        new_node.parent = this_node.parent;
        for (int i = 0; i < new_node.size; i++)
        {
            new_node.key[i] = this_node.key[carry+1+i];
            new_node.ptr[i] = this_node.ptr[carry+1+i];
        }
        new_node.ptr[new_node.size] = this_node.ptr[DEGREE];
        // insert
        if (locat <= carry)
        {
            for (int i = this_node.size - 1; i >= locat; i--)
            {
                this_node.key[i+1] = this_node.key[i];
                this_node.ptr[i+2] = this_node.ptr[i+1];
            }
            this_node.key[locat] = toinsert;
            this_node.ptr[locat+1] = right_address;
            this_node.size++;
        }
        else 
        {
            locat -= carry + 1;
            for (int i = new_node.size - 1; i >= locat; i--)
            {
                new_node.key[i+1] = new_node.key[i];
                new_node.ptr[i+2] = new_node.ptr[i+1]; 
            }
            new_node.key[locat] = toinsert;
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

    void erase_leaf(long address, const K& key)
    {
        
        Node &tmp = *file.readwrite(address);
        K* found = lower_bound(tmp.key, tmp.key+tmp.size, key, comp);
        if (!(*found == key)) return;
        int locat = found - tmp.key;
        if (locat == tmp.size) return;
        data.delete_space(tmp.ptr[locat]);
        for (int i = locat; i < tmp.size-1; i++)
        {
            tmp.key[i] = tmp.key[i+1];
            tmp.ptr[i] = tmp.ptr[i+1];
        }
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
        K* this_key = upper_bound(parent_node.key, parent_node.key+parent_node.size, this_node.key[0], comp) - 1;
        int locat = this_key - parent_node.key;
        // borrow from right sibling
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
                this_node.key[this_node.size] = right_node->key[0];
                this_node.ptr[this_node.size] = right_node->ptr[0];
                this_node.size++;
                for (int i = 1; i < right_node->size; i++)
                {
                    right_node->key[i-1] = right_node->key[i];
                    right_node->ptr[i-1] = right_node->ptr[i];
                }
                right_node->size--;
                *(this_key+1) = right_node->key[0];
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
                {
                    this_node.key[i] = this_node.key[i-1];
                    this_node.ptr[i] = this_node.ptr[i-1];
                }
                left_node->size--;
                this_node.key[0] = left_node->key[left_node->size];
                this_node.ptr[0] = left_node->ptr[left_node->size];
                this_node.size++;
                *this_key = this_node.key[0];
                return;
            }
        }
        // merge with siblings
        if (right)
        {
            for (int i = 0; i < right_node->size; i++)
            {
                this_node.key[this_node.size+i] = right_node->key[i];
                this_node.ptr[this_node.size+i] = right_node->ptr[i];
            }
            this_node.size += right_node->size;
            this_node.ptr[DEGREE] = right_node->ptr[DEGREE];
            for (int i = locat+1; i < parent_node.size-1; i++)
            {
                parent_node.key[i] = parent_node.key[i+1];
                parent_node.ptr[i+1] = parent_node.ptr[i+2];
            }
            parent_node.size--;
            file.delete_space(right);
        }
        else
        {
            for (int i = 0; i < this_node.size; i++)
            {
                left_node->key[left_node->size+i] = this_node.key[i];
                left_node->ptr[left_node->size+i] = this_node.ptr[i];
            }
            left_node->size += this_node.size;
            left_node->ptr[DEGREE] = this_node.ptr[DEGREE];
            for (int i = locat; i < parent_node.size-1; i++)
            {
                parent_node.key[i] = parent_node.key[i+1];
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
        K* this_key = upper_bound(parent_node.key, parent_node.key+parent_node.size, this_node.key[0], comp) - 1;
        int locat = this_key - parent_node.key;
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
                 this_node.key[this_node.size-1] = *(this_key+1); // NOT this_node.key[this_node.size-1] = son->key[0]!!!
                *(this_key+1) = right_node->key[0]; // THIS LINE SHOULD BE DONE BEFORE MOVING!
                for (int i = 1; i < right_node->size; i++)
                {
                    right_node->key[i-1] = right_node->key[i];
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
                    this_node.key[i] = this_node.key[i-1];
                    this_node.ptr[i] = this_node.ptr[i-1];
                }
                son = file.readwrite(left_node->ptr[left_node->size]);
                this_node.ptr[0] = left_node->ptr[left_node->size];
                son->parent = address;
                this_node.size++;
                left_node->size--;
                this_node.key[0] = *this_key; // NOT this_node.key[0] = son->key[0]!
                *this_key = left_node->key[left_node->size]; // SIGNIFICANT STEP!
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
                this_node.key[this_node.size+1+i] = right_node->key[i];
                this_node.ptr[this_node.size+i+2] = right_node->ptr[i+1]; 
                son = file.readwrite(right_node->ptr[i+1]);
                son->parent = address;
            }
            this_node.key[this_node.size] = parent_node.key[locat+1];
            this_node.size += right_node->size + 1;
            for (int i = locat + 1; i < parent_node.size - 1; i++)
            {
                parent_node.key[i] = parent_node.key[i+1];
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
                left_node->key[left_node->size+1+i] = this_node.key[i];
                left_node->ptr[left_node->size+i+2] = this_node.ptr[i+1]; 
                son = file.readwrite(this_node.ptr[i+1]);
                son->parent = left;
            }
            left_node->key[left_node->size] = parent_node.key[locat];
            left_node->size += this_node.size + 1;
            for (int i = locat; i < parent_node.size - 1; i++)
            {
                parent_node.key[i] = parent_node.key[i+1];
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
