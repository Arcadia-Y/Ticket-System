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
    bool released;
    char station_num;
    char type;
    Date start_date;
    Date end_date;
    Time leave_time[MAXSTA-1]; // no leave_time for last station
    Time arrive_time[MAXSTA-1]; // no arrive_time for first station
    char stations[MAXSTA][31];
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
        vector<Index_Info> av, bv, candidate;
        vector<char> to_num;
        train_index.find(a, av);
        train_index.find(b, bv);
        find_train(av, bv, candidate, to_num);
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
        find_transfer(a, b, d, p, info);
        if (info.time == 1e9)
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

    void buy_ticket(int timestamp, const Mystring<21>& u, const Mystring<21>& id, Date d,
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
            order_index.insert(u, -timestamp);
            order_db.insert(timestamp, order);
            auto seat2 = seat_db.readwrite(index);
            for (int i = f_id; i < t_id; i++)
                seat2->s[i] -= n;
            std::cout << (long long)n * (train->price[t_id] - train->price[f_id]) << '\n';
            return;
        }
        order.state = 0;
        order_index.insert(u, -timestamp);
        order_db.insert(timestamp, order);
        order_queue.insert(index, timestamp);
        std::cout << "queue\n";
    }

    void query_order(const Mystring<21>& u)
    {
        vector<int> res;
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
        vector<int> order_v;
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
        vector<int> queue;
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
        char train_id[21];
        char f_id;
        char t_id;
        Date d; // departure date of the train, not the order
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
    struct Seat_Index
    {
        Mystring<21> id;
        Date date;
        friend bool operator<(const Seat_Index& a, const Seat_Index& b)
        {
            if (!(a.id == b.id)) return a.id < b.id;
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
    struct Transfer_Index
    {
        int cost;
        Time t1; // leave_time of a
        Time t2; // arrive_time of tranfer station
        char f_id;
        char t_id;
    };
    struct B_Tansfer_Index
    {
        Date start;
        Date end;
        char offset;
    };
    BPT<Mystring<21>, Train_Data> train_db;
    Multi_BPT<Mystring<31>, Index_Info> train_index; // station name as index
    BPT<Seat_Index, Seats> seat_db;
    BPT<int, Order_Data> order_db; // int is timestamp
    Multi_BPT<Mystring<21>, int> order_index; // int is -timestamp, username as index
    Multi_BPT<Seat_Index, int> order_queue; // int is timestamp

    // find all trains that go from a to b
    void find_train(vector<Index_Info>& av, vector<Index_Info>& bv, vector<Index_Info>& res, vector<char>& to_num)
    {
        if (av.empty() || bv.empty()) return;
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

    // find possible transfer station and mark invalid train
    void find_common_station(vector<Index_Info>& a_index, vector<Index_Info>& b_index, Date d,  map<Mystring<31>,bool>& res)
    {
        map<Mystring<31>, bool> from_a;
        for (int i = 0; i < a_index.size(); i++)
        {
            // check date
            auto train = train_db.readonly(a_index[i].train_id);
            int offset = train->leave_time[a_index[i].num].h / 24;
            Date require_d = d - offset;
            if (require_d < train->start_date || train->end_date < require_d)
            {
                a_index[i].num = MAXSTA;
                continue;
            }
            // insert
            for (char j = a_index[i].num + 1; j < train->station_num; j++)
                from_a[train->stations[j]] = true;
        }
        vector<Transfer_Index> blank;
        for (int i = 0; i < b_index.size(); i++)
        {
            auto train = train_db.readonly(b_index[i].train_id);
            if (train->end_date + train->arrive_time[train->station_num-2].h / 24 < d)
            {
                b_index[i].num = MAXSTA;
                continue;
            }
            for (char j = 0; j < b_index[i].num; j++)
                if (from_a.find(train->stations[j]) != from_a.end())
                    res[train->stations[j]] = true;
        }
    }

    static bool transfer_comp_time(const Transfer_Info& a, const Transfer_Info& b)
    {
        if (a.time != b.time) return a.time < b.time;
        if (a.cost != b.cost) return a.cost < b.cost;
        if (!(a.train_id[0] == b.train_id[0])) return a.train_id[0] < b.train_id[0];
        return a.train_id[1] < b.train_id[1];
    }

    static bool transfer_comp_cost(const Transfer_Info& a, const Transfer_Info& b)
    {
        if (a.cost != b.cost) return a.cost < b.cost;
        if (a.time != b.time) return a.time < b.time;
        if (!(a.train_id[0] == b.train_id[0])) return a.train_id[0] < b.train_id[0];
        return a.train_id[1] < b.train_id[1];
    }

    void check_transfer_station_time(const vector<Index_Info>& a_train, const vector<Index_Info>& b_train, const vector<char>& a_to_num,
        const vector<char>& b_to_num, Date d, Transfer_Info& res)
    {
        vector<Transfer_Index> a_index, b_index;
        vector<B_Tansfer_Index> b_info;
        int a_size = a_train.size(), b_size = b_train.size();
        a_index.reserve(a_size+1);
        b_index.reserve(b_size+1);
        b_info.reserve(b_size+1);
        int* a_ptr = new int[a_size];
        int* b_ptr = new int[b_size];
        // fill in index_info
        for (int i = 0; i < a_size; i++)
        {
            auto train = train_db.readonly(a_train[i].train_id);
            a_ptr[i] = i;
            a_index[i].f_id = a_train[i].num;
            a_index[i].t_id = a_to_num[i];
            a_index[i].t1 = train->leave_time[a_index[i].f_id];
            a_index[i].t2 = train->arrive_time[a_index[i].t_id-1];
            char a_offset = a_index[i].t1.h / 24;
            a_index[i].t1.h -= a_offset * 24;
            a_index[i].t2.h -= a_offset * 24;
            a_index[i].cost = train->price[a_index[i].t_id] - train->price[a_index[i].f_id];
        }
        for (int i = 0; i < b_size; i++)
        {
            auto train = train_db.readonly(b_train[i].train_id);
            b_ptr[i] = i;
            b_index[i].f_id = b_train[i].num;
            b_index[i].t_id = b_to_num[i];
            b_index[i].t1 = train->leave_time[b_index[i].f_id];
            b_index[i].t2 = train->arrive_time[b_index[i].t_id-1];
            b_index[i].cost = train->price[b_index[i].t_id] - train->price[b_index[i].f_id];
            b_info[i].start = train->start_date;
            b_info[i].end = train->end_date;
            b_info[i].offset = b_index[i].t1.h / 24;
            b_index[i].t1.h -= b_info[i].offset * 24;
            b_index[i].t2.h -= b_info[i].offset * 24;
        }
        sort(a_ptr, a_ptr+a_size, [&a_index](int x, int y){ return a_index[x].t2 < a_index[y].t2;});
        int* a_final = new int[a_size];
        a_final[0] = a_ptr[0];
        int a_cnt = 1;
        for (int i = 1; i < a_size; ++i)
        {
            if (a_index[a_ptr[i]].t1 < a_index[a_final[a_cnt-1]].t1) continue;
            a_final[a_cnt] = a_ptr[i];
            ++a_cnt;
        }
        delete []a_ptr;
        sort(b_ptr, b_ptr+b_size, [&b_info](int x, int y){ return b_info[y].end + b_info[y].offset < b_info[x].end + b_info[x].offset;});
        int b_last = b_size;
        Transfer_Info tmp_info;
        for (int i = 0; i < a_cnt; i++)
        {
            int I = a_final[i];
            char a_offset = a_index[I].t2.h / 24;
            a_index[I].t2.h -= a_offset * 24;
            tmp_info.train_id[0] = a_train[I].train_id;
            tmp_info.f_id[0] = a_train[I].num;
            tmp_info.t_id[0] = a_to_num[I];
            for (int j = 0; j < b_last; j++)
            {
                int J = b_ptr[j];
                Date require_d = d + a_offset - b_info[J].offset + (int)(b_index[J].t1 < a_index[I].t2);
                if (b_info[J].end < require_d)
                {
                    b_last = j;
                    break;
                }
                // check duplicate
                if (b_train[J].train_id == a_train[I].train_id) continue;
                tmp_info.date = std::max(require_d, b_info[J].start);
                tmp_info.time = time_between(d, a_index[I].t1, tmp_info.date + b_info[J].offset, b_index[J].t2);
                tmp_info.cost = a_index[I].cost + b_index[J].cost;
                if (transfer_comp_time(tmp_info, res))
                {
                    tmp_info.f_id[1] = b_index[J].f_id;
                    tmp_info.t_id[1] = b_index[J].t_id;
                    tmp_info.train_id[1] = b_train[J].train_id;
                    res = tmp_info;
                }
            }
        }
        delete []a_final;
        delete []b_ptr;
    }

    void check_transfer_station_cost(const vector<Index_Info>& a_train, const vector<Index_Info>& b_train, const vector<char>& a_to_num,
        const vector<char>& b_to_num, Date d, Transfer_Info& res)
    {
        vector<Transfer_Index> a_index, b_index;
        vector<B_Tansfer_Index> b_info;
        int a_size = a_train.size(), b_size = b_train.size();
        a_index.reserve(a_size+1);
        b_index.reserve(b_size+1);
        b_info.reserve(b_size+1);
        int* a_ptr = new int[a_size];
        int* b_ptr = new int[b_size];
        // fill in index_info
        for (int i = 0; i < a_size; i++)
        {
            auto train = train_db.readonly(a_train[i].train_id);
            a_ptr[i] = i;
            a_index[i].f_id = a_train[i].num;
            a_index[i].t_id = a_to_num[i];
            a_index[i].t1 = train->leave_time[a_index[i].f_id];
            a_index[i].t2 = train->arrive_time[a_index[i].t_id-1];
            char a_offset = a_index[i].t1.h / 24;
            a_index[i].t1.h -= a_offset * 24;
            a_index[i].t2.h -= a_offset * 24;
            a_index[i].cost = train->price[a_index[i].t_id] - train->price[a_index[i].f_id];
        }
        for (int i = 0; i < b_size; i++)
        {
            auto train = train_db.readonly(b_train[i].train_id);
            b_ptr[i] = i;
            b_index[i].f_id = b_train[i].num;
            b_index[i].t_id = b_to_num[i];
            b_index[i].t1 = train->leave_time[b_index[i].f_id];
            b_index[i].t2 = train->arrive_time[b_index[i].t_id-1];
            b_index[i].cost = train->price[b_index[i].t_id] - train->price[b_index[i].f_id];
            b_info[i].start = train->start_date;
            b_info[i].end = train->end_date;
            b_info[i].offset = b_index[i].t1.h / 24;
            b_index[i].t1.h -= b_info[i].offset * 24;
            b_index[i].t2.h -= b_info[i].offset * 24;
        }
        sort(a_ptr, a_ptr+a_size, [&a_index](int x, int y){ return a_index[x].t2 < a_index[y].t2;});
        int* a_final = new int[a_size];
        a_final[0] = a_ptr[0];
        int a_cnt = 1;
        for (int i = 1; i < a_size; ++i)
        {
            if (a_index[a_ptr[i]].cost > a_index[a_final[a_cnt-1]].cost) continue;
            a_final[a_cnt] = a_ptr[i];
            ++a_cnt;
        }
        delete []a_ptr;
        sort(b_ptr, b_ptr+b_size, [&b_info](int x, int y){ return b_info[y].end + b_info[y].offset < b_info[x].end + b_info[x].offset;});
        int b_last = b_size;
        Transfer_Info tmp_info;
        for (int i = 0; i < a_cnt; i++)
        {
            int I = a_final[i];
            char a_offset = a_index[I].t2.h / 24;
            a_index[I].t2.h -= a_offset * 24;
            tmp_info.train_id[0] = a_train[I].train_id;
            tmp_info.f_id[0] = a_train[I].num;
            tmp_info.t_id[0] = a_to_num[I];
            for (int j = 0; j < b_last; j++)
            {
                int J = b_ptr[j];
                Date require_d = d + a_offset - b_info[J].offset + (int)(b_index[J].t1 < a_index[I].t2);
                if (b_info[J].end < require_d)
                {
                    b_last = j;
                    break;
                }
                // check duplicate
                if (b_train[J].train_id == a_train[I].train_id) continue;
                tmp_info.date = std::max(require_d, b_info[J].start);
                tmp_info.time = time_between(d, a_index[I].t1, tmp_info.date + b_info[J].offset, b_index[J].t2);
                tmp_info.cost = a_index[I].cost + b_index[J].cost;
                if (transfer_comp_cost(tmp_info, res))
                {
                    tmp_info.f_id[1] = b_index[J].f_id;
                    tmp_info.t_id[1] = b_index[J].t_id;
                    tmp_info.train_id[1] = b_train[J].train_id;
                    res = tmp_info;
                }
            }
        }
        delete []a_final;
        delete []b_ptr;
    }

    void find_transfer(const Mystring<31>& a, const Mystring<31>& b, Date d, bool p, Transfer_Info& ret)
    {
        vector<Index_Info> tmpa_index, tmpb_index, b_index, a_index;
        train_index.find(a, tmpa_index);
        if (tmpa_index.empty()) return;
        train_index.find(b, tmpb_index);
        if (tmpb_index.empty()) return;
        map<Mystring<31>, bool> common_station;
        find_common_station(tmpa_index, tmpb_index, d, common_station);
        if (common_station.empty()) return;
        a_index.reserve(tmpa_index.size());
        for (auto i = tmpa_index.begin(); i != tmpa_index.end(); ++i)
            if ((*i).num != MAXSTA)
                a_index.push_back(*i);
        b_index.reserve(tmpb_index.size());
        for (auto i = tmpb_index.begin(); i != tmpb_index.end(); ++i)
            if ((*i).num != MAXSTA)
                b_index.push_back(*i);
        // iterate over common stations
        if (!p)
        {
            for (auto c = common_station.begin(); c != common_station.end(); ++c)
            {
                vector<Index_Info> c_index;
                train_index.find(c->first, c_index);
                vector<Index_Info> train_a, train_b;
                vector<char> a_to_num, b_to_num;
                find_train(a_index, c_index, train_a, a_to_num);
                find_train(c_index, b_index, train_b, b_to_num);
                check_transfer_station_time(train_a, train_b, a_to_num, b_to_num, d, ret);
            }
        }
        else
        {
            for (auto c = common_station.begin(); c != common_station.end(); ++c)
            {
                vector<Index_Info> c_index;
                train_index.find(c->first, c_index);
                vector<Index_Info> train_a, train_b;
                vector<char> a_to_num, b_to_num;
                find_train(a_index, c_index, train_a, a_to_num);
                find_train(c_index, b_index, train_b, b_to_num);
                check_transfer_station_cost(train_a, train_b, a_to_num, b_to_num, d, ret);
            }
        }
    }
};

} // namespace sjtu

#endif
