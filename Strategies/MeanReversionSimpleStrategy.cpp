#include "MeanReversionSimpleStrategy.h"

/**
 * @brief Sets the OrderManager for submitting orders
 */
void MeanReversionSimple::setOrderManager(OrderManager* om) {
	orderManager = om;
}

/**
 * @brief Main strategy logic - implements mean reversion trading
 * 
 * Strategy rules:
 * 1. Initialize: On first tick, just store the price
 * 2. Buy signal: If not in position and price dropped 0.5% (tick.price < lastPrice * 0.995)
 *    - Submit a MARKET buy order for 1 share
 *    - Record entry price and set inPosition = true
 * 3. Sell signal: If in position and price rose 0.5% from entry (tick.price > entryPrice * 1.005)
 *    - Submit a MARKET sell order for 1 share
 *    - Set inPosition = false
 * 4. Update: Always update lastPrice to current price
 * 
 * The 0.5% thresholds (0.995 and 1.005) are hardcoded for simplicity.
 * In a real strategy, these would be configurable parameters.
 */
void MeanReversionSimple::onTick(const Tick& tick) {
	// First tick: just initialize lastPrice, no trading decision yet
	if (lastPrice < 0) {
		lastPrice = tick.price;
		return;
	}

	// BUY SIGNAL: Price dropped 0.5% from last price (mean reversion opportunity)
	// Only buy if we're not already in a position
	if (!inPosition && tick.price < lastPrice * 0.995) {
		// Create a MARKET buy order for 1 share at current price
		Order buy = { 
			Order::Side::BUY,      // Buy side
			OrderType::MARKET,      // Market order (executes immediately)
			tick.timestamp,         // Current timestamp
			1.0,                    // Volume: 1 share
			tick.price              // Price (for MARKET orders, this is the execution price)
		};
		orderManager->submit(buy);
		
		// Record entry price and mark that we're in a position
		entryPrice = tick.price;
		inPosition = true;
		
		// Thread-safe console output
		{
			std::lock_guard<std::mutex> lock(globalPrintMutex);
			std::cout << "[MEAN REVERSION BUY] @ " << tick.price << "\n";
		}
	}
	// SELL SIGNAL: Price rose 0.5% from entry price (take profit)
	// Only sell if we're in a position
	else if (inPosition && tick.price > entryPrice * 1.005) {
		// Create a MARKET sell order for 1 share at current price
		Order sell = { 
			Order::Side::SELL,     // Sell side
			OrderType::MARKET,      // Market order (executes immediately)
			tick.timestamp,         // Current timestamp
			1.0,                    // Volume: 1 share
			tick.price              // Price (for MARKET orders, this is the execution price)
		};
		orderManager->submit(sell);
		
		// Mark that we're no longer in a position
		inPosition = false;
		
		// Thread-safe console output
		{
			std::lock_guard<std::mutex> lock(globalPrintMutex);
			std::cout << "[MEAN REVERSION SELL] @ " << tick.price << "\n";
		}
	}

	// Always update lastPrice for next tick comparison
	lastPrice = tick.price;
}
