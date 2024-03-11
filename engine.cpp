#include <iostream>
#include <thread>
#include <memory>

#include "io.hpp"
#include "order_book.hpp"
#include "order.hpp"
#include "engine.hpp"

Engine::Engine() {}

void Engine::accept(ClientConnection connection)
{
	auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
	thread.detach();
}

void Engine::connection_thread(ClientConnection connection)
{
	while (true)
	{
		ClientCommand input{};
		switch (connection.readInput(input))
		{
		case ReadResult::Error:
			SyncCerr{} << "Error reading input" << std::endl;
		case ReadResult::EndOfFile:
			return;
		case ReadResult::Success:
			break;
		}

		// Functions for printing output actions in the prescribed format are
		// provided in the Output class:
		switch (input.type)
		{
		case input_cancel:
		{
			SyncCerr{} << "Got cancel: ID: " << input.order_id << std::endl;

			// Remember to take timestamp at the appropriate time, or compute
			// an appropriate timestamp!
			auto output_time = getCurrentTimestamp();
			Output::OrderDeleted(input.order_id, true, output_time);
			break;
		}

		default:
		{
			SyncCerr{}
					<< "Got order: " << static_cast<char>(input.type) << " " << input.instrument << " x " << input.count << " @ "
					<< input.price << " ID: " << input.order_id << std::endl;

			// Remember to take timestamp at the appropriate time, or compute
			// an appropriate timestamp!
			std::shared_ptr<Order> order = std::make_shared<Order>(input.order_id,
																														 input.instrument,
																														 input.price,
																														 input.count,
																														 input.type == input_sell ? Side::SELL : Side::BUY,
																														 0);
			std::shared_ptr<OrderBook> ob = GetOrderBook(order->GetInstrumentId());
			ob->HandleOrder(order);

			// auto output_time = getCurrentTimestamp();
			// Output::OrderAdded(input.order_id, input.instrument, input.price, input.count, input.type == input_sell,
			// 									 output_time);
			break;
		}
		}

		// Additionally:

		// Remember to take timestamp at the appropriate time, or compute
		// an appropriate timestamp!
		// intmax_t output_time = getCurrentTimestamp();

		// Check the parameter names in `io.hpp`.
		// Output::OrderExecuted(123, 124, 1, 2000, 10, output_time);
	}
}

std::shared_ptr<OrderBook> Engine::GetOrderBook(instrument_id_t instrument)
{
	return instruments.Get(instrument);
}