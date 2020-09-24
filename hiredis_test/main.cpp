#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <hiredis/hiredis.h>
#include "../utility.hpp"

std::string parse_reply(redisReply* p_reply, std::vector<std::string>& vec_result) {
    std::vector<std::string>().swap(vec_result);
    std::ostringstream ostr;
    switch (p_reply->type) {
        case REDIS_REPLY_STRING:
            printf("返回值为STRING:%s\n", p_reply->str);
            vec_result.emplace_back(p_reply->str);
            break;
        case REDIS_REPLY_ARRAY:
            printf("返回值为数组, 数组大小:%d\n", p_reply->elements);
            for (int i = 0; i < p_reply->elements; ++i) {
                vec_result.emplace_back(p_reply->element[i]->str);
            }
            break;
        case REDIS_REPLY_INTEGER:
            printf("返回值为INTEGER:%d\n", p_reply->integer);
            ostr << p_reply->integer;
            vec_result.emplace_back(ostr.str());
            break;
        case REDIS_REPLY_NIL:
            ostr << "返回值为空";
            return ostr.str();
        case REDIS_REPLY_STATUS:
            printf("返回状态值: %s\n", p_reply->str);
            vec_result.emplace_back(p_reply->str);
            break;
        case REDIS_REPLY_ERROR:
            ostr << "执行出错:" << p_reply->str;
            return ostr.str();
        default:
            return "未知的redisReply类型";
            break;
    }
    return std::string();
}

void incr_val() {
    redisContext* p_redis_ctx = nullptr;
    std::string ip = "127.0.0.1";
    int port = 6379;
    p_redis_ctx = redisConnect(ip.c_str(), port);
    if (p_redis_ctx == NULL || p_redis_ctx->err) {
        if (p_redis_ctx) {
            printf("Error: %s\n", p_redis_ctx->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
    std::vector<std::string> vec_result;
    for (int i = 0; i < 10; ++i) {
        void* p_reply = redisCommand(p_redis_ctx, "incr key_test");
        std::string err_msg = parse_reply(static_cast<redisReply*>(p_reply), vec_result);
        if (err_msg.empty()) {
            printf("执行结果:\n");
            for (auto& res : vec_result) {
                std::cout << res << std::endl;
            }
        } else {
            printf("执行出错: %s\n", err_msg.c_str());
        }
        freeReplyObject(p_reply);
    }
    redisFree(p_redis_ctx);
}

int add(int a, int b) {
    return a + b;
}

int main()
{
    std::vector<std::thread> threads(5);
    for (int i = 0; i< 5; ++i) {
        threads[i] = std::thread(incr_val);
    }
    for (int i = 0; i< 5; ++i) {
        threads[i].join();
    }
    return 0;
}

