#include "exchange.cpp"
#include <map>
#include <mutex>

class OrderManagement
{
public:
    /*
    * This method receives order request from upstream system.
    * This method should finally call send method if the order request is eligible for sending to exchange.
    */
    void onData(OrderRequest&& request, RequestType type);
    void onData(OrderRequest&& request); // will suggest not to use this

    /*
    * This method will be called after the data is received from exchange and
    * converted to OrderResponse object. The response object needs to be processed.
    */
    void onData(OrderResponse&& response);

    /*
    * This method sends the request to exchange.
    * This method is not thread safe.
    */
    void send(const OrderRequest& request);

    /*
    * This method sends the logon message to exchange.
    * This method is not thread safe.
    */
    void sendLogon();

    /*
    * This method sends the logout message to exchange.
    * This method is not thread safe.
    */
    void sendLogout();

    // constructor destructor
    OrderManagement();
    ~OrderManagement();


private:
    void exchangeThread();
    void add(orderNode* root);
    void cancelRequest(orderNode* root);
    void cancelResponse(orderNode* root);

    orderNode* m_bufferQueueHead; // double linked list for order-buffer, act as a queue for the exchange
    orderNode* m_bufferQueueTail;
    std::map<uint64_t, orderNode*> m_orderHash; // help to identify if a order is inside the orderQueue, if exist then point to the root
    
    uint32_t m_threadDurationInMs;
    Exchange* m_exchangeObj;
    std::unique_ptr<std::thread> m_exchangeThread;
    std::mutex m_queueMutex;
    uint64_t m_bufferSize;
    uint64_t m_exchangeThrottle;
    bool m_bexitPending;
};