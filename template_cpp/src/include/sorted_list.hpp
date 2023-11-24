#pragma once

#include "message.hpp"
#include <list>
#include <optional>

class SortedList {
    std::list<BroadcastMessage> list;

public:
    void insert(BroadcastMessage m) {
        auto begin = list.begin();

        while (begin != list.end() &&
               m.get_sequence_number() > begin->get_sequence_number()) {
            begin++;
        }

        list.insert(begin, m);
    }

    std::optional<BroadcastMessage &> front() {
        return list.empty()
                   ? std::nullopt
                   : std::make_optional<BroadcastMessage &>(list.front());
    }

    BroadcastMessage pop_front() {
        BroadcastMessage bm = std::move(list.front());
        list.pop_front();

        return bm;
    }
};