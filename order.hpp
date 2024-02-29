#ifndef ORDER_HPP
#define ORDER_HPP

#include <string>

typedef unsigned int order_id_t;
typedef std::string instrument_id_t;
typedef unsigned int price_t;

enum class Side
{
  BUY,
  SELL
};

class Order
{
public:
  Order(unsigned int order_id, std::string instrument, unsigned int price, unsigned int count, Side side, std::chrono::microseconds::rep timestamp);
  order_id_t GetOrderId() const
  {
    return order_id;
  }
  instrument_id_t GetInstrumentId() const
  {
    return instrument;
  }
  price_t GetPrice() const
  {
    return price;
  }
  unsigned int GetCount() const
  {
    return count;
  }
  Side GetSide() const
  {
    return side;
  }
  std::chrono::microseconds::rep GetTimestamp()
  {
    return timestamp;
  }

private:
  order_id_t order_id;
  instrument_id_t instrument;
  price_t price;
  unsigned int count;
  Side side; // TODO: enum
  std::chrono::microseconds::rep timestamp;
};

#endif