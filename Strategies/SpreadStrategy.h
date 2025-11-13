#pragma once

#include <iostream>
#include <mutex>
#include "QuoteStrategy.h"
#include "OrderManager.h"

extern std::mutex globalPrintMutex;

/**
 * @brief Market making strategy that profits from bid-ask spread
 * 
 * This strategy acts as a market maker by:
 * - Placing LIMIT buy orders slightly below the bid price
 * - Placing LIMIT sell orders slightly above the ask price
 * - Profiting from the spread when orders are filled
 * 
 * Strategy logic:
 * 1. Calculate the bid-ask spread
 * 2. Only trade if spread is large enough (>= minSpread)
 * 3. Place LIMIT buy order at (bid - offset) - trying to buy below market
 * 4. Place LIMIT sell order at (ask + offset) - trying to sell above market
 * 5. Maintain position limits to avoid excessive exposure
 * 
 * Parameters:
 * - orderSize: Volume for each order (default: 1.0)
 * - minSpread: Minimum spread required to trade (default: 0.01 = 1%)
 * - offset: Price offset from bid/ask (default: 0.005 = 0.5%)
 * 
 * Position limits:
 * - Won't buy if position >= 5.0 (avoid too much long exposure)
 * - Won't sell if position <= -5.0 (avoid too much short exposure)
 * 
 * This is a market making strategy that provides liquidity and profits
 * from the bid-ask spread, similar to how real market makers operate.
 */
class SpreadStrategy : public QuoteStrategy {
private:
	OrderManager* orderManager;  // Pointer to OrderManager for submitting orders
	double orderSize;            // Volume for each order
	double minSpread;            // Minimum spread required to trade (as fraction, e.g., 0.01 = 1%)
	double offset;               // Price offset from bid/ask (as fraction, e.g., 0.005 = 0.5%)
	
public:
	/**
	 * @brief Constructs the SpreadStrategy with configurable parameters
	 * 
	 * @param size Order volume (default: 1.0)
	 * @param minSpread Minimum spread to trade, as fraction (default: 0.01 = 1%)
	 * @param offset Price offset from bid/ask, as fraction (default: 0.005 = 0.5%)
	 */
	SpreadStrategy(double size = 1.0, double minSpread = 0.01, double offset = 0.005) 
		: orderSize(size), minSpread(minSpread), offset(offset) {}

	/**
	 * @brief Sets the OrderManager for this strategy
	 * 
	 * Called by BacktestEngine before backtesting starts.
	 */
	void setOrderManager(OrderManager* om) override;

	/**
	 * @brief Main strategy logic - called for each quote tick
	 * 
	 * Analyzes bid/ask prices and places LIMIT orders to profit from spread:
	 * - Places buy orders below bid (trying to buy cheap)
	 * - Places sell orders above ask (trying to sell expensive)
	 * - Only trades when spread is large enough
	 * 
	 * @param tick Current quote tick with bid/ask prices
	 */
	void onTick(const QuoteTick& tick) override;
};