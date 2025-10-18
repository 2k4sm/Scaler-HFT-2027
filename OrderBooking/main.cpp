#include "order_book.cpp"
#include <iostream>
#include <chrono>
#include <vector>

uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void print_snapshot(const OrderBook& book, size_t depth) {
    std::vector<PriceLevel> bids, asks;
    book.get_snapshot(depth, bids, asks);
    std::cout << "\nSnapshot (top " << depth << " levels):\n";
    std::cout << "Bids:\n";
    for (const auto& lvl : bids) {
        std::cout << "  " << lvl.price << " : " << lvl.total_quantity << "\n";
    }
    std::cout << "Asks:\n";
    for (const auto& lvl : asks) {
        std::cout << "  " << lvl.price << " : " << lvl.total_quantity << "\n";
    }
}

int main() {
    OrderBook book;

    book.add_order(Order{1, true, 100.0, 10, now_ns()});
    book.add_order(Order{2, true, 100.0, 20, now_ns()});
    book.add_order(Order{3, true, 101.0, 15, now_ns()});

    book.add_order(Order{4, false, 102.0, 5, now_ns()});
    book.add_order(Order{5, false, 102.0, 10, now_ns()});
    book.add_order(Order{6, false, 101.5, 12, now_ns()});

    std::cout << "\nInitial book:\n";
    book.print_book(5);

    std::cout << "\nAdd aggressive buy order (should match against lowest ask):\n";
    book.add_order(Order{7, true, 102.0, 8, now_ns()});
    book.print_book(5);

    std::cout << "\nCancel order 2 (buy 100.0):\n";
    book.cancel_order(2);
    book.print_book(5);

    std::cout << "\nAmend order 1 (buy 100.0) quantity to 5:\n";
    book.amend_order(1, 100.0, 5);
    book.print_book(5);

    std::cout << "\nAmend order 3 (buy 101.0) price to 101.5:\n";
    book.amend_order(3, 101.5, 15);
    book.print_book(5);

    print_snapshot(book, 3);

    std::cout << "\nCancel all orders at price 102.0 (asks):\n";
    book.cancel_order(4);
    book.cancel_order(5);
    book.print_book(5);

    std::cout << "\nAdd aggressive sell order (should match multiple bids):\n";
    book.add_order(Order{8, false, 100.0, 30, now_ns()});
    book.print_book(5);

    std::cout << "\nCancel non-existent order (should be false): ";
    std::cout << (book.cancel_order(999) ? "true" : "false") << "\n";

    std::cout << "Amend non-existent order (should be false): ";
    std::cout << (book.amend_order(999, 105.0, 10) ? "true" : "false") << "\n";

    std::cout << "\nAdd aggressive buy order (partial match, remainder added):\n";
    book.add_order(Order{9, true, 101.5, 20, now_ns()});
    book.print_book(5);

    std::cout << "\nAdd passive sell order (no match, just added):\n";
    book.add_order(Order{10, false, 105.0, 50, now_ns()});
    book.print_book(5);

    print_snapshot(book, 10);

    return 0;
}
