#pragma once

#include <iostream>
#include <mutex>

#include "Strategy.h"
#include "BarAggregator.h"
#include "OrderManager.h"

extern std::mutex globalPrintMutex;

/**
 * @brief Base class for strategies that operate on bars (OHLCV) instead of individual ticks
 * 
 * Some strategies work better with aggregated data (bars/candlesticks) rather than
 * individual ticks. This class automatically aggregates ticks into bars and calls
 * onBar() when a bar is completed.
 * 
 * How it works:
 * 1. Inherit from BarStrategy instead of Strategy
 * 2. Implement onBar() instead of onTick()
 * 3. The BarStrategy automatically aggregates ticks into bars
 * 4. When a bar is completed, onBar() is called with the completed bar
 * 
 * Example use cases:
 * - Strategies that use technical indicators (moving averages, RSI, etc.)
 * - Strategies that analyze candlestick patterns
 * - Strategies that need OHLC data (not just last price)
 * 
 * The windowSize parameter determines how ticks are aggregated:
 * - windowSize in milliseconds (e.g., 60000 = 1-minute bars)
 */
class BarStrategy : public Strategy {
private:
	BarAggregator aggregator;        // Aggregates ticks into bars
	OrderManager* orderManager = nullptr;  // OrderManager pointer (inherited from Strategy)
	
public:
	/**
	 * @brief Constructs a BarStrategy with the specified bar window size
	 * 
	 * @param windowSize Time window in milliseconds for bar aggregation
	 *                   Default: 60ms (very short bars, likely a typo - should be 60000 for 1 minute)
	 */
	BarStrategy(uint64_t windowSize = 60) : aggregator(windowSize) {}

	virtual ~BarStrategy() = default;

	/**
	 * @brief Called when a bar is completed
	 * 
	 * This is the main method that bar-based strategies implement.
	 * It receives a completed OHLCV bar instead of individual ticks.
	 * 
	 * @param bar The completed bar with OHLCV data
	 */
	virtual void onBar(const Bar& bar) = 0;

	/**
	 * @brief Processes a tick and aggregates it into bars
	 * 
	 * This method is called by BacktestEngine for each tick.
	 * It delegates to the BarAggregator, and when a bar is completed,
	 * calls the derived class's onBar() method.
	 * 
	 * @param tick The current market tick
	 */
	void onTick(const Tick& tick) override {
		// Update aggregator with new tick
		// Returns a completed bar if we moved to a new time window
		auto bar = aggregator.update(tick);
		
		// If a bar was completed, call the strategy's onBar() method
		if (bar.has_value()) {
			onBar(bar.value());
		}
	}
};