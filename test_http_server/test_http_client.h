#pragma once
#include <string>
#include <future>

struct event_base;

struct RequestContext
{
    event_base* base;
    std::promise<std::string> prom_rsp;
};

int TestPostHttp(const std::string& http_url, std::string& data, RequestContext& req_ctx);
int TestGetHttp(const std::string& http_url, RequestContext& req_ctx);
