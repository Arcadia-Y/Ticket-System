// a subsystem to process train and order data
#ifndef TRAIN_SYSTEM_HPP
#define TRAIN_SYSTEM_HPP

#include <iostream>
#include "B_plus_tree/BPT.hpp"
#include "STLite/algorithm.hpp"
#include "STLite/utility.hpp"
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
    Date start_date;
    Date end_date;
    Time leave_time[MAXSTA-1]; // no leave_time for last station
    Time arrive_time[MAXSTA-1]; // no arrive_time for first station
    char stations[MAXSTA][31];
    bool released;
    char station_num;
    char type;
    int seat; // no seat for last station
    int price[MAXSTA]; // the price from first station
};

class Train_System
{
public:
    Train_System(): train_db("train"), train_index("station_index"), seat_db("seat"),
    order_db("order"), order_index("user_order_index"), order_queue("order_queue") {}
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
        Seat_Index index;
        index.id = id;
        index.date = found->start_date;
        Seats seats;
        for (int i = 0; i < found->station_num - 1; i++)
            seats[i] = found->seat;
        for (; index.date < found->end_date; ++index.date)
            seat_db.insert(index, seats);
        seat_db.insert(index, seats);
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
        if (found->released)
        {
            Seat_Index index;
            index.date = d;
            index.id = id;
            auto seat = seat_db.readonly(index);
            std::cout << id << ' ' << found->type << '\n';
            std::cout << found->stations[0] << " xx-xx xx:xx -> " << d << ' ' << 
            found->leave_time[0] << " 0 " << seat->s[0] << '\n';
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
                adjust_date(day, t);
                std::cout << day << ' ' << t << ' ' << found->price[i] << ' ' << seat->s[i] << '\n';
            }
            day = d;
            t = found->arrive_time[found->station_num-2];
            adjust_date(day, t);
            std::cout << found->stations[found->station_num-1] << ' ' << day << ' ' << t <<
            " -> xx-xx xx:xx " << found->price[found->station_num-1] << " x\n";
        }
        else
        {
            std::cout << id << ' ' << found->type << '\n';
            std::cout << found->stations[0] << " xx-xx xx:xx -> " << d << ' ' << 
            found->leave_time[0] << " 0 " << found->seat << '\n';
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
                adjust_date(day, t);
                std::cout << day << ' ' << t << ' ' << found->price[i] << ' ' << found->seat << '\n';
            }
            day = d;
            t = found->arrive_time[found->station_num-2];
            adjust_date(day, t);
            std::cout << found->stations[found->station_num-1] << ' ' << day << ' ' << t <<
            " -> xx-xx xx:xx " << found->price[found->station_num-1] << " x\n";
        } 
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
            Time origin_leave_time = journey.leave_time = train->leave_time[candidate[i].num];
            int offset = 0;
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
            journey.time = journey.arrive_time - origin_leave_time;
            adjust_date(journey.arrive_date, journey.arrive_time);
            journey.price = train->price[to_num[i]] - train->price[candidate[i].num];
            journey.seat = 1e9;
            Seat_Index index;
            index.id = candidate[i].train_id;
            index.date = require_date;
            auto seat = seat_db.readonly(index);
            for (char j = candidate[i].num; j < to_num[i]; j++)
                journey.seat = std::min(journey.seat, seat->s[j]);
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
            [&res](int x, int y)
            {
                return res[x].price < res[y].price;
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
        Transfer_Info info;
        info.cost = info.time = 1e9;
        if (!find_transfer(a, b, d, p, info))
        {
            std::cout << "0\n";
            return;
        }
        // print a_train info
        auto a_train = train_db.readonly(info.train_id[0]);
        Time a_leave_time = a_train->leave_time[info.f_id[0]], a_arrive_time = a_train->arrive_time[info.t_id[0]-1];
        int a_offset = a_leave_time.h / 24;
        a_leave_time.h %= 24;
        Seat_Index a_index;
        a_index.id = info.train_id[0];
        Date a_arrive_date = a_index.date = d - a_offset;
        adjust_date(a_arrive_date, a_arrive_time);
        auto a_seat = seat_db.readonly(a_index);
        int left = 1e9;
        for (int i = info.f_id[0]; i < info.t_id[0]; i++)
            left = std::min(left, a_seat->s[i]);
        std::cout << info.train_id[0] << ' ' << a << ' ' << d << ' ' << a_leave_time << " -> " << a_train->stations[info.t_id[0]] << ' ' << 
        a_arrive_date << ' ' << a_arrive_time << ' ' << a_train->price[info.t_id[0]] - a_train->price[info.f_id[0]] << ' ' << left << '\n';
        // print b_train info
        auto b_train = train_db.readonly(info.train_id[1]);
        Time b_leave_time = b_train->leave_time[info.f_id[1]], b_arrive_time = b_train->arrive_time[info.t_id[1]-1];
        Seat_Index b_index;
        b_index.id = info.train_id[1];
        Date b_leave_date = b_index.date = info.date;
        Date b_arrive_date = b_leave_date;
        adjust_date(b_leave_date, b_leave_time);
        adjust_date(b_arrive_date, b_arrive_time);
        auto b_seat = seat_db.readonly(b_index);
        left = 1e9;
        for (int i = info.f_id[1]; i < info.t_id[1]; i++)
            left = std::min(left, b_seat->s[i]);
        std::cout << info.train_id[1] << ' ' << b_train->stations[info.f_id[1]] << ' ' << b_leave_date << ' ' << b_leave_time << " -> " << b << ' ' <<
        b_arrive_date << ' ' << b_arrive_time << ' ' << b_train->price[info.t_id[1]] - b_train->price[info.f_id[1]] << ' ' << left << '\n';
    }

    void buy_ticket(const Mystring<21>& u, const Mystring<21>& id, Date d,
                    const Mystring<31>& from, const Mystring<31>& to, int n, bool q)
    {
        auto train = train_db.readonly(id);
        if (train == nullptr || !train->released)
        {
            std::cout << "-1\n";
            return;
        }
        int f_id = -1, t_id = -1;
        for (int i = 0; i < train->station_num; i++)
        {
            if (!strcmp(from.string, train->stations[i]))
                f_id = i;
            else if (!strcmp(to.string, train->stations[i]))
            {
                t_id = i;
                break;
            }
        }
        if (f_id == -1 || t_id == -1)
        {
            std::cout << "-1\n";
            return;
        }
        Seat_Index index;
        index.id = id;
        int offset = train->leave_time[f_id].h / 24;
        index.date = d - offset;
        auto seat = seat_db.readonly(index);
        if (seat == nullptr)
        {
            std::cout << "-1\n";
            return;
        }
        int left = 1e9;
        for (int i = f_id; i < t_id; i++)
            left = std::min(left, seat->s[i]);
        if ((left < n && !q) || n > train->seat)
        {
            std::cout << "-1\n";
            return;
        }
        Order_Data order;
        strcpy(order.train_id, id.string);
        order.d = index.date;
        order.f_id = f_id;
        order.t_id = t_id;
        order.num = n;
        if (left >= n)
        {
            order.state = 1;
            long address = order_db.new_space();
            order_index.insert(u, -address);
            order_db.write(address, order);
            auto seat2 = seat_db.readwrite(index);
            for (int i = f_id; i < t_id; i++)
                seat2->s[i] -= n;
            std::cout << (long long)n * (train->price[t_id] - train->price[f_id]) << '\n';
            return;
        }
        order.state = 0;
        long address = order_db.new_space();
        order_index.insert(u, -address);
        order_db.write(address, order);
        order_queue.insert(index, address);
        std::cout << "queue\n";
    }

    void query_order(const Mystring<21>& u)
    {
        vector<long> res;
        order_index.find(u, res);
        std::cout << res.size() << '\n';
        for (auto i = res.begin(); i != res.end(); i++)
        {
            auto order = order_db.readonly(-*i);
            auto train = train_db.readonly(order->train_id);
            if (order->state == 1)
                std::cout << "[success] ";
            else if (!order->state)
                std::cout << "[pending] ";
            else
                std::cout << "[refunded] ";
            std::cout << order->train_id << ' ';
            std::cout << train->stations[order->f_id] << ' ';
            Date d = order->d;
            Time t = train->leave_time[order->f_id];
            adjust_date(d, t);
            std::cout << d << ' ' << t << " -> ";
            std::cout << train->stations[order->t_id] << ' ';
            d = order->d;
            t = train->arrive_time[order->t_id-1];
            adjust_date(d, t);
            std::cout << d << ' ' << t << ' ';
            std::cout << train->price[order->t_id] - train->price[order->f_id] << ' ';
            std::cout << order->num << '\n';
        }
    }

    int refund_ticket(const Mystring<21>& u, int n)
    {
        vector<long> order_v;
        order_index.find(u, order_v);
        if (n >= order_v.size()) return -1;
        auto order = order_db.readwrite(-order_v[n]);
        if (order->state == -1) return -1;
        Seat_Index index;
        index.id = order->train_id;
        index.date = order->d;
        if (order->state == 0)
        {
            order->state = -1;
            order_queue.erase(index, -order_v[n]);
            return 0;
        }
        order->state = -1;
        auto seat = seat_db.readwrite(index);
        for (int i = order->f_id; i < order->t_id; i++)
            seat->s[i] += order->num;
        vector<long> queue;
        order_queue.find(index, queue);
        for (auto it = queue.begin(); it != queue.end(); it++)
        {
            auto cptr = order_db.readonly(*it);
            int left = 1e9;
            for (int i = cptr->f_id; i < cptr->t_id; i++)
                left = std::min(seat->s[i], left);
            if (left < cptr->num) continue;
            auto ptr = order_db.readwrite(*it);
            ptr->state = 1;
            for (int i = cptr->f_id; i < cptr->t_id; i++)
                seat->s[i] -= cptr->num;
            order_queue.erase(index, *it);
        }
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
    struct Order_Data
    {
        signed char state;// -1 for refunded, 0 for pending, 1 for success
        char f_id;
        char t_id;
        Date d; // departure date of the train, not the order
        char train_id[21];
        int num;
    };
    struct Journey_Data
    {
        int time;
        int price;
        int seat;
        char train_id[21];
        Date leave_date;
        Time leave_time;
        Date arrive_date;
        Time arrive_time;
    };
    struct Index_Info
    {
        char num;
        Mystring<21> train_id;
        friend bool operator<(const Index_Info& a, const Index_Info& b)
        {
            return a.train_id < b.train_id;
        }
        friend bool operator==(const Index_Info& a, const Index_Info& b)
        {
            return a.train_id == b.train_id;
        }
    };
    struct Seat_Index
    {
        Date date;
        Mystring<21> id;
        friend bool operator<(const Seat_Index& a, const Seat_Index& b)
        {
            int res = strcmp(a.id.string, b.id.string);
            if (res) return res < 0;
            return a.date < b.date;
        }
        friend bool operator==(const Seat_Index& a, const Seat_Index& b)
        {
            return a.id == b.id && a.date == b.date;
        }
    };
    struct Seats
    {
        int s[MAXSTA-1];
        int& operator[](int i)
        {
            return s[i];
        }
    };
    struct Transfer_Info
    {
        int time;
        int cost;
        Mystring<21> train_id[2];
        Date date; // departure date of train[1]
        char f_id[2];
        char t_id[2];
    };
    BPT<Mystring<21>, Train_Data> train_db;
    Multi_BPT<Mystring<31>, Index_Info> train_index; // station name as index
    BPT<Seat_Index, Seats> seat_db;
    Datafile<Order_Data> order_db;
    Multi_BPT<Mystring<21>, long> order_index; // long is -address in order_db, username as index
    Multi_BPT<Seat_Index, long> order_queue; // long is address in order_db

    // find all trains that go from a to b
    void find_train(const Mystring<31>& a, const Mystring<31>& b, vector<Index_Info>& res, vector<char>& to_num)
    {
        vector<Index_Info> av, bv;
        train_index.find(a, av);
        if (av.empty()) return;
        train_index.find(b, bv);
        if (bv.empty()) return;
        auto aptr = av.begin();
        auto bptr = bv.begin();
        int toreserve = std::min(av.size(), bv.size());
        res.reserve(toreserve);
        to_num.reserve(toreserve);
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

    static bool transfer_comp_time(const Transfer_Info& a, const Transfer_Info& b)
    {
        if (a.time != b.time) return a.time < b.time;
        if (a.cost != b.cost) return a.cost < b.cost;
        int res = strcmp(a.train_id[0].string, b.train_id[0].string);
        if (res) return res < 0;
        return a.train_id[1] < b.train_id[1];
    }

    static bool transfer_comp_cost(const Transfer_Info& a, const Transfer_Info& b)
    {
        if (a.cost != b.cost) return a.cost < b.cost;
        if (a.time != b.time) return a.time < b.time;
        int res = strcmp(a.train_id[0].string, b.train_id[0].string);
        if (res) return res < 0;
        return a.train_id[1] < b.train_id[1];
    }

    // find the best transfer info
    bool find_transfer(const Mystring<31>& a, const Mystring<31>& b, Date d, bool p, Transfer_Info& ret)
    {
        bool (*comp)(const Transfer_Info& a, const Transfer_Info& b);
        if (!p) comp = transfer_comp_time;
        else comp = transfer_comp_cost;
        bool flag = false;
        vector<Index_Info> a_index, b_index;
        train_index.find(a, a_index);
        // from_a: station as index, pair<id in a_index, t_id> as value
        map<Mystring<31>, vector<pair<int, char>>> from_a;
        // insert reachable city into from_a
        for (int i = 0; i < a_index.size(); i++)
        {
            // check date
            auto train = train_db.readonly(a_index[i].train_id);
            int offset = train->leave_time[a_index[i].num].h / 24;
            Date require_d = d - offset;
            if (require_d < train->start_date || train->end_date < require_d)
                continue;
            // insert
            for (char j = a_index[i].num + 1; j < train->station_num; j++)
            {
                if (train->stations[j] == b) continue;
                pair<int, char> toinsert(i, j);
                auto found = from_a.find(train->stations[j]);
                if (found == from_a.end())
                {
                    vector<pair<int, char>> tmp_v;
                    tmp_v.push_back(toinsert);
                    from_a.insert(pair<Mystring<31>, vector<pair<int, char>>>(train->stations[j], tmp_v));
                }
                else
                    found->second.push_back(toinsert);
            }
        }
        if (from_a.empty()) return flag;
        // iterate over trains passing by b
        train_index.find(b, b_index);
        Transfer_Info tmp_info;
        for (int i = 0; i < b_index.size(); i++)
        {
            // check date (roughly)
            char b_id = b_index[i].num;
            auto b_train = train_db.readonly(b_index[i].train_id);
            char offset = b_train->arrive_time[b_id-1].h / 24;
            Time b_arrive_t = b_train->arrive_time[b_id];
            b_arrive_t.h -= 24 * offset;
            Date require_d = d - offset;
            if (b_train->end_date < require_d)
                continue;
            // iterate over stations earlier than b
            tmp_info.train_id[1] = b_index[i].train_id;
            tmp_info.t_id[1] = b_id;
            for (int j = 0; j < b_id; j++)
            {
                auto found = from_a.find(b_train->stations[j]);
                if (found == from_a.end()) continue;
                // iterate over possible train[0]
                for (auto k = found->second.begin(); k != found->second.end(); k++)
                {
                    auto a_id = a_index[(*k).first].train_id;
                    // check duplicate
                    if (a_id == b_index[i].train_id)
                        continue;
                    char f_id = a_index[(*k).first].num;
                    char t_id = (*k).second;
                    auto a_train = train_db.readonly(a_id);
                    // find earliest required departure date of b_train
                    Time a_t = a_train->leave_time[f_id], t_t = a_train->arrive_time[t_id-1];
                    Date t_d = d;
                    t_d += t_t.h / 24 - a_t.h / 24;
                    t_t.h %= 24;
                    Time b_leave_t = b_train->leave_time[j];
                    offset = b_leave_t.h / 24;
                    b_leave_t.h %= 24;
                    require_d = t_d - offset + (int)(b_leave_t < t_t);
                    if (b_train->end_date < require_d)
                        continue;
                    // fill in tmp_info
                    flag = true;
                    tmp_info.train_id[0] = a_id;
                    tmp_info.date = std::max(b_train->start_date, require_d);
                    tmp_info.time = (a_train->arrive_time[t_id-1] - a_train->leave_time[f_id]) +
                        time_between(t_d, t_t, tmp_info.date + offset, b_leave_t) + 
                        (b_train->arrive_time[b_id-1] - b_train->leave_time[j]);
                    tmp_info.cost = (a_train->price[t_id] - a_train->price[f_id]) + 
                        (b_train->price[b_id] - b_train->price[j]);
                    // try update ret
                    if (comp(tmp_info, ret))
                    {
                        tmp_info.f_id[0] = f_id;
                        tmp_info.f_id[1] = j;
                        tmp_info.t_id[0] = t_id;
                        ret = tmp_info;
                    }
                }
            }
        }
        return flag; 
    }

};

} // namespace sjtu

#endif