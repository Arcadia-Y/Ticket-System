// a parser to handle input commands
#ifndef PARSER_HPP
#define PARSER_HPP

#include "STLite/vector.hpp"
#include "file/Mystring.hpp"
#include "train_system.hpp"
#include "user_system.hpp"
#include <cstring>
#include <iostream>
#include <string>

namespace sjtu
{

class Parser
{
public:
    // parse a line
    void parseline(const std::string &line)
    {
        tokens.clear();
        std::string blank;
        tokens.push_back(blank);
        auto it = &tokens[0];
        for (int i = 0; i < line.size(); i++)
        {
            if (line[i] == ' ')
            {
                if (!it->empty())
                {
                    tokens.push_back(blank);
                    it = &tokens[tokens.size()-1];
                }
            }
            else
                *it += line[i];  
        }
        if (it->empty()) tokens.pop_back();
    }

    // execute the parsed line
    void execute()
    {
        std::cout << tokens[0] << ' ';
        if (tokens[1] == "query_profile")
        {
            std::string c, u;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-c")
                    c = tokens[i+1];
                else
                    u = tokens[i+1];
            }
            user_system.query_profile(c, u);
        }
        else if (tokens[1] == "query_ticket")
        {
            Mystring<31> s, t;
            Date d;
            bool p = 0;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-s")
                    s = tokens[i+1];
                else if (tokens[i] == "-t")
                    t = tokens[i+1];
                else if (tokens[i] == "-d")
                    d = tokens[i+1];
                else if (tokens[i+1] == "cost")
                    p = 1;
            }
            train_system.query_ticket(s, t, d, p);
        }
        else if (tokens[1] == "buy_ticket")
        {
            std::string u;
            Mystring<21> id;
            Date d;
            Mystring<31> f, t;
            int n;
            bool q = false;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-u")
                    u = tokens[i+1];
                else if (tokens[i] == "-i")
                    id = tokens[i+1];
                else if (tokens[i] == "-d")
                    d = tokens[i+1];
                else if (tokens[i] == "-f")
                    f = tokens[i+1];
                else if (tokens[i] == "-t")
                    t = tokens[i+1];
                else if (tokens[i] == "-n")
                    n = std::stoi(tokens[i+1]);
                else if (tokens[i+1] == "true")
                    q = true;
            }
            if (!user_system.check_login(u))
            {
                std::cout << "-1\n";
                return;
            }
            train_system.buy_ticket(u, id, d, f, t, n, q);
        }
        else if (tokens[1] == "login")
        {
            std::string u, p;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-u")
                    u = tokens[i+1];
                else
                    p = tokens[i+1];
            }
            std::cout << user_system.login(u, p) << '\n';
        }
        else if (tokens[1] == "logout")
        {
            std::cout << user_system.logout(tokens[3]) << '\n';
        }
        else if (tokens[1] == "modify_profile")
        {
            std::string c, u;
            // get c, u
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-u")
                    u = tokens[i+1];
                else if (tokens[i] == "-c")
                    c = tokens[i+1];
            }
            char c_priv;
            auto profile = user_system.get_profile(c, u, c_priv);
            if (profile == nullptr)
            {
                std::cout << "-1\n";
                return;
            }
            // check -g
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-g")
                {
                    char g = std::stoi(tokens[i+1]);
                    if (g >= c_priv)
                    {
                        std::cout << "-1\n";
                        return;
                    }
                    profile->priv = g;
                }
            }
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-p")
                    strcpy(profile->password, tokens[i+1].c_str());
                else if (tokens[i] == "-n")
                    strcpy(profile->name, tokens[i+1].c_str());
                else if (tokens[i] == "-m")
                    strcpy(profile->mail, tokens[i+1].c_str());
            }
            std::cout << u << ' ' << profile->name << ' ' << profile->mail << ' ' << (int)profile->priv << '\n'; 
        }
        else if (tokens[1] == "query_order")
        {
            if (!user_system.check_login(tokens[3]))
            {
                std::cout << "-1\n";
                return;
            }
            train_system.query_order(tokens[3]);
        }
        else if (tokens[1] == "add_user")
        {
            std::string c, u;
            User_Data data;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-c")
                    c = tokens[i+1];
                else if (tokens[i] == "-u")
                    u = tokens[i+1];
                else if (tokens[i] == "-p")
                    strcpy(data.password, tokens[i+1].c_str());
                else if (tokens[i] == "-n")
                    strcpy(data.name, tokens[i+1].c_str());
                else if (tokens[i] == "-m")
                    strcpy(data.mail, tokens[i+1].c_str());
                else
                    data.priv = std::stoi(tokens[i+1]);
            }
            if (user_system.empty())
            {
                data.priv = 10;
                std::cout << user_system.add_first_user(u, data) << '\n';
            }
            else
                std::cout << user_system.add_user(c, u, data) << '\n';
        }
        else if (tokens[1] == "add_train")
        {
            Mystring<21> id;
            char n, y;
            int m;
            Time x;
            vector<std::string> s, p, t, o, d;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-i")
                    id = tokens[i+1];
                else if (tokens[i] == "-n")
                    n = std::stoi(tokens[i+1]);
                else if (tokens[i] == "-m")
                    m = std::stoi(tokens[i+1]);
                else if (tokens[i] == "-s")
                    parse_exp(tokens[i+1], s);
                else if (tokens[i] == "-p")
                    parse_exp(tokens[i+1], p);
                else if (tokens[i] == "-x")
                    x = tokens[i+1];
                else if (tokens[i] == "-t")
                    parse_exp(tokens[i+1], t);
                else if (tokens[i] == "-o")
                    parse_exp(tokens[i+1], o);
                else if (tokens[i] == "-d")
                    parse_exp(tokens[i+1], d);
                else
                    y = tokens[i+1][0];
            }
            if (train_system.is_id_exist(id))
            {
                std::cout << "-1\n";
                return;
            }
            Train_Data data;
            data.released = false;
            data.station_num = n;
            data.start_date = d[0];
            data.end_date = d[1];
            data.type = y;
            strcpy(data.stations[0], s[0].c_str());
            data.seat = m;
            data.leave_time[0] = x;
            data.price[0] = 0;
            for (char i = 1; i < n-1; i++)
            {
                strcpy(data.stations[i], s[i].c_str());
                data.arrive_time[i-1] = data.leave_time[i-1] + std::stoi(t[i-1]);
                data.leave_time[i] = data.arrive_time[i-1] + std::stoi(o[i-1]);
                data.price[i] = data.price[i-1] + std::stoi(p[i-1]);
            }
            strcpy(data.stations[n-1], s[n-1].c_str());
            data.arrive_time[n-2] = data.leave_time[n-2] + std::stoi(t[n-2]);
            data.price[n-1] = data.price[n-2] + std::stoi(p[n-2]);
            train_system.add_train(id, data);
        }
        else if (tokens[1] == "delete_train")
        {
            std::cout << train_system.delete_train(tokens[3]) << '\n'; 
        }
        else if (tokens[1] == "release_train")
        {
            std::cout << train_system.release_train(tokens[3]) << '\n';
        }
        else if (tokens[1] == "query_train")
        {
            Mystring<21> id;
            Date d;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-i")
                    id = tokens[i+1];
                else
                    d = tokens[i+1];
            }
            train_system.query_train(id, d);
        }
        else if (tokens[1] == "query_transfer")
        {
            Mystring<31> s, t;
            Date d;
            bool p = 0;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-s")
                    s = tokens[i+1];
                else if (tokens[i] == "-t")
                    t = tokens[i+1];
                else if (tokens[i] == "-d")
                    d = tokens[i+1];
                else if (tokens[i+1] == "cost")
                    p = 1;
            }
            train_system.query_transfer(s, t, d, p);
        }
        else if (tokens[1] == "refund_ticket")
        {
            Mystring<21> u;
            int n = 1;
            for (int i = 2; i < tokens.size(); i += 2)
            {
                if (tokens[i] == "-u")
                    u = tokens[i+1];
                else
                    n = std::stoi(tokens[i+1]);
            }
            if (user_system.check_login(u.string))
                std::cout << train_system.refund_ticket(u, n-1) << '\n';
            else
                std::cout << "-1\n";
        }
        else if (tokens[1] == "clean")
        {
            train_system.clean();
            user_system.clean();
            std::cout << "0\n";
        }
        else if (tokens[1] == "exit")
        {
            std::cout << "bye\n";
            exit(0);
        }
    }

private:
    User_System user_system;
    Train_System train_system;
    vector<std::string> tokens;

    void parse_exp(const std::string& s, vector<std::string>& res)
    {
        std::string blank;
        res.push_back(blank);
        auto it = &res[0];
        for (int i = 0; i < s.size(); i++)
        {
            if (s[i] == '|')
            {
                res.push_back(blank);
                it = &res[res.size()-1];
            }
            else
                *it += s[i];
        }
        if (it->empty()) res.pop_back();
    }
};

} // namespace sjtu

#endif
