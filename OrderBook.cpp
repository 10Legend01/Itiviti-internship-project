#include "OrderBook.h"

#include <cfloat>

void OrderBook::OrderAdd(const std::string& line, size_t& p)
{
    const uint64_t id = std::stoull(parse(line, p));
    const auto sym = parse(line, p);
    const bool sideBid = parse(line, p) == "Buy";
    const uint64_t vol = std::stoull(parse(line, p));
    const double price = atof(parse(line, p).c_str());
    if (filter && f_sym != sym) {
        return;
    }

    IDMap[id] = {sym, sideBid, price};
    bool BBO = false;
    //// Repeating, use #define, you can't use ptr
    if (sideBid) {
        OrderList[sym].first.insert({{price, id}, vol});
        if (OrderList[sym].first.begin()->first.first == price) {
            BBO = true;
        }
    }
    else {
        OrderList[sym].second.insert({{price, id}, vol});
        if (OrderList[sym].second.begin()->first.first == price) {
            BBO = true;
        }
    }

    if (BBO) {
        BBOPrint(sym);
    }
    VWAPAutoPrint(sym, price, sideBid);
}

void OrderBook::OrderModify(const std::string& line, size_t& p)
{
    const uint64_t id = std::stoull(parse(line, p));
    const auto sym = IDMap[id].sym;
    const auto newVol = std::stoull(parse(line, p));
    const double newPrice = atof(parse(line, p).c_str());
    if (filter && f_sym != sym) {
        return;
    }
    const auto info = &IDMap[id];
    const bool sideBid = info->sideBid;
    const double price = info->price;

    bool BBO = false;
    //// Repeating, use #define, you can't use ptr
    if (sideBid) {
        auto bidList = &OrderList[sym].first;
        auto node = bidList->extract({price, id});
        node.key() = {newPrice, id};
        node.mapped() = newVol;
        bidList->insert(std::move(node));
        auto firstPrice = &bidList->begin()->first.first;
        if (*firstPrice == price || *firstPrice == newPrice) {
            BBO = true;
        }
    }
    else {
        auto askList = &OrderList[sym].second;
        auto node = askList->extract({price, id});
        node.key() = {newPrice, id};
        node.mapped() = newVol;
        askList->insert(std::move(node));
        auto firstPrice = &askList->begin()->first.first;
        if (*firstPrice == price || *firstPrice == newPrice) {
            BBO = true;
        }
    }

    info->price = newPrice;

    if (BBO) {
        BBOPrint(sym);
    }
    VWAPAutoPrint(sym, sideBid ? std::max(price, newPrice) : std::min(price, newPrice), sideBid);
}

void OrderBook::OrderCancel(const std::string& line, size_t& p)
{
    const uint64_t id = std::stoull(parse(line, p));
    if (IDMap.find(id) == IDMap.end()) {
        return;
    }

    const auto info = &IDMap[id];
    const auto price = info->price;
    const auto sym = info->sym;
    const auto sideBid = info->sideBid;
    bool BBO = false;
    //// Repeating, use #define, you can't use ptr
    if (sideBid) {
        if (OrderList[sym].first.begin()->first.first == price) {
            BBO = true;
        }
        OrderList[sym].first.erase({price, id});
    }
    else {
        if (OrderList[sym].second.begin()->first.first == price) {
            BBO = true;
        }
        OrderList[sym].second.erase({price, id});
    }
    IDMap.erase(id);

    if (BBO) {
        BBOPrint(info->sym);
    }
    VWAPAutoPrint(info->sym, price, sideBid);
}

void OrderBook::BBOSub(const std::string& line, size_t& p)
{
    auto sym = parse(line, p);
    if (!filter || f_sym == sym) {
        BBOList.insert(sym);
    }
    BBOPrint(sym);
}

void OrderBook::BBOUnsub(const std::string& line, size_t& p)
{
    BBOList.erase(parse(line, p));
}

void OrderBook::VWAPSub(const std::string& line, size_t& p)
{
    auto sym = parse(line, p);
    const uint64_t vol = std::stoull(parse(line, p));
    if (!filter || f_sym == sym) {
        VWAPInfo[sym].first[vol] = {-1, -1};
        VWAPSinglePrint(sym, vol);
    }
}

void OrderBook::VWAPUnsub(const std::string& line, size_t& p)
{
    const auto sym = parse(line, p);
    if (!filter || f_sym == sym) {
        VWAPInfo[sym].first.erase(std::stoull(parse(line, p)));
    }
}

void OrderBook::Print(const std::string& line, size_t& p)
{
    const auto sym = parse(line, p);
    if (filter && f_sym != sym) {
        return;
    }
    std::cout << sym << ": PRINT:\n";
    auto bidList = &OrderList[sym].first;
    auto askList = &OrderList[sym].second;
    auto bidPtr = bidList->begin();
    auto askPtr = askList->begin();
    double bidCurrPrice = (bidPtr != bidList->end() ? bidPtr->first.first : 0), askCurrPrice = (askPtr != askList->end() ? askPtr->first.first : 0);
    uint64_t bidCurrVol = 0, askCurrVol = 0;
    while (bidPtr != bidList->end() || askPtr != askList->end()) {
        while (bidPtr != bidList->end() && bidCurrPrice == bidPtr->first.first) {
            bidCurrVol += bidPtr++->second;
        }
        while (askPtr != askList->end() && askCurrPrice == askPtr->first.first) {
            askCurrVol += askPtr++->second;
        }
        std::cout << (bidCurrVol ? std::to_string(bidCurrVol) + '@' + std::to_string(bidCurrPrice) : "NIL")
        << " | " << (askCurrVol ? std::to_string(askCurrVol) + '@' + std::to_string(askCurrPrice) : "NIL") << "\n";
        bidCurrVol = 0;
        askCurrVol = 0;
        if (bidPtr != bidList->end()) {
            bidCurrPrice = bidPtr->first.first;
        }
        if (askPtr != askList->end()) {
            askCurrPrice = askPtr->first.first;
        }
    }
    std::cout << "\n";
}

void OrderBook::PrintFull(const std::string& line, size_t& p)
{
    const auto sym = parse(line, p);
    if (filter && f_sym != sym) {
        return;
    }
    std::cout << sym << ": PRINT_FULL:\n";
    auto bidList = &OrderList[sym].first;
    auto askList = &OrderList[sym].second;
    auto bidPtr = bidList->begin();
    auto askPtr = askList->begin();
    while (bidPtr != bidList->end() || askPtr != askList->end()) {
        std::cout << (bidPtr != bidList->end() ? std::to_string(bidPtr++->second) + '@' + std::to_string(bidPtr->first.first) : "NIL")
        << " | " << (askPtr != askList->end() ? std::to_string(askPtr++->second) + '@' + std::to_string(askPtr->first.first) : "NIL") << "\n";
    }
    std::cout << "\n";
}

void OrderBook::BBOPrint(const std::string& sym)
{
    if (BBOList.find(sym) == BBOList.end()) {
        return;
    }
    auto bidList = &OrderList[sym].first;
    auto askList = &OrderList[sym].second;
    auto bidPtr = bidList->begin();
    auto askPtr = askList->begin();
    double bidCurrPrice = (bidPtr != bidList->end() ? bidPtr->first.first : 0), askCurrPrice = (askPtr != askList->end() ? askPtr->first.first : 0);
    uint64_t bidCurrVol = 0, askCurrVol = 0;
    while (bidPtr != bidList->end() && bidCurrPrice == bidPtr->first.first) {
        bidCurrVol += bidPtr++->second;
    }
    while (askPtr != askList->end() && askCurrPrice == askPtr->first.first) {
        askCurrVol += askPtr++->second;
    }
    std::cout << sym << ": BBO:\n" << (bidCurrVol ? std::to_string(bidCurrVol) + '@' + std::to_string(bidCurrPrice) : "NIL")
    << " | " << (askCurrVol ? std::to_string(askCurrVol) + '@' + std::to_string(askCurrPrice) : "NIL") << "\n\n";
}

void OrderBook::VWAPSinglePrint(const std::string& sym, const uint64_t& vol)
{
    const auto VWAPList = &VWAPInfo[sym].first;
    const auto vwapPtr = VWAPList->find(vol);
    if (vwapPtr != VWAPList->begin() && std::prev(vwapPtr)->second == std::make_pair(0.0, 0.0)) {
        vwapPtr->second = {0, 0};
        std::cout << sym << ": VWAP(" << vol << ") 'bid/ask':\nNIL/NIL\n\n";
        return;
    }

    double bid = 0, ask = 0;
    double bidPrice, askPrice;
    const auto BidList = &OrderList[sym].first;
    const auto AskList = &OrderList[sym].second;

    uint64_t currVol = vol;
    //// Maybe use #define
    for (auto & i : *BidList) {
        bidPrice = i.first.first;
        if (currVol < i.second) {
            bid += currVol * bidPrice;
            currVol = 0;
            break;
        }
        bid += i.second * bidPrice;
        currVol -= i.second;
    }
    if (currVol != 0) {
        bid = 0;
    }
    bid /= vol;
    currVol = vol;
    for (auto & i : *AskList) {
        askPrice = i.first.first;
        if (currVol < i.second) {
            ask += currVol * askPrice;
            currVol = 0;
            break;
        }
        ask += i.second * askPrice;
        currVol -= i.second;
    }
    if (currVol != 0) {
        ask = 0;
    }
    ask /= vol;

    VWAPList->operator[](vol).first = bid;
    VWAPList->operator[](vol).second = ask;
    std::cout << sym << ": VWAP(" << vol << ") 'bid/ask':\n" << (bid ? std::to_string(bid) : "NIL") << '/' << (ask ? std::to_string(ask) : "NIL") << "\n\n";

    if (std::next(vwapPtr) == VWAPList->end()) {
        if (bid == 0) {
            VWAPInfo[sym].second.first = 0;
        }
        else {
            VWAPInfo[sym].second.first = bidPrice;
        }
        if (ask == 0) {
            VWAPInfo[sym].second.second = DBL_MAX;
        }
        else {
            VWAPInfo[sym].second.second = askPrice;
        }
    }

}

void OrderBook::VWAPAutoPrint(const std::string& sym, const double& minPrice, const bool& sideBid)
{
    if (VWAPInfo.find(sym) == VWAPInfo.end() || VWAPInfo[sym].first.empty() || (sideBid && minPrice < VWAPInfo[sym].second.first) || (!sideBid && minPrice > VWAPInfo[sym].second.second)) {
        return;
    }

    double vwap = 0;
    const auto VWAPList = &VWAPInfo[sym].first;
    auto vwapPtr = VWAPList->begin();
    bool change = true;
    if (sideBid) {
        const auto bidList = &OrderList[sym].first;
        auto bidPtr = bidList->begin();
        uint64_t currVol = 0;
        const auto maxPrice = VWAPInfo[sym].second.first;
        while (bidPtr != bidList->end() && bidPtr->first.first >= maxPrice) {
            if (bidPtr->first.first <= minPrice && vwapPtr->first <= bidPtr->second + currVol) { /// Print;
                auto bid = (vwap + (vwapPtr->first - currVol) * bidPtr->first.first) / vwapPtr->first;
                if (vwapPtr->second.first == bid) {
                    change = false;
                    break;
                }
                vwapPtr->second.first = bid;
                const auto ask = vwapPtr->second.second;
                std::cout << sym << ": VWAP(" << vwapPtr->first << ") 'bid/ask':\n" << (bid ? std::to_string(bid) : "NIL") << '/' << (ask ? std::to_string(ask) : "NIL") << "\n\n";
                if (++vwapPtr == VWAPList->end()) {
                    break;
                }
                continue;
            }
            vwap += bidPtr->first.first * bidPtr->second;
            currVol += bidPtr++->second;
        }
        if (vwapPtr == VWAPList->end()) {
            VWAPInfo[sym].second.first = bidPtr->first.first;
        }
        else if (change) {
            VWAPInfo[sym].second.first = -1; /// Change on side Ask
            while (vwapPtr != VWAPList->end() && vwapPtr->second.first != 0) {
                vwapPtr->second.first = 0;
                const auto ask = vwapPtr->second.second;
                std::cout << sym << ": VWAP(" << vwapPtr->first << ") 'bid/ask':\n" << "NIL/" << (ask ? std::to_string(ask) : "NIL") << "\n\n";
                vwapPtr++;
            }
        }

    }
    else {
        const auto askList = &OrderList[sym].second;
        auto askPtr = askList->begin();
        uint64_t currVol = 0;
        const auto maxPrice = VWAPInfo[sym].second.second;
        while (askPtr != askList->end() && askPtr->first.first < maxPrice) {
            if (askPtr->first.first >= minPrice && vwapPtr->first <= askPtr->second + currVol) { /// Print
                auto ask = (vwap + (vwapPtr->first - currVol) * askPtr->first.first) / vwapPtr->first;
                if (vwapPtr->second.second == ask) {
                    change = false;
                    break;
                }
                vwapPtr->second.second = ask;
                const auto bid = vwapPtr->second.first;
                std::cout << sym << ": VWAP(" << vwapPtr->first << ") 'bid/ask':\n" << (bid ? std::to_string(bid) : "NIL") << '/' <<  (ask ? std::to_string(ask) : "NIL") << "\n\n";
                if (++vwapPtr == VWAPList->end()) {
                    break;
                }
                continue;
            }
            vwap += askPtr->first.first * askPtr->second;
            currVol += askPtr++->second;
        }
        if (vwapPtr == VWAPList->end()) {
            VWAPInfo[sym].second.second = askPtr->first.first;
        }
        else if (change) {
            VWAPInfo[sym].second.second = DBL_MAX;
            while (vwapPtr != VWAPList->end() && vwapPtr->second.second != 0) {
                vwapPtr->second.second = 0;
                const auto bid = vwapPtr->second.first;
                std::cout << sym << ": VWAP(" << vwapPtr->first << ") 'bid/ask':\n" << (bid ? std::to_string(bid) : "NIL") << "/NIL" << "\n\n";
                vwapPtr++;
            }
        }
    }
}
