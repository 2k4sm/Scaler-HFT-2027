#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <iomanip>

struct Order {
    uint64_t order_id;
    bool is_buy;
    double price;
    uint64_t quantity;
    uint64_t timestamp_ns;
};

struct PriceLevel {
    double price;
    uint64_t total_quantity;
};


class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    void add_order(const Order& order);
    bool cancel_order(uint64_t order_id);
    bool amend_order(uint64_t order_id, double new_price, uint64_t new_quantity);
    void get_snapshot(size_t depth, std::vector<PriceLevel>& bids, std::vector<PriceLevel>& asks) const;
    void print_book(size_t depth = 10) const;

private:
    struct InternalOrder {
        Order order;
        InternalOrder* next;
    };

    // Simple fixed-size memory pool for InternalOrder
    static constexpr size_t POOL_SIZE = 100000;
    class InternalOrderPool {
    public:
        InternalOrderPool() : free_list_(nullptr), pool_(nullptr) {
            pool_ = new InternalOrder[POOL_SIZE];
            for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
                pool_[i].next = &pool_[i + 1];
            }
            pool_[POOL_SIZE - 1].next = nullptr;
            free_list_ = &pool_[0];
        }
        ~InternalOrderPool() { delete[] pool_; }

        InternalOrder* allocate(const Order& order) {
            if (!free_list_) return nullptr;
            InternalOrder* obj = free_list_;
            free_list_ = free_list_->next;
            obj->order = order;
            obj->next = nullptr;
            return obj;
        }
        void deallocate(InternalOrder* obj) {
            obj->next = free_list_;
            free_list_ = obj;
        }
    private:
        InternalOrder* free_list_;
        InternalOrder* pool_;
    };

    using OrderList = std::vector<InternalOrder*>;
    using PriceMap = std::map<double, OrderList, std::greater<>>;
    using PriceMapAsc = std::map<double, OrderList, std::less<>>;

    PriceMap bids;
    PriceMapAsc asks;
    std::unordered_map<uint64_t, std::tuple<bool, double, size_t>> order_lookup;
    InternalOrderPool pool;
};

// --- Method Definitions ---

void OrderBook::add_order(const Order& order) {
    Order incoming = order;
    bool is_buy = incoming.is_buy;

    // Match incoming order against the book
    if (is_buy) {
        while (incoming.quantity > 0 && !asks.empty()) {
            auto best_ask_it = asks.begin();
            double best_ask_price = best_ask_it->first;
            if (incoming.price < best_ask_price) break;

            OrderList& ask_list = best_ask_it->second;
            while (incoming.quantity > 0 && !ask_list.empty()) {
                InternalOrder* ask_order = ask_list.front();
                uint64_t trade_qty = std::min(incoming.quantity, ask_order->order.quantity);
                std::cout << "TRADE: " << trade_qty << " @ " << best_ask_price << " (Buy " << incoming.order_id << " / Sell " << ask_order->order.order_id << ")\n";
                incoming.quantity -= trade_qty;
                ask_order->order.quantity -= trade_qty;

                if (ask_order->order.quantity == 0) {
                    order_lookup.erase(ask_order->order.order_id);
                    pool.deallocate(ask_order);
                    ask_list.erase(ask_list.begin());
                    for (size_t i = 0; i < ask_list.size(); ++i) {
                        order_lookup[ask_list[i]->order.order_id] = std::make_tuple(false, best_ask_price, i);
                    }
                } else {
                    break;
                }
            }
            if (ask_list.empty()) {
                asks.erase(best_ask_it);
            }
        }
        if (incoming.quantity > 0) {
            InternalOrder* internal_order = pool.allocate(incoming);
            if (!internal_order) {
                std::cerr << "Order pool exhausted!\n";
                return;
            }
            auto& price_map = bids;
            auto it = price_map.find(incoming.price);
            if (it == price_map.end()) {
                OrderList lst;
                lst.push_back(internal_order);
                price_map[incoming.price] = std::move(lst);
                order_lookup[incoming.order_id] = std::make_tuple(true, incoming.price, 0);
            } else {
                it->second.push_back(internal_order);
                order_lookup[incoming.order_id] = std::make_tuple(true, incoming.price, it->second.size() - 1);
            }
        }
    } else {
        while (incoming.quantity > 0 && !bids.empty()) {
            auto best_bid_it = bids.begin();
            double best_bid_price = best_bid_it->first;
            if (incoming.price > best_bid_price) break;

            OrderList& bid_list = best_bid_it->second;
            while (incoming.quantity > 0 && !bid_list.empty()) {
                InternalOrder* bid_order = bid_list.front();
                uint64_t trade_qty = std::min(incoming.quantity, bid_order->order.quantity);
                std::cout << "TRADE: " << trade_qty << " @ " << best_bid_price << " (Sell " << incoming.order_id << " / Buy " << bid_order->order.order_id << ")\n";
                incoming.quantity -= trade_qty;
                bid_order->order.quantity -= trade_qty;

                if (bid_order->order.quantity == 0) {
                    order_lookup.erase(bid_order->order.order_id);
                    pool.deallocate(bid_order);
                    bid_list.erase(bid_list.begin());
                    for (size_t i = 0; i < bid_list.size(); ++i) {
                        order_lookup[bid_list[i]->order.order_id] = std::make_tuple(true, best_bid_price, i);
                    }
                } else {
                    break;
                }
            }
            if (bid_list.empty()) {
                bids.erase(best_bid_it);
            }
        }
        if (incoming.quantity > 0) {
            InternalOrder* internal_order = pool.allocate(incoming);
            if (!internal_order) {
                std::cerr << "Order pool exhausted!\n";
                return;
            }
            auto& price_map = asks;
            auto it = price_map.find(incoming.price);
            if (it == price_map.end()) {
                OrderList lst;
                lst.push_back(internal_order);
                price_map[incoming.price] = std::move(lst);
                order_lookup[incoming.order_id] = std::make_tuple(false, incoming.price, 0);
            } else {
                it->second.push_back(internal_order);
                order_lookup[incoming.order_id] = std::make_tuple(false, incoming.price, it->second.size() - 1);
            }
        }
    }
}

bool OrderBook::cancel_order(uint64_t order_id) {
    auto it = order_lookup.find(order_id);
    if (it == order_lookup.end()) return false;
    bool is_buy;
    double price;
    size_t idx;
    std::tie(is_buy, price, idx) = it->second;

    if (is_buy) {
        auto price_level_it = bids.find(price);
        if (price_level_it != bids.end()) {
            auto& vec = price_level_it->second;
            if (idx < vec.size()) {
                pool.deallocate(vec[idx]);
                vec.erase(vec.begin() + idx);
                for (size_t i = idx; i < vec.size(); ++i) {
                    order_lookup[vec[i]->order.order_id] = std::make_tuple(true, price, i);
                }
            }
            if (vec.empty()) bids.erase(price_level_it);
        }
    } else {
        auto price_level_it = asks.find(price);
        if (price_level_it != asks.end()) {
            auto& vec = price_level_it->second;
            if (idx < vec.size()) {
                pool.deallocate(vec[idx]);
                vec.erase(vec.begin() + idx);
                for (size_t i = idx; i < vec.size(); ++i) {
                    order_lookup[vec[i]->order.order_id] = std::make_tuple(false, price, i);
                }
            }
            if (vec.empty()) asks.erase(price_level_it);
        }
    }
    order_lookup.erase(it);
    return true;
}

bool OrderBook::amend_order(uint64_t order_id, double new_price, uint64_t new_quantity) {
    auto it = order_lookup.find(order_id);
    if (it == order_lookup.end()) return false;
    bool is_buy;
    double price;
    size_t idx;
    std::tie(is_buy, price, idx) = it->second;

    // If price changes, treat as cancel + add
    if (new_price != price) {
        InternalOrder* old_order = nullptr;
        if (is_buy) {
            auto price_level_it = bids.find(price);
            if (price_level_it != bids.end() && idx < price_level_it->second.size()) {
                old_order = price_level_it->second[idx];
            }
        } else {
            auto price_level_it = asks.find(price);
            if (price_level_it != asks.end() && idx < price_level_it->second.size()) {
                old_order = price_level_it->second[idx];
            }
        }
        if (old_order) {
            Order new_order = old_order->order;
            new_order.price = new_price;
            new_order.quantity = new_quantity;
            cancel_order(order_id);
            add_order(new_order);
            return true;
        }
        return false;
    }

    // Only quantity changes
    InternalOrder* order_ptr = nullptr;
    if (is_buy) {
        auto price_level_it = bids.find(price);
        if (price_level_it != bids.end() && idx < price_level_it->second.size()) {
            order_ptr = price_level_it->second[idx];
        }
    } else {
        auto price_level_it = asks.find(price);
        if (price_level_it != asks.end() && idx < price_level_it->second.size()) {
            order_ptr = price_level_it->second[idx];
        }
    }
    if (order_ptr && order_ptr->order.quantity != new_quantity) {
        order_ptr->order.quantity = new_quantity;
    }
    return true;
}

void OrderBook::get_snapshot(size_t depth, std::vector<PriceLevel>& bids_out, std::vector<PriceLevel>& asks_out) const {
    bids_out.clear();
    asks_out.clear();

    size_t count = 0;
    for (const auto& [price, order_list] : bids) {
        if (count >= depth) break;
        uint64_t total_qty = 0;
        for (const auto& internal_order : order_list) {
            total_qty += internal_order->order.quantity;
        }
        if (total_qty > 0) {
            bids_out.push_back(PriceLevel{price, total_qty});
            ++count;
        }
    }

    count = 0;
    for (const auto& [price, order_list] : asks) {
        if (count >= depth) break;
        uint64_t total_qty = 0;
        for (const auto& internal_order : order_list) {
            total_qty += internal_order->order.quantity;
        }
        if (total_qty > 0) {
            asks_out.push_back(PriceLevel{price, total_qty});
            ++count;
        }
    }
}

void OrderBook::print_book(size_t depth) const {
    std::vector<PriceLevel> bids_vec, asks_vec;
    get_snapshot(depth, bids_vec, asks_vec);

    std::cout << "Order Book (Top " << depth << " levels):\n";
    std::cout << std::setw(16) << "BID_QTY" << " | "
              << std::setw(12) << "BID_PX" << " || "
              << std::setw(12) << "ASK_PX" << " | "
              << std::setw(16) << "ASK_QTY" << "\n";
    std::cout << std::string(16, '-') << "-+-"
              << std::string(12, '-') << "-++-"
              << std::string(12, '-') << "-+-"
              << std::string(16, '-') << "\n";

    for (size_t i = 0; i < depth; ++i) {
        std::string bid_qty = (i < bids_vec.size()) ? std::to_string(bids_vec[i].total_quantity) : "";
        std::string bid_px = (i < bids_vec.size()) ? std::to_string(bids_vec[i].price) : "";
        std::string ask_px = (i < asks_vec.size()) ? std::to_string(asks_vec[i].price) : "";
        std::string ask_qty = (i < asks_vec.size()) ? std::to_string(asks_vec[i].total_quantity) : "";

        std::cout << std::setw(16) << bid_qty << " | "
                  << std::setw(12) << bid_px << " || "
                  << std::setw(12) << ask_px << " | "
                  << std::setw(16) << ask_qty << "\n";
    }
}