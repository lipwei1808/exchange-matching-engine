#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <assert.h>

#include "order.hpp"
#include "order_book.hpp"

template void OrderBook::Handle<Side::BUY>(std::shared_ptr<Order> order);
template void OrderBook::Handle<Side::SELL>(std::shared_ptr<Order> order);
template void OrderBook::Add<Side::SELL>(std::shared_ptr<Order> order);
template void OrderBook::Add<Side::BUY>(std::shared_ptr<Order> order);
template bool OrderBook::Execute<Side::SELL>(std::shared_ptr<Order> order);
template bool OrderBook::Execute<Side::BUY>(std::shared_ptr<Order> order);
template std::shared_ptr<Price> OrderBook::GetPrice<Side::SELL>(price_t price);
template std::shared_ptr<Price> OrderBook::GetPrice<Side::BUY>(price_t price);

template <Side side>
void OrderBook::Handle(std::shared_ptr<Order> order)
{
    assert(side == Side::SELL || side == Side::BUY);
    assert(order->GetSide() == side);
    assert(order->GetActivated() == false);
    std::unique_lock<std::mutex> l(order_book_lock);

    // Get timestamp for order
    order->SetTimestamp(getCurrentTimestamp());

    // Insert dummy node into order
    if constexpr (side == Side::BUY)
        Add<Side::BUY>(order);
    else
        Add<Side::SELL>(order);

    l.unlock();

    // Execute
    bool filled = Execute<side>(order);
    if (!filled)
    {
        Output::OrderAdded(
            order->GetOrderId(),
            order->GetInstrumentId().c_str(),
            order->GetPrice(),
            order->GetCount(),
            side == Side::SELL,
            order->GetTimestamp());
    }

    // Add
    order->Activate();
}

template <Side side>
void OrderBook::Add(std::shared_ptr<Order> order)
{
    assert(order->GetSide() == side);
    if constexpr (side == Side::BUY)
        std::unique_lock<std::mutex> l(bids_lock);
    else
        std::unique_lock<std::mutex> l(asks_lock);
    std::shared_ptr<Price> p = GetPrice<side>(order->GetPrice());
    p->push_back(order);
}

/**
 * @param order active buy order to execute.
 * @return if buy order has been fully completed.
 */
template <Side side>
bool OrderBook::Execute(std::shared_ptr<Order> order)
{
    assert(side == Side::SELL || side == Side::BUY);
    assert(order->GetSide() == side);
    assert(order->GetActivated() == false);
    std::unique_lock<std::mutex> l;
    if constexpr (side == Side::BUY)
        l = std::unique_lock<std::mutex>(asks_lock);
    else
        l = std::unique_lock<std::mutex>(bids_lock);

    while (order->GetCount() > 0)
    {
        // Check if any sell orders
        if constexpr (side == Side::BUY)
        {
            if (asks.Size() == 0)
                return false;
        }
        else
        {
            if (bids.Size() == 0)
                return false;
        }

        // Check if lowest sell order can match the buy
        auto firstEl = ([&]() {
          if constexpr (side == Side::BUY) 
            return asks.begin();
          else 
            return bids.begin(); 
        })();
        price_t price = firstEl->first;
        assert(firstEl->second.initialised);
        std::shared_ptr<Price> priceQueue = firstEl->second.Get();
        assert(priceQueue->size() != 0);

        if constexpr (side == Side::BUY)
        {
            if (price > order->GetPrice())
                return false;
        }
        else
        {
            if (price < order->GetPrice())
                return false;
        }

        // Iteratively match with all orders in this price queue.
        while (order->GetCount() > 0 && priceQueue->size())
        {
            std::shared_ptr<Order> oppOrder = priceQueue->front();
            // Check if first order's timestamp comes before the current buy
            if (oppOrder->GetTimestamp() > order->GetTimestamp())
                break;

            // Check if sell order is activated
            while (!oppOrder->GetActivated())
                oppOrder->cv.wait(l);

            // Check if dummy order has already been filled
            if (oppOrder->GetCount() == 0)
            {
                priceQueue->pop_front();
                continue;
            }
            oppOrder->IncrementExecutionId();
            MatchOrders(order, oppOrder);
            if (oppOrder->GetCount() == 0)
                priceQueue->pop_front();
        }

        if (priceQueue->size() == 0)
        {
            // Remove from map of prices
            size_t num = ([&]() -> size_t
                    {
          if constexpr (side == Side::BUY) 
            return asks.Erase(firstEl->first);
          else 
            return bids.Erase(firstEl->first); })();
            assert(num == 1);
        }
    }
    return order->GetCount() == 0;
}

void OrderBook::MatchOrders(std::shared_ptr<Order> incoming, std::shared_ptr<Order> resting)
{
    unsigned int qty = std::min(incoming->GetCount(), resting->GetCount());
    if (resting->GetCount() == incoming->GetCount())
    {
        incoming->Fill();
        resting->Fill();
    }
    else if (resting->GetCount() > incoming->GetCount())
    {
        resting->Fill(incoming->GetCount());
        incoming->Fill();
    }
    else
    {
        incoming->Fill(resting->GetCount());
        resting->Fill();
    }
    Output::OrderExecuted(
        resting->GetOrderId(), incoming->GetOrderId(), resting->GetExecutionId(), resting->GetPrice(), qty, getCurrentTimestamp());
}

template <Side side>
std::shared_ptr<Price> OrderBook::GetPrice(price_t price)
{
    WrapperValue<std::shared_ptr<Price>> &w = ([&]() {
      if constexpr (side == Side::BUY)
        return std::ref(bids.Get(price));
      else
        return std::ref(asks.Get(price)); 
    })();

    std::unique_lock<std::mutex> l(w.lock);
    if (!w.initialised)
    {
        w.initialised = true;
        w.val = std::make_shared<Price>();
    }
    return w.val;
}