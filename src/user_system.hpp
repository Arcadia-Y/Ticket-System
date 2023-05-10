// a subsystem to process user data
#ifndef USER_SYSTEM_HPP
#define USER_SYSTEM_HPP

#include "STLite/map.hpp"
#include "B_plus_tree/BPT.hpp"
#include "file/Mystring.hpp"

namespace sjtu
{

struct User_Data
{
    char priv;
    char password[31];
    char name[16];
    char mail[31];
};

class User_System
{
public:
    User_System(): userdb("user") {}
    ~User_System() = default;

    bool empty() const
    {
        return userdb.empty();
    }

    int add_first_user(const std::string& username, const User_Data& data)
    {
        userdb.insert(username, data);
        return 0;
    }

    int add_user(const std::string& cur_user, const std::string& new_user, const User_Data& data)
    {
        auto found = user_list.find(cur_user);
        if (found == user_list.end() || !found->second)
            return -1;
        auto ptr = userdb.readonly(cur_user);
        if (ptr->priv <= data.priv) return -1;
        ptr = userdb.readonly(new_user);
        if (ptr != nullptr) return -1;
        userdb.insert(new_user, data);
        return 0;
    }

    int login(const std::string& user, const std::string& passwd)
    {
        auto info = userdb.readonly(user);
        if (info == nullptr) return -1;
        auto found = user_list.find(user);
        if (found != user_list.end() && found->second) return -1;
        if (info->password != passwd) return -1;
        user_list[user] = true;
        return 0;
    }

    int logout(const std::string& user)
    {
        auto found = user_list.find(user);
        if (found == user_list.end() || !found->second) return -1;
        found->second = false;
        return 0;
    }

    void query_profile(const std::string& c, const std::string& u)
    {
        auto found = user_list.find(c);
        if (found == user_list.end() || !found->second)
        {
            std::cout << "-1\n";
            return;
        }
        auto info = userdb.readonly(c);
        char c_priv = info->priv;
        info = userdb.readonly(u);
        if (c != u && (info == nullptr || c_priv <= info->priv))
        {
            std::cout << "-1\n";
            return;
        }
        std::cout << u << ' ' << info->name << ' ' << info->mail << ' ' << (short)info->priv << '\n';
    }

    User_Data* get_profile(const std::string& c, const std::string& u, char& c_priv)
    {
        auto found = user_list.find(c);
        if (found == user_list.end() || !found->second)
            return nullptr;
        auto info = userdb.readonly(c);
        c_priv = info->priv;
        info = userdb.readonly(u);
        if (c != u && (info == nullptr || c_priv <= info->priv))
            return nullptr;
        auto res = userdb.readwrite(u);
        return res;
    }

    bool check_login(const std::string& u)
    {
        auto found = user_list.find(u);
        if (found == user_list.end() || !found->second) return false;
        return true;
    }

    void clean()
    {
        userdb.clean();
        user_list.clear();
    }

private:
    BPT<Mystring<21>, User_Data> userdb;
    map<std::string, bool> user_list;
    
};

} // namespace sjtu

#endif
