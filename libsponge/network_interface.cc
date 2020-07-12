#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    // check map table
    // if has to queue directly
    bool _can_find_hop_ip = _mapIP2Info.find(next_hop_ip) != _mapIP2Info.end();
    if (_can_find_hop_ip && _mapIP2Info[next_hop_ip]._Is_EA_valid)
    {
        EthernetFrame _frame_ethernet;

        EthernetHeader& hdr = _frame_ethernet.header();
        hdr.src = _ethernet_address;
        hdr.dst = _mapIP2Info[next_hop_ip]._ethernet_address;        
        hdr.type = EthernetHeader::TYPE_IPv4;

        _frame_ethernet.payload() = dgram.payload();

        _frames_out.push(_frame_ethernet);
    }
    else
    {
        if (false == _can_find_hop_ip)
        {
            ARP_INFO arp_info;
            _mapIP2Info.insert(make_pair(next_hop_ip, arp_info));
        }

        if (_mapIP2Info[next_hop_ip]._arp_msg_sent
            && _mapIP2Info[next_hop_ip]._passed_send_time <= 5000)
        {
            // wait 5000ms, don't overflow the network
        }
        else
        {            
            ARPMessage _ARP_msg;
            _ARP_msg.sender_ethernet_address = _ethernet_address;        
            _ARP_msg.sender_ip_address = _ip_address.ipv4_numeric();
            // _ARP_msg.target_ethernet_address = ;
            _ARP_msg.target_ip_address = next_hop_ip;
            _ARP_msg.opcode = ARPMessage::OPCODE_REQUEST;

            // cerr << "==============try to arp IP:"  << _ARP_msg.to_string() << endl;

            EthernetFrame _frame_ethernet;
            EthernetHeader& hdr = _frame_ethernet.header();
            hdr.src = _ethernet_address;
            hdr.dst = ETHERNET_BROADCAST;
            hdr.type = EthernetHeader::TYPE_ARP;
            _frame_ethernet.payload().append(Buffer(_ARP_msg.serialize()));

            _frames_out.push(_frame_ethernet);
        }        

        // push the unkown to queue;
        Stored_EthernetFrame stStore;
        //stStore.dgram = std::move(dgram);
        stStore.buff = Buffer(std::move(dgram.serialize().concatenate()));
        stStore.next_hop = next_hop;

        {
            EthernetFrame _frame_ethernet;

            EthernetHeader& hdr = _frame_ethernet.header();
            hdr.src = _ethernet_address;
            hdr.type = EthernetHeader::TYPE_IPv4;

            _frame_ethernet.payload() = dgram.serialize();
            _frames_before_ARP.push(_frame_ethernet);
        }
    }
    // if not exist send ARP msg
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);

    const EthernetHeader& hdr = frame.header();
    Buffer buff(std::move(frame.payload().concatenate()));

    if (hdr.type == EthernetHeader::TYPE_IPv4)
    {
        InternetDatagram frame_parse;
        if (frame_parse.parse(buff) != ParseResult::NoError)
            return {};
        
        return frame_parse;
    }
    else if (hdr.type == EthernetHeader::TYPE_ARP)
    {        
        // if recv arp request
        ARPMessage arpMsg;
        if (arpMsg.parse(buff) != ParseResult::NoError)
            return {};

        // cerr << "==========here comes the APR." 
        //      << "\t buffer queue size:" << _frames_before_ARP.size() 
        //      << "\t ARP:" << arpMsg.to_string() << endl;

        if (arpMsg.opcode == ARPMessage::OPCODE_REPLY)
        {
            uint32_t next_hop_ip = arpMsg.sender_ip_address;
            bool _can_find_hop_ip = _mapIP2Info.find(next_hop_ip) != _mapIP2Info.end();

            if (not _can_find_hop_ip)
                return {};

            ARP_INFO& arpInfo = _mapIP2Info[next_hop_ip];
            arpInfo._Is_EA_valid = true;
            arpInfo.EA_alive_time = 0;
            arpInfo._ethernet_address = arpMsg.sender_ethernet_address;

            // sender buff in queue
            while (not _frames_before_ARP.empty())
            {
                EthernetFrame& _frame_ethernet = _frames_before_ARP.front();
                EthernetHeader& hdrOut = _frame_ethernet.header();
                hdrOut.src = _ethernet_address;
                hdrOut.dst = arpMsg.sender_ethernet_address;

                // send and pop this one;
                _frames_out.push(_frame_ethernet);
                _frames_before_ARP.pop();
            }

        }
        else if (arpMsg.opcode == ARPMessage::OPCODE_REQUEST)
        {
            // RFC1982, if request is coming and remote don't have, restore the sender IP.
            uint32_t sender_ip = arpMsg.sender_ip_address;

            ARP_INFO arpInfo;
            arpInfo._Is_EA_valid = true;
            arpInfo.EA_alive_time = 0;
            arpInfo._ethernet_address = arpMsg.sender_ethernet_address;

            _mapIP2Info[sender_ip] = arpInfo;

            // =========================================================
            // send reply
            ARPMessage _ARP_msg;
            _ARP_msg.sender_ethernet_address = _ethernet_address;        
            _ARP_msg.sender_ip_address = _ip_address.ipv4_numeric();
            _ARP_msg.target_ethernet_address = arpMsg.sender_ethernet_address;
            _ARP_msg.target_ip_address = arpMsg.sender_ip_address;
            
            _ARP_msg.target_ip_address = _ip_address.ipv4_numeric();
            _ARP_msg.opcode = ARPMessage::OPCODE_REPLY;

            EthernetFrame _frame_ethernet;
            EthernetHeader& hdrOut = _frame_ethernet.header();
            hdrOut.src = _ethernet_address;
            hdrOut.dst = _ARP_msg.sender_ethernet_address;
            hdrOut.type = EthernetHeader::TYPE_ARP;
            _frame_ethernet.payload().append(Buffer(_ARP_msg.serialize()));

            _frames_out.push(_frame_ethernet);
            // =========================================================
        }
        
    }

    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // 5 second tick of arp
    if (_mapIP2Info.size() == 0)
        return;

    auto it = _mapIP2Info.begin();
    while (it != _mapIP2Info.end())
    {
        // check timer;        
        ARP_INFO &arpInfo = it->second;
        if (arpInfo._arp_msg_sent && not arpInfo._Is_EA_valid)
        {
            arpInfo._passed_send_time += ms_since_last_tick;
        }
        else if (arpInfo._Is_EA_valid && not arpInfo._arp_msg_sent)
        {
            arpInfo.EA_alive_time += ms_since_last_tick;
            if (arpInfo.EA_alive_time >= ARP_EA_VALID_TIME)
            {
                arpInfo._Is_EA_valid = false;
            }
        }
        it++;
    }
}
