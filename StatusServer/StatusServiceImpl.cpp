#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string generate_unique_string() {
    boost::uuids::uuid uuid =  boost::uuids::random_generator()();
    std::string unique_string = boost::uuids::to_string(uuid);
    return unique_string;
}


StatusServiceImpl::StatusServiceImpl() : _server_index(0) {
    std::shared_ptr<ConfigMgr> cfg = ConfigMgr::GetInstance();
    ChatServer server;
    server._host = (*cfg)["ChatServer1"]["Host"];
    server._port = (*cfg)["ChatServer1"]["Port"];
    _servers.push_back(server);

    server._host = (*cfg)["ChatServer2"]["Host"];
    server._port = (*cfg)["ChatServer2"]["Port"];
    _servers.push_back(server);
}

Status StatusServiceImpl::GetChatServer(ServerContext* context,
     const GetChatServerReq* request, GetChatServerRsp* response) {
    _server_index = (_server_index++) % (_servers.size());
    ChatServer& server = _servers[_server_index];

    response->set_host(server._host);
    response->set_port(server._port);
    response->set_error(ERROR_CODES::SUCCESS);
    response->set_token(generate_unique_string());
    return Status::OK;
}