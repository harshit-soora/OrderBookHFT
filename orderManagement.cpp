#include "orderManagement.h"


OrderManagement::OrderManagement() {
    const std::string fileName = "test_output.txt";
    m_log.open(fileName);

    m_bufferQueueHead = nullptr;
    m_bufferQueueTail = nullptr;

    Exchange::OnDataCallback callback = [this](OrderResponse&& response) {
        this->onData(std::move(response));
    };
    m_threadDurationInMs = 10000; // testing purpose; will run the system for 10 sec
    m_exchangeObj = new Exchange(callback);
    m_bufferSize = 0;
    m_exchangeThrottle = 100;
    m_bexitPending = true;

    try {
        m_exchangeThread.reset(new std::thread([this] {this->exchangeThread();}));
    }
    catch (const std::exception& e) {
        std::cout << "Error encountered while creating excahnge thread" << std::endl;
    }
}

void OrderManagement::add(orderNode* root) {
    if(m_bufferQueueTail && m_bufferQueueHead)
    {
        m_bufferQueueTail->next = root;
        root->prev = m_bufferQueueTail;
        m_bufferQueueTail = root;
    }
    else if(m_bufferQueueTail)
    {
        // special case, where we have consumed all the orders
        m_bufferQueueHead = root;
        m_bufferQueueTail->next = root;
        root->prev = m_bufferQueueTail;
        m_bufferQueueTail = root;
    }
    else
    {
        m_bufferQueueTail = root;
        m_bufferQueueHead = root;
    }
}

void OrderManagement::cancelRequest(orderNode* root) {
    orderNode* left = root->prev;
    orderNode* right = root->next;

    if(left && right)
    {
        left->next = right;
        right->prev = left;
    }
    else if(left)
    {
        m_bufferQueueTail = left;
        left->next = nullptr;
    }
    else if(right)
    {
        m_bufferQueueHead = right;
        right->prev = nullptr;
    }
    else
    {
        m_bufferQueueHead = nullptr;
        m_bufferQueueTail = nullptr;
    }

    delete root;
}

// Only difference here is we don't want to manipulate the head and tail pointers
void OrderManagement::cancelResponse(orderNode* root) {
    orderNode* left = root->prev;
    orderNode* right = root->next;

    if(left && right)
    {
        left->next = right;
        right->prev = left;
    }
    else if(left)
    {
        left->next = nullptr;
    }
    else if(right)
    {
        right->prev = nullptr;
    }

    delete root;
}

void OrderManagement::onData(OrderRequest&& request) {
    onData(std::move(request), RequestType::New);
}

void OrderManagement::onData(OrderRequest&& request, RequestType type) {
    if(type == RequestType::Unknown)
    {
        return; //Ignore and drop the request
    }

    std::lock_guard<std::mutex> autoLock(m_queueMutex);
    if(type == RequestType::New)
    {
        orderNode* curr = new orderNode(std::move(request));
        m_orderHash[request.m_orderId] = curr;
        m_bufferSize ++;
        add(curr);
    }
    else if(type == RequestType::Modify)
    {
        if(m_orderHash.count(request.m_orderId))
        {
            orderNode* curr = m_orderHash[request.m_orderId];

            // This order is already send to the exchange, no profit in modifing it now
            if(curr->status == orderStatus::Exchange)
                return;

            orderNode* left = curr->prev;
            orderNode* right = curr->next;
            orderNode* root = new orderNode(std::move(request));
            m_orderHash[request.m_orderId] = root;
            delete curr;
            root->prev = left;
            root->next = right;
        }
        else {
            // add as a new order
            orderNode* curr = new orderNode(std::move(request));
            m_orderHash[request.m_orderId] = curr;
            add(curr);
        }
    }
    else if(type == RequestType::Cancel)
    {
        if(m_orderHash.count(request.m_orderId))
        {
            orderNode* curr = m_orderHash[request.m_orderId];

            // This order is already send to the exchange, no profit in cancelling it now
            if(curr->status == orderStatus::Exchange)
                return;

            cancelRequest(curr);
            m_orderHash.erase(m_orderHash.find(request.m_orderId));
            if(m_bufferSize > 0) // may end up with a large number since it is unsigned
                m_bufferSize--;
        }
        // else drop the request as no order exist to be cancelled
    }
}

void OrderManagement::onData(OrderResponse&& response) {
    auto endTime = std::chrono::system_clock::now();

    // No need to acquire lock as these values are not edited with the client orders
    if(m_orderHash.count(response.m_orderId)) {
        orderNode* curr = m_orderHash[response.m_orderId];

        // If order is not send to the exchange, then drop the response
        if(curr->status == orderStatus::Buffering)
            return;

        if(response.m_responseType == ResponseType::Accept)
        {
            m_log << "Accepted order - ";
            m_log << response.m_orderId;
            m_log << " round trip delay (in ms) ";
            m_log << std::chrono::duration_cast<std::chrono::milliseconds>(endTime-curr->ts).count();
            m_log << "\n";
        }
        else if(response.m_responseType == ResponseType::Reject)
        {
            m_log << "Rejected order - ";
            m_log << response.m_orderId;
            m_log << " round trip delay (in ms) ";
            m_log << std::chrono::duration_cast<std::chrono::milliseconds>(endTime-curr->ts).count();
            m_log << "\n";
        }
        else
        {
            m_log << "Unknown response type for order - ";
            m_log << response.m_orderId;
            m_log << " round trip delay (in ms) ";
            m_log << std::chrono::duration_cast<std::chrono::milliseconds>(endTime-curr->ts).count();
            m_log << "\n";
        }

        cancelResponse(curr);
        m_orderHash.erase(m_orderHash.find(response.m_orderId));
    }
} 

void OrderManagement::send(const OrderRequest& request) {
    m_exchangeObj->processOrder(request);
}

void OrderManagement::exchangeThread()
{
    uint64_t orderExecuted = 0;
    auto startTime = std::chrono::system_clock::now();
    auto threadStartTime = startTime;
    auto endTime = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count();
    int printThrottle = 10;

    std::cout << "Exchange thread have started." << std::endl;
    while(m_bexitPending)
    {
        orderExecuted = 0;
        startTime = std::chrono::system_clock::now();

        while(orderExecuted < m_exchangeThrottle)
        {
            std::unique_lock<std::mutex> autoLock(m_queueMutex);
            endTime = std::chrono::system_clock::now();
            millis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count();

            if(millis >= 1000)
            {
                //As we went into next second ie. throttle resets
                orderExecuted = 0;
            }

            if(m_bufferSize > 0)
            {
                //Get head -> send it -> move head -> decrease counter
                if(m_bufferQueueHead!=nullptr)
                {
                    m_bufferQueueHead->status = orderStatus::Exchange;
                    send(m_bufferQueueHead->order);
                    m_bufferQueueHead = m_bufferQueueHead->next; //One issue, head now may point null, will handle in onData()
                    m_bufferSize--;
                    orderExecuted++;
                }
                else {
                    if(printThrottle) {
                        printThrottle--;
                        std::cout << "Extreme case of head null while buffer size is " << m_bufferSize << std::endl;
                    }
                    
                    orderNode* last = m_bufferQueueTail;
                    m_bufferQueueHead = m_bufferQueueTail;
                    m_bufferSize = 0;
                    while(m_bufferQueueHead != nullptr)
                    {
                        if(m_bufferQueueHead->status==orderStatus::Exchange)
                            break;
                        m_bufferSize ++;

                        last = m_bufferQueueHead;
                        m_bufferQueueHead = m_bufferQueueHead->prev;
                    }
                    m_bufferQueueHead = last;

                    std::cout << "Head restored, new buffer size " << m_bufferSize << std::endl;
                }
            }

            autoLock.unlock();

            endTime = std::chrono::system_clock::now();
            millis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime-threadStartTime).count();

            if(millis > m_threadDurationInMs)
            {
                std::cout << "Its time to exit the exchange thread" << std::endl;
                m_bexitPending = false;
                break;
            }
        }

        // If we exit early from second complete, then will wait till the current second elapse
        endTime = std::chrono::system_clock::now();
        millis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count();

        if(millis%1000 != 0) {
            std::cout << "Waiting for " << 1000- millis%1000 << " ms before the throttle reset from next second" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 - millis % 1000));
        }
    }
    std::cout << "Exchange thread have exited. Open test_output to see the round trip delay" << std::endl;
}

void OrderManagement::sendLogon() {
    
}

void OrderManagement::sendLogout() {

}

OrderManagement::~OrderManagement() {
    m_bexitPending = false;
    if(m_exchangeThread) {
        m_exchangeThread->join();
        m_exchangeThread.reset();
    }

    orderNode* temp;
    while(m_bufferQueueHead != nullptr)
    {
        temp = m_bufferQueueHead;
        m_bufferQueueHead = m_bufferQueueHead->next;
        delete temp;
    }
    m_bufferQueueHead = nullptr;
    m_bufferQueueTail = nullptr;

    delete [] m_exchangeObj;
    m_exchangeObj = nullptr;
    m_bufferSize = 0;
    m_exchangeThrottle = 100;

    m_orderHash.clear();
    m_log.close();
}