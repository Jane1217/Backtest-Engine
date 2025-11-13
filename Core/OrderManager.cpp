#include "OrderManager.h"

/**
 * @brief Submits an order for execution
 * 
 * MARKET orders execute immediately at the current price.
 * LIMIT orders are queued and will execute when price conditions are met.
 */
void OrderManager::submit(Order& order) {
	if (order.type == OrderType::MARKET) {
		// Market orders execute immediately - no price checking needed
		execute(order);
	} else {
		// Limit orders wait in the pending queue until price conditions are met
		pendingOrders.push_back(order);
	}
}

/**
 * @brief Checks pending LIMIT orders against a regular tick
 * 
 * For each pending order, checks if the current tick price meets the execution condition:
 * - BUY limit: executes when market price drops to or below our limit (tick.price <= order.price)
 * - SELL limit: executes when market price rises to or above our limit (tick.price >= order.price)
 * 
 * Orders that don't execute remain in the pending queue.
 */
void OrderManager::handleTick(const Tick& tick) {
	std::vector<Order> stillPending;  // Orders that didn't execute this tick

	for (auto& order : pendingOrders) {
		bool executed = false;

		// BUY limit order: execute if price dropped to our buy level or below
		if (order.side == Order::Side::BUY && tick.price <= order.price) {
			execute(order);
			executed = true;
		}
		// SELL limit order: execute if price rose to our sell level or above
		else if (order.side == Order::Side::SELL && tick.price >= order.price) {
			execute(order);
			executed = true;
		}

		// Keep order in pending queue if it didn't execute
		if (!executed) stillPending.push_back(order);
	}

	// Replace pending orders with only those that didn't execute
	pendingOrders = std::move(stillPending);
}

/**
 * @brief Checks pending LIMIT orders against a quote tick (bid/ask prices)
 * 
 * Similar to handleTick but uses bid/ask prices from the order book:
 * - BUY limit: executes when our limit price >= ask price (we're willing to pay at least the ask)
 * - SELL limit: executes when our limit price <= bid price (we're willing to sell at most the bid)
 * 
 * This is more realistic because it uses actual order book prices rather than last trade price.
 */
void OrderManager::handleTick(const QuoteTick& quote) {
	std::vector<Order> stillPending;
	stillPending.reserve(pendingOrders.size());  // Pre-allocate for efficiency

	for (auto& order : pendingOrders) {
		bool executed = false;

		// BUY limit order: execute if we're willing to pay at least the ask price
		if (order.side == Order::Side::BUY && order.price >= quote.ask) {
			execute(order);
			executed = true;
		}
		// SELL limit order: execute if we're willing to sell at most the bid price
		else if (order.side == Order::Side::SELL && order.price <= quote.bid) {
			execute(order);
			executed = true;
		}

		// Keep order in pending queue if it didn't execute
		if (!executed) stillPending.push_back(order);
	}

	// Replace pending orders with only those that didn't execute
	pendingOrders = std::move(stillPending);
}

/**
 * @brief Executes an order and updates position and cash
 * 
 * When an order executes:
 * - BUY: We acquire shares (position increases), pay cash (cash decreases)
 * - SELL: We dispose of shares (position decreases), receive cash (cash increases)
 * 
 * Note: This doesn't check if we have enough cash or position to execute.
 * In a real system, you'd want to add validation here.
 */
void OrderManager::execute(Order& order) {
	if (order.side == Order::Side::BUY) {
		// Buying: increase position, decrease cash
		position += order.volume;
		cash -= order.volume * order.price;
	}
	else {
		// Selling: decrease position, increase cash
		position -= order.volume;
		cash += order.volume * order.price;
	}
}

/**
 * @brief Calculates total portfolio value (profit and loss)
 * 
 * PnL = cash + (position * current_price)
 * 
 * This represents what our portfolio would be worth if we closed all positions
 * at the current market price. It's the sum of:
 * - Cash we have on hand
 * - Value of our current position at the current price
 * 
 * @param lastPrice Current market price to value the position
 * @return Total portfolio value
 */
double OrderManager::getPnL(double lastPrice) const {
	return cash + position * lastPrice;
}

/**
 * @brief Returns the current position (number of shares owned)
 * 
 * @return Current position (positive = long position, negative = short position, 0 = flat)
 */
double OrderManager::getPosition() const {
	return position;
}
