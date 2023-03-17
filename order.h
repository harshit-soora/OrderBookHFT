#include <string>
#include <chrono>
#include <thread>
#include <iostream>

struct Logon
{
    std::string username;
    std::string password;
};

struct Logout
{
    std::string username;
};

struct OrderRequest
{
    int m_symbolId;
    double m_price;
    uint64_t m_qty;
    char m_side; // possible values 'B' or 'S'
    uint64_t m_orderId;

    OrderRequest(OrderRequest&& o) {
        m_symbolId = std::move(o.m_symbolId);
        m_price = std::move(o.m_price);
        m_qty = std::move(o.m_qty);
        m_side = std::move(o.m_side);
        m_orderId = std::move(o.m_orderId);
    }

    OrderRequest(int symbol, double price, uint64_t qty, char side, uint64_t orderId) {
        m_symbolId = symbol;
        m_price = price;
        m_qty = qty;
        m_side = side;
        m_orderId = orderId;
    }
};

enum class RequestType
{
    Unknown = 0,
    New = 1,
    Modify = 2,
    Cancel = 3
};

enum class ResponseType
{
    Unknown = 0,
    Accept = 1,
    Reject = 2,
};

struct OrderResponse
{
    uint64_t m_orderId;
    ResponseType m_responseType;

    OrderResponse(uint64_t id, int type)
    {
        m_orderId = id;
        if(type == 1)
        {
            m_responseType = ResponseType::Accept;
        }
        else if(type == 2)
        {
            m_responseType = ResponseType::Reject;
        }
        else
        {
            m_responseType = ResponseType::Unknown;
        }
    }
};

enum class orderStatus
{
    Buffering = 0,
    Exchange = 1,
};

typedef struct orderNode
{
    struct OrderRequest order;
    orderStatus status;
    orderNode* next;
    orderNode* prev;

    orderNode(OrderRequest&& req) : order(std::move(req))
    {
        status = orderStatus::Buffering;
        next = nullptr;
        prev = nullptr;
    }
}orderNode;