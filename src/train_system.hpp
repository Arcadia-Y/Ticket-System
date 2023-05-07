// a subsystem to process train and order data
#ifndef TRAIN_SYSTEM_HPP
#define TRAIN_SYSTEM_HPP

#include <iostream>
#include "B_plus_tree/BPT.hpp"
#include "STLite/algorithm.hpp"
#include "STLite/vector.hpp"
#include "STLite/map.hpp"
#include "file/Mystring.hpp"
#include "B_plus_tree/Multi_BPT.hpp"
#include "date.hpp"

#define MAXSTA 100

namespace sjtu
{

struct Train_Data
{
    bool released;
    char station_num;
    char type;
    Date start_date;
    Date end_date;
    Time leave_time[MAXSTA-1]; // no leave_time for last station
    Time arrive_time[MAXSTA-1]; // no arrive_time for first station
    char stations[MAXSTA][31];
    int seat[MAXSTA-1]; // no seat for last station
    int price[MAXSTA]; // the price from first station
};

struct Order_Data
{
    signed char state;// -1 for refunded, 0 for pending, 1 for success
    char train_id[21];
    char from[31];
    char to[31];
    Date leave_date;
    Time leave_time;
    Date arrive_date;
    Time arrive_time;
    int price;
    int num;
};

struct Journey_Data
{
    char train_id[21];
    Date leave_date;
    Time leave_time;
    Date arrive_date;
    Time arrive_time;
    int price;
    int seat;
    int time;
};

class Train_System
{
public:
    Train_System(): train_db("train"), train_index("train_index"), order_db("order"),
    order_index("order_index"), order_queue("order_queue") {}
    ~Train_System() = default;

    bool is_id_exist(const Mystring<21>& id)
    {
        auto found = train_db.readonly(id);
        if (found != nullptr) return true;
        return false;
    }

    void add_train(const Mystring<21>& id, const Train_Data& data)
    {
        train_db.insert(id, data);
        std::cout << "0\n";
    }

    int delete_train(const Mystring<21>& id)
    {
        auto found = train_db.readonly(id);
        if (found == nullptr || found->released) return -1;
        train_db.erase(id);
        return 0;
    }

    int release_train(const Mystring<21>& id)
    {
        auto found = train_db.readwrite(id);
        if (found == nullptr || found->released) return -1;
        found->released = true;
        Index_Info info;
        info.train_id = id;
        for (char i = 0; i < found->station_num; i++)
        {
            info.num = i;
            train_index.insert(found->stations[i], info);
        }
        return 0;
    }

    void query_train(const Mystring<21>& id, Date d)
    {
        auto found = train_db.readonly(id);
        if (found == nullptr)
        {
            std::cout << "-1\n";
            return;
        }
        if (d < found->start_date || found->end_date < d)
        {
            std::cout << "-1\n";
            return;
        }
        std::cout << id << ' ' << found->type << '\n';
        std::cout << found->stations[0] << " xx-xx xx:xx -> " << d << ' ' << 
        found->leave_time << " 0 " << found->seat[0] << '\n';
        Date day;
        Time t;
        for (char i = 1; i < found->station_num - 1; i++)
        {
            day = d;
            t = found->arrive_time[i-1];
            adjust_date(day, t);
            std::cout << found->stations[i] << ' ' << day << ' ' << t << " -> ";
            day = d;
            t = found->leave_time[i];
            adjust_date(d, t);
            std::cout << day << ' ' << t << ' ' << found->price[i] << ' ' << found->seat[i] << '\n';
        }
        day = d;
        t = found->arrive_time[found->station_num-2];
        adjust_date(day, t);
        std::cout << found->stations[found->station_num-1] << ' ' << day << ' ' << t <<
        " -> xx-xx xx:xx " << found->price[found->station_num-1] << " x\n";
    }

    // p = 0 for "-p time", p = 1 for "-p cost"
    void query_ticket(const Mystring<31>& a, const Mystring<31>& b, Date d, bool p) 
    {
        vector<Index_Info> candidate;
        vector<char> to_num;
        find_train(a, b, candidate, to_num);
        int size = candidate.size();
        Journey_Data journey;
        vector<Journey_Data> res;
        for (int i = 0; i < size; ++i)
        {
            auto train = train_db.readonly(candidate[i].train_id);
            // check validity
            if (candidate[i].num == train->station_num) continue;
            int offset = 0;
            journey.leave_time = train->leave_time[candidate[i].num];
            while (journey.leave_time.h >= 24)
            {
                ++offset;
                journey.leave_time.h -= 24;
            }
            Date require_date = d;
            require_date -= offset;
            if (require_date < train->start_date || train->end_date < require_date) continue;
            // fill in information
            strcpy(journey.train_id, candidate[i].train_id.string);
            journey.leave_date = d;
            journey.arrive_date = require_date;
            journey.arrive_time = train->arrive_time[to_num[i]-1];
            journey.time = journey.arrive_time - journey.leave_time;
            adjust_date(journey.arrive_date, journey.arrive_time);
            journey.price = train->price[to_num[i]] - train->price[candidate[i].num];
            journey.seat = 1e9;
            for (char j = candidate[i].num; j < to_num[i]; j++)
                journey.seat = std::min(journey.seat, train->seat[j]);
            res.push_back(journey);
        }
        size = res.size();
        int* array = new int[size];
        for (int i = 0; i < size; i++)
            array[i] = i;
        if (!p) // -p time
        {
            sort(array, array+size, 
            [&res](int x, int y)
            {
                return res[x].time < res[y].time;
            });
        }
        else // -p cost
        {
            sort(array, array+size, 
            [&res](int a, int b)
            {
                return res[a].price < res[b].price;
            });
        }
        std::cout << size << '\n';
        for (int i = 0; i < size; i++)
        {
            int j = array[i];
            std::cout << res[j].train_id << ' ' << a << ' ' << res[j].leave_date << ' ' << res[j].leave_time << " -> " <<
            b << ' ' << res[j].arrive_date << ' ' << res[j].arrive_time << ' ' << res[j].price << ' ' << res[j].seat << '\n';
        }
    }

    void query_transfer(const Mystring<31>& a, const Mystring<31>& b, Date d, bool p)
    {

    }

    void buy_ticket(const Mystring<21>& u, const Mystring<21>& id, Date d,
                    const Mystring<31>& from, const Mystring<31>& to, bool q)
    {

    }

    void query_order(const Mystring<21>& u)
    {
        
    }

    int refund_ticket(const Mystring<21>& u, int n)
    {
        return 0;
    }

    void clean()
    {
        train_db.clean();
        train_index.clean();
        order_db.clean();
        order_index.clean();
        order_queue.clean();
    }

private:
    struct Index_Info
    {
        Mystring<21> train_id;
        char num;
        friend bool operator<(const Index_Info& a, const Index_Info& b)
        {
            return a.train_id < b.train_id;
        }
        friend bool operator==(const Index_Info& a, const Index_Info& b)
        {
            return a.train_id == b.train_id;
        }
    };
    BPT<Mystring<21>, Train_Data> train_db;
    Multi_BPT<Mystring<31>, Index_Info> train_index;
    BPT<int, Order_Data> order_db;
    Multi_BPT<Mystring<16>, int> order_index;
    Multi_BPT<Mystring<21>, int> order_queue;

    // find all trains that go from a to b
    void find_train(const Mystring<31>& a, const Mystring<31>& b, vector<Index_Info>& res, vector<char>& to_num)
    {
        vector<Index_Info> av, bv;
        train_index.find(a, av);
        train_index.find(b, bv);
        auto aptr = av.begin();
        auto bptr = bv.begin();
        while (true)
        {
            while (aptr != av.end() && (*aptr).train_id < (*bptr).train_id) ++aptr;
            if (aptr == av.end()) return;
            if ((*aptr).train_id == (*bptr).train_id)
            {
                if ((*aptr).num < (*bptr).num)
                {
                    res.push_back(*aptr);
                    to_num.push_back((*bptr).num);
                }
                ++aptr;
            }
            ++bptr;
            if (aptr == av.end() || bptr == bv.end()) return;
            while (bptr != bv.end() && (*bptr).train_id < (*aptr).train_id) ++bptr;
            if (bptr == bv.end()) return;
            if ((*aptr).train_id == (*bptr).train_id)
            {
                if ((*aptr).num < (*bptr).num)
                {
                    res.push_back(*aptr);
                    to_num.push_back((*bptr).num);
                }
                ++bptr;
            }
            ++aptr;
            if (aptr == av.end() || bptr == bv.end()) return; 
        }
    }

};

} // namespace sjtu

#endif
