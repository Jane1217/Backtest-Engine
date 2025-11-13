#pragma once

#include "OrderManager.h"
#include <cstdint>

/**
 * @brief Abstract base class for all trading strategies
 * 
 * This is the core interface that all trading strategies must implement.
 * A strategy receives market data (ticks) and makes trading decisions
 * by submitting orders through the OrderManager.
 * 
 * The lifecycle of a strategy:
 * 1. setOrderManager() - Strategy is given access to order execution
 * 2. onStart() - Called once before backtesting begins (optional override)
 * 3. onTick() - Called for each market tick (required implementation)
 * 4. onEnd() - Called once after backtesting completes (optional override)
 * 
 * To create a new strategy, inherit from this class and implement:
 * - setOrderManager(): Store the OrderManager pointer for later use
 * - onTick(): Your trading logic that analyzes ticks and submits orders
 */
class Strategy {
public:
	virtual ~Strategy() = default;  // Virtual destructor for proper cleanup of derived classes
	
	/**
	 * @brief Sets the OrderManager that this strategy will use to submit orders
	 * 
	 * This is called by the BacktestEngine before backtesting starts.
	 * The strategy should store this pointer to use when making trading decisions.
	 * 
	 * @param om Pointer to the OrderManager instance for this strategy
	 */
	virtual void setOrderManager(OrderManager* om) = 0;
	
	/**
	 * @brief Called once before backtesting begins
	 * 
	 * Override this method to initialize strategy state, calculate initial
	 * indicators, or perform any setup work before processing ticks.
	 * Default implementation does nothing.
	 */
	virtual void onStart() {}
	
	/**
	 * @brief Called once after all ticks have been processed
	 * 
	 * Override this method to perform cleanup, final calculations, or
	 * log final statistics after backtesting completes.
	 * Default implementation does nothing.
	 */
	virtual void onEnd() {}
	
	/**
	 * @brief Called for each market tick during backtesting
	 * 
	 * This is where your trading logic lives. The strategy receives each
	 * tick as it arrives and can analyze it to make trading decisions.
	 * Orders are submitted through the OrderManager that was set earlier.
	 * 
	 * @param tick The current market tick containing price, volume, and timestamp
	 */
	virtual void onTick(const Tick& tick) = 0;
};