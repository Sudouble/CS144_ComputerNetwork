#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    ST_ROUTE_TABLE stTable;
    stTable.route_prefix = route_prefix;
    stTable.prefix_length = prefix_length;
    stTable.next_hop = next_hop;
    stTable.inferface_num = interface_num;

    _route_table.push_back(stTable);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // DUMMY_CODE(dgram);
    // Your code here.
    IPv4Header& header = dgram.header();    
    if (header.ttl == 0 || header.ttl == 1)
        return; // drop

    // longest frefix match check
    size_t max_result_index = 0;
    int8_t max_prefix = -1;
    for (size_t i = 0; i < _route_table.size(); ++i)
    {
        ST_ROUTE_TABLE& table = _route_table.at(i);
        int8_t cal_result = calc_longest_match(table.route_prefix, table.prefix_length, header.dst);
        if (cal_result > max_prefix)
        {
            max_prefix = cal_result;
            max_result_index = i;
        }
    }
    --header.ttl;
    ST_ROUTE_TABLE& table = _route_table.at(max_result_index);
    Address addrSend = table.next_hop.has_value() ? table.next_hop.value() :  Address::from_ipv4_numeric(header.dst);    
    interface(table.inferface_num).send_datagram(dgram, addrSend);
}

int8_t Router::calc_longest_match(const uint32_t route_prefix, const uint8_t prefix_length, const uint32_t dst)
{
    uint8_t _count_prefix = 0;    
    for (uint8_t i = 0; i < prefix_length; i++)
    {
        uint8_t src = (route_prefix >> (31 - i)) & 0x01;
        uint8_t dest = (dst >> (31 - i)) & 0x01;
        if (src != dest)
            break;
        ++_count_prefix;
    }
    return prefix_length == _count_prefix ? _count_prefix : 0;
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
