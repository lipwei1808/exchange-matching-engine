#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "engine.hpp"
#include "io.hpp"
#include "order.hpp"
#include "order_book.hpp"

void Engine::accept(ClientConnection connection)
{
    auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
    thread.detach();
}

void Engine::connection_thread(ClientConnection connection)
{
    std::unordered_map<int, std::shared_ptr<Order>> orders;
    while (true)
    {
        ClientCommand input{};
        switch (connection.readInput(input))
        {
            case ReadResult::Error:
                SyncCerr{} << "Error reading input" << std::endl;
            case ReadResult::EndOfFile:
                SyncCerr{} << "END OF FILE\n";
                return;
            case ReadResult::Success:
                break;
        }

        // Functions for printing output actions in the prescribed format are
        // provided in the Output class:
        switch (input.type)
        {
            case input_cancel: {
                SyncCerr{} << "Got cancel: ID: " << input.order_id << std::endl;

                // Remember to take timestamp at the appropriate time, or compute
                // an appropriate timestamp!
                if (orders.find(input.order_id) == orders.end())
                {
                    Output::OrderDeleted(input.order_id, false, getCurrentTimestamp());
                    break;
                }
                std::shared_ptr<Order> order = orders[input.order_id];
                std::shared_ptr<OrderBook> ob = GetOrderBook(order->GetInstrumentId());
                if (order->GetSide() == Side::BUY)
                    ob->Cancel<Side::BUY>(order);
                else
                    ob->Cancel<Side::SELL>(order);
                break;
            }

            default: {
                SyncCerr{} << "Got order: " << static_cast<char>(input.type) << " " << input.instrument << " x " << input.count << " @ "
                           << input.price << " ID: " << input.order_id << std::endl;
                // Remember to take timestamp at the appropriate time, or compute
                // an appropriate timestamp!
                std::shared_ptr<Order> order = std::make_shared<Order>(
                    input.order_id, input.instrument, input.price, input.count, input.type == input_sell ? Side::SELL : Side::BUY, 0);
                orders.insert({order->GetOrderId(), order});
                std::shared_ptr<OrderBook> ob = GetOrderBook(order->GetInstrumentId());
                if (order->GetSide() == Side::BUY)
                {
                    SyncInfo() << "WAITING TO ENTER ID: " << input.order_id << std::endl;
                    std::unique_lock<std::mutex> l(ob->buy);
                    SyncInfo() << "ENTERING ID: " << input.order_id << std::endl;
                    ob->Handle<Side::BUY>(order);
                }
                else
                {
                    SyncInfo() << "WAITING TO ENTER ID: " << input.order_id << std::endl;
                    std::unique_lock<std::mutex> l(ob->sell);
                    SyncInfo() << "ENTERING ID: " << input.order_id << std::endl;
                    ob->Handle<Side::SELL>(order);
                }
                break;
            }
        }
        SyncCerr() << "END OF INPUT\n";
    }
}

std::shared_ptr<OrderBook> Engine::GetOrderBook(instrument_id_t instrument)
{
    WrapperValue<std::shared_ptr<OrderBook>> & w = instruments.Get(instrument);
    std::unique_lock<std::mutex> l(w.lock);
    if (!w.initialised)
    {
        w.initialised = true;
        w.val = std::make_shared<OrderBook>();
    }

    return w.val;
}