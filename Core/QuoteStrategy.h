#pragma once
#include <cstdint>

#include "Strategy.h"
#include "OrderManager.h"

/**
 * @brief Base class for strategies that operate on quote ticks (bid/ask) instead of trade ticks
 * 
 * Some strategies need to see the bid/ask spread (the difference between buy and sell prices)
 * rather than just the last trade price. This class provides an interface for such strategies.
 * 
 * QuoteStrategy inherits from Strategy but overrides onTick() to accept QuoteTick instead.
 * The regular onTick(const Tick&) is implemented as a dummy (does nothing) since
 * quote strategies don't use regular ticks.
 * 
 * Example use cases:
 * - Spread trading strategies (profit from bid-ask spread)
 * - Market making strategies (provide liquidity at bid/ask)
 * - Strategies that need to see order book depth
 */
class QuoteStrategy : public Strategy {
public:
	virtual ~QuoteStrategy() = default;
	virtual void setOrderManager(OrderManager* om) = 0;
	virtual void onStart() {}
	virtual void onEnd() {}

	/**
	 * @brief Dummy implementation - quote strategies don't use regular ticks
	 * 
	 * This is required because we inherit from Strategy, but quote strategies
	 * only care about QuoteTick, not regular Tick.
	 */
	void onTick(const Tick&) override {} // Dummy override
	
	/**
	 * @brief Called for each quote tick during backtesting
	 * 
	 * This is the main method that quote strategies implement.
	 * It receives bid/ask prices instead of just a single trade price.
	 * 
	 * @param tick The current quote tick with bid, ask, and volume
	 */
	virtual void onTick(const QuoteTick& tick) = 0;
};