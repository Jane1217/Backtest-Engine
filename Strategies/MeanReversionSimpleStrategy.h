#pragma once

#include "Strategy.h"
#include "OrderManager.h"

/**
 * @brief Simple mean reversion trading strategy
 * 
 * This strategy implements a basic mean reversion approach:
 * - BUY when price drops by 0.5% (expecting price to bounce back up)
 * - SELL when price rises by 0.5% from entry (taking profit)
 * 
 * The idea is that prices tend to revert to their mean - if price drops,
 * it's likely to bounce back, and vice versa. This is a contrarian strategy
 * that buys on dips and sells on rallies.
 * 
 * Strategy logic:
 * 1. Track the last seen price
 * 2. If not in position and price drops 0.5% from last price -> BUY
 * 3. If in position and price rises 0.5% from entry price -> SELL
 * 
 * This is a simple example strategy to demonstrate the framework.
 */
class MeanReversionSimple : public Strategy {
private:
	OrderManager* orderManager;  // Pointer to OrderManager for submitting orders
	double lastPrice = -1.0;     // Last seen price (initialized to -1 to indicate no price seen yet)
	bool inPosition = false;     // Whether we currently hold a position
	double entryPrice = 0.0;     // Price at which we entered the current position
	
public:
	/**
	 * @brief Constructs the strategy with optional OrderManager
	 * 
	 * @param om OrderManager pointer (can be set later via setOrderManager)
	 */
	MeanReversionSimple(OrderManager* om = nullptr) : orderManager(om) {}

	/**
	 * @brief Sets the OrderManager for this strategy
	 * 
	 * Called by BacktestEngine before backtesting starts.
	 */
	void setOrderManager(OrderManager* om) override;

	/**
	 * @brief Main strategy logic - called for each market tick
	 * 
	 * Analyzes the current tick and makes trading decisions:
	 * - If price dropped 0.5%, buy (expecting mean reversion)
	 * - If price rose 0.5% from entry, sell (take profit)
	 * 
	 * @param tick Current market tick
	 */
	void onTick(const Tick& tick) override;
};