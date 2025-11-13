#pragma once

#include <algorithm>
#include <deque>
#include "Strategy.h"

/**
 * @brief Breakout trading strategy (template class)
 * 
 * This strategy implements a breakout/momentum approach:
 * - BUY when price breaks above the highest price in a rolling window (upward breakout)
 * - SELL when price breaks below the lowest price in a rolling window (downward breakout)
 * 
 * The idea is that when price breaks through a recent range (high or low),
 * it's likely to continue in that direction due to momentum.
 * 
 * Strategy logic:
 * 1. Maintain a rolling window of recent prices (WinSize ticks)
 * 2. Calculate the high and low of this window
 * 3. If not in position and price > window high -> BUY (upward breakout)
 * 4. If in position and price < window low -> SELL (downward breakout)
 * 
 * Template parameter:
 * - WinSize: Size of the rolling window (default: 20 ticks)
 *   Larger window = more conservative (waits for stronger breakouts)
 *   Smaller window = more aggressive (trades on smaller breakouts)
 * 
 * This is a template class, so the implementation is in the header file.
 * The .cpp file only includes the header.
 */
template<int WinSize = 20>
class BreakoutStrategy : public Strategy {
private:
	OrderManager* orderManager;        // Pointer to OrderManager for submitting orders
	double lastPrice = -1.0;          // Last seen price (not currently used, but kept for potential future use)
	std::deque<double> recentPrices;  // Rolling window of recent prices (FIFO queue)
	bool inPosition = false;          // Whether we currently hold a position
	double entryPrice = 0.0;          // Price at which we entered the current position
	
public:
	/**
	 * @brief Constructs the strategy with optional OrderManager
	 * 
	 * @param om OrderManager pointer (can be set later via setOrderManager)
	 */
	BreakoutStrategy(OrderManager* om = nullptr) : orderManager(om) {}

	/**
	 * @brief Sets the OrderManager for this strategy
	 * 
	 * Called by BacktestEngine before backtesting starts.
	 */
	void setOrderManager(OrderManager* om) override {
		orderManager = om;
	}

	/**
	 * @brief Main strategy logic - called for each market tick
	 * 
	 * Implements breakout detection:
	 * 1. Maintains a rolling window of WinSize recent prices
	 * 2. When window is full, calculates high and low
	 * 3. BUY signal: Price breaks above window high (upward momentum)
	 * 4. SELL signal: Price breaks below window low (downward momentum)
	 * 
	 * @param tick Current market tick
	 */
	void onTick(const Tick& tick) override {
		// Only trade if we have enough historical data (window is full)
		if (recentPrices.size() >= WinSize) {
			// Calculate the highest and lowest prices in the rolling window
			double high = *std::max_element(recentPrices.begin(), recentPrices.end());
			double low = *std::min_element(recentPrices.begin(), recentPrices.end());

			// BUY SIGNAL: Price breaks above the window high (upward breakout)
			// Only buy if we're not already in a position
			if (!inPosition && tick.price > high) {
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
			}
			// SELL SIGNAL: Price breaks below the window low (downward breakout)
			// Only sell if we're in a position
			else if (inPosition && tick.price < low) {
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
			}
		}

		// Update the rolling window: add current price, remove oldest if window is full
		recentPrices.push_back(tick.price);
		if (recentPrices.size() > WinSize) {
			recentPrices.pop_front();  // Remove oldest price to maintain window size
		}
	}
};