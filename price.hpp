#ifndef PRICE_HPP
#define PRICE_HPP
#include <queue>
#include <memory>
#include "order.hpp"

class Price
{
public:
  Price(unsigned int price);
  bool AddOrder(std::shared_ptr<Order> order);
  std::shared_ptr<Order> PopOrder();
  size_t Size() const;
  unsigned int GetPrice() const;

private:
  std::queue<std::shared_ptr<Order>> orders;
  unsigned int price;
};

#endif