#ifndef MSGNODE_H
#define MSGNODE_H

class LogicSystem;
class MsgNode {
public:
    MsgNode(short max_len);
    ~MsgNode();
    void Clear();
public:
    short _cur_len;
    short _total_len;
    char* _data;
};

class RecvNode : public MsgNode {
    friend class LogicSystem;
public:
    RecvNode(short max_len, short msg_id);
private:
    short _msg_id;
};

class SendNode : public MsgNode {
    friend class LogicSystem;
public:
    SendNode(const char* msg, short max_len, short msg_id);
private:
    short _msg_id;
};
#endif // MSGNODE_H