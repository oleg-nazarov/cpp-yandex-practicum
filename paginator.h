#pragma once
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

template <typename Iterator>
class IteratorRange {
   public:
    IteratorRange() = default;

    IteratorRange(Iterator range_begin, Iterator range_end) : range_begin_(range_begin), range_end_(range_end) {}

    Iterator begin() const {
        return range_begin_;
    }

    Iterator end() const {
        return range_end_;
    }

   private:
    Iterator range_begin_;
    Iterator range_end_;
};

template <typename Iterator>
class Paginator {
   public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
        // all pages that are fullfilled according to "page_size"
        while (std::next(range_begin, page_size) <= range_end) {
            auto page_end = std::next(range_begin, page_size);

            page_iterators_.push_back(IteratorRange<Iterator>(range_begin, page_end));
            range_begin = page_end;
        }

        // last page that is not fullfilled according to "page_size"
        if (range_begin < range_end) {
            page_iterators_.push_back({range_begin, range_end});
        }
    }

    auto begin() const {
        return page_iterators_.begin();
    }

    auto end() const {
        return page_iterators_.end();
    }

   private:
    std::vector<IteratorRange<Iterator>> page_iterators_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& it) {
    for (Iterator iterator = it.begin(); iterator != it.end(); ++iterator) {
        os << *iterator;
    }

    return os;
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const std::vector<IteratorRange<Iterator>>& page_iterators) {
    for (IteratorRange<Iterator> it : page_iterators) {
        os << it;
    }

    return os;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}
