#include "MsgNode.h"
#include <iostream>
#include <string.h>
#include "const.h"
#include <boost/asio.hpp>

MsgNode::MsgNode(short max_len)
     : _total_len(max_len), _cur_len(0) {
    // ()会对申请的内存进行初始化
    _data = new char[_total_len + 1]();
    _data[_total_len] = '\0';
}

MsgNode::~MsgNode() {
    std::cout << "~MsgNode() destruct." << std::endl;
    delete[] _data;
}
void MsgNode::Clear() {
    ::memset(_data, 0, _total_len);
    _cur_len = 0;
}

RecvNode::RecvNode(short max_len, short msg_id)
     : MsgNode(max_len), _msg_id(msg_id) {

}

SendNode::SendNode(const char* msg, short max_len, short msg_id)
     : MsgNode(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id) {
    short network_msgid = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data, &network_msgid, HEAD_ID_LENGTH);
    short network_msglen = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data + HEAD_ID_LENGTH, &network_msglen, HEAD_DATA_LENGTH);
    memcpy(_data + HEAD_TOTAL_LEN, msg, max_len);
}