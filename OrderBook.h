#include <iostream>
#include <map>
#include <set>
#include <functional>

using namespace std::placeholders;

class OrderBook
{
private:

    struct IDInfo
    {
        std::string sym;
        bool sideBid;
        double price;
    };

    struct Bid_compare {
        bool operator() (const std::pair<double, uint64_t>& lhs, const std::pair<double, uint64_t>& rhs) const {
            return lhs.first > rhs.first|| (lhs.first == rhs.first && lhs.second < rhs.second);
        }
    };
    struct Ask_compare {
        bool operator() (const std::pair<double, uint64_t>& lhs, const std::pair<double, uint64_t>& rhs) const {
            return lhs.first < rhs.first || (lhs.first == rhs.first && lhs.second < rhs.second);
        }
    };

    const bool filter;
    const std::string f_sym;

    std::set<std::string> BBOList;
    std::map<std::string, std::pair<std::map<uint64_t, std::pair<double, double>>, std::pair<double, double>>> VWAPInfo;

    std::map<uint64_t, IDInfo> IDMap;
    std::map<std::string, std::pair<std::map<std::pair<double, uint64_t>, uint64_t, Bid_compare>, std::map<std::pair<double, uint64_t>, uint64_t, Ask_compare>>> OrderList;

    const std::map<std::string, std::function<void(const std::string&, size_t&)>> StringToCommand =
    {
        {"ORDER ADD", std::bind(&OrderBook::OrderAdd, this, _1, _2)},
        {"ORDER MODIFY", std::bind(&OrderBook::OrderModify, this, _1, _2)},
        {"ORDER CANCEL", std::bind(&OrderBook::OrderCancel, this, _1, _2)},
        {"SUBSCRIBE BBO", std::bind(&OrderBook::BBOSub, this, _1, _2)},
        {"UNSUBSCRIBE BBO", std::bind(&OrderBook::BBOUnsub, this, _1, _2)},
        {"SUBSCRIBE VWAP", std::bind(&OrderBook::VWAPSub, this, _1, _2)},
        {"UNSUBSCRIBE VWAP", std::bind(&OrderBook::VWAPUnsub, this, _1, _2)},
        {"PRINT", std::bind(&OrderBook::Print, this, _1, _2)},
        {"PRINT_FULL", std::bind(&OrderBook::PrintFull, this, _1, _2)}
    };

public:
    OrderBook(const std::string& s)
        : f_sym(s)
        , filter(!s.empty())
    {
    }

    void parseLine(const std::string& line)
    {
        std::cout << "Do: \"" << line << "\"\n";
        size_t p = 0;
        StringToCommand.find(parse(line, p)).operator->()->second(line, p);
        /*
        try {
            StringToCommand.find(parse(line, p)).operator->()->second(line, p);
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
        }*/
    }

private:

    static std::string parse(const std::string& line, size_t& p)
    {
        const size_t start = p;
        for (char c : line.substr(start)) {
            p++;
            if (c == ',') {
                break;
            }
        }
        return line.substr(start, p - start - (p >= line.length() ? 0 : 1));
    }

    void OrderAdd(const std::string& line, size_t& p); /// Do ORDER_ADD calls
    void OrderModify(const std::string& line, size_t& p); /// Do ORDER_MODIFY calls
    void OrderCancel(const std::string& line, size_t& p); /// Do ORDER_CANCEL calls
    void BBOSub(const std::string& line, size_t& p); /// Do SUBSCRIBE_BBO calls
    void BBOUnsub(const std::string& line, size_t& p); /// Do UNSUBSCRIBE_BBO calls
    void VWAPSub(const std::string& line, size_t& p); /// Do SUBSCRIBE_VWAP calls
    void VWAPUnsub(const std::string& line, size_t& p); /// Do UNSUBSCRIBE_VWAP calls
    void Print(const std::string& line, size_t& p); /// Do PRINT calls
    void PrintFull(const std::string& line, size_t& p); /// Do PRINT_FULL calls

    void BBOPrint(const std::string& sym); /// Print BBO, can't seek changes

    void VWAPSinglePrint(const std::string& sym, const uint64_t& vol); /// Calls only at SUBSCRIBE_VWAP
    void VWAPAutoPrint(const std::string& sym, const double& minPrice, const bool& sideBid); /// Seek changes at all VWAP subscribes and print only changed

};
