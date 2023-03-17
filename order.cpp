#include "order.h"

using namespace std;



struct Logon
{
    std::string username;
    std::string password;
}
struct Logout
{
    std::string username;
}
struct OrderRequest
{
    int m_symbolId;
    double m_price;
    uint64_t m_qty;
    char m_side; // possible values 'B' or 'S'
    uint64_t m_orderId;
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
};
class OrderManagement
{
public:
void onData(OrderRequest && request) {
// this method receives order request from upstream system.
// provide implementation for this method
// this method should finally call send method should the order
request is
// eligible for sending to exchange.
// you are free to decide which thread would execute this
function.
}
// onData method gets invoked when the exchange sends the
response back.
void onData(OrderResponse && response) {
// you have to provide implementation of this method.
// this method will be called after the data is received from
exchange and
// converted to OrderResponse object. The response object needs
to be
// processed.
// you are free to decide which thread would execute this
function.
}
// send methods sends the request to exchange.
void send(const OrderRequest& request) {
// you may assume that this method sends the request to
exchange.
// you DONT have to provide implementation for this method.
// this method is not thread safe.
// you are free to decide which thread would execute this
function.
}
void sendLogon() {
// you may assume that this method sends the logon message to
exchange.
// you DONT have to provide implementation for this method.
// this method is not thread safe.
}
void sendLogout() {
// you may assume that this method sends the logout message to
exchange.
// you DONT have to provide implementation for this method.
// this method is not thread safe.
}
// feel free to add any number of variables / functions
};