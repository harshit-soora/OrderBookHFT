/*
 * This class will be a dummy upstream exchange which will
 * randomly match some orders while rejecting others
 * This will be a helper class to "Test" so that we can do end-to-end testing
*/

#include "order.h"
#include <time.h>
#include <functional>

class Exchange
{
    public:
        using OnDataCallback = std::function<void(OrderResponse&& response)>;

        Exchange(OnDataCallback callback)
        {
            srand(time(0));
            m_callback = callback;
            std::cout << "[Exchange] Exchange have been setup, will start sending the response" << std::endl;
        }

        void processOrder(const OrderRequest& request)
        {
            int type = rand() % 3;

            if(type == 0)
            {
                type = rand() % 3; // Making chances of getting Unknown less ie. 1/9
            }
            OrderResponse res(request.m_orderId, type);

            if(!printFirst)
            {
                printFirst = true;
                std::cout << "[Exchange] Sending the response for first order, id- " << request.m_orderId<< std::endl;
            }
            m_callback(std::move(res));
        }

    private:
        OnDataCallback m_callback;
        bool printFirst = false;
};
