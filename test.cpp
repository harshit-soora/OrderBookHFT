/*
 * This class will generate test cases for the Order class
 * This will represent upstream client while generating orders randomly 
 * Orders will be send in burst to our order book which will then buffer 
 * the orders (if throttle) or send it over to the exchange
*/

#include "orderManagement.h"
#include <vector>

OrderManagement *m_obj;
std::vector<uint64_t> m_orderIdList;

void generateRandomOrder()
{
    int symbol = rand();
    double price = rand();
    uint64_t qty = rand() % 100;
    char side = 'B';
    if(rand() % 2 == 1)
        side = 'S';
    uint64_t orderId = m_orderIdList[rand()%1000]; // So we got chances to reuse the same orderid for modify/cancel

    int orderType = rand() % 3 + 1;
    OrderRequest req = OrderRequest(symbol, price, qty, side, orderId);
    if(orderType==1)
    {
        m_obj->onData(std::move(req), RequestType::New);
    }
    else if(orderType==2)
    {
        m_obj->onData(std::move(req), RequestType::Modify);
    }
    else if(orderType==3)
    {
        m_obj->onData(std::move(req), RequestType::Cancel);
    }
}

/*
 * The idea is send some order one-after other with while(true), will call these bursts
 * to send some order with a sleep interval of random 0-100ms delay
*/
void burstTest()
{
    int limitOrder = 1000;
    int burst = 10; // maximum bursts
    int count = 0;
    int flag = 0; // To switch between burst mode{1} and non burst mode{0}
    
    std::cout << "[Test] Start sending burst orders" << std::endl;
    while(limitOrder>0)
    {
        flag = rand() % 2;
        count = rand() % 100;

        if(flag == 1 && burst>0)
        {//Burst
            burst--;
            std::cout << "[Test] Burst round of " << count << " orders" << std::endl;
            while(count--)
            {
                generateRandomOrder();
                limitOrder--;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(count));
            generateRandomOrder();
            limitOrder--;
        }
    }
}

int main()
{
    m_obj = new OrderManagement();
    m_orderIdList.resize(1000);
    for(uint64_t i=0;i<1000;i++)
    {
        m_orderIdList[i] = i;
    }//We will send orderid from a pool of 1000 ids to do the burst test 
    burstTest();

    return 0;
}