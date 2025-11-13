#pragma once

#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>

#include "Strategy.h"
#include "QuoteStrategy.h"
#include "TimeFrame.h"
#include "OrderManager.h"
#include "StatsCollector.h"

/**
 * @brief Container for all components needed to run a single strategy
 * 
 * Each strategy runs in its own context with:
 * - name: Identifier for this strategy instance
 * - strategy: The actual strategy implementation
 * - tf: Time frame this strategy operates on
 * - orderManager: Handles order execution and position tracking for this strategy
 * - statistics: Collects performance metrics during backtesting
 * - worker: Thread handle for parallel execution
 */
struct StrategyContext {
	std::string name;                    // Strategy identifier (e.g., "Mean_Reversion")
	std::unique_ptr<Strategy> strategy;  // The strategy implementation
	TimeFrame tf;                        // Time frame for this strategy
	OrderManager orderManager;            // Manages orders and positions for this strategy
	StatsCollector statistics;            // Collects performance statistics
	std::thread worker;                  // Thread handle for parallel execution

	/**
	 * @brief Constructs a StrategyContext with all necessary components
	 * 
	 * @param name Strategy name/identifier
	 * @param strategy The strategy instance (moved into this context)
	 * @param tf Time frame for this strategy
	 * @param initialCash Starting capital for this strategy
	 */
	StrategyContext(const std::string& name, std::unique_ptr<Strategy> strategy, const TimeFrame& tf, const double initialCash);
};

/**
 * @brief Global mutex for thread-safe console output
 * 
 * Declared here, defined in BacktestEngine_Project.cpp
 */
extern std::mutex globalPrintMutex;

/**
 * @brief Core backtesting engine that orchestrates strategy execution
 * 
 * The BacktestEngine is the main coordinator that:
 * 1. Stores market data (ticks) to backtest on
 * 2. Registers multiple strategies to test
 * 3. Runs all strategies in parallel threads on the same data
 * 4. Collects and reports performance statistics
 * 
 * Each strategy runs independently with its own OrderManager and StatsCollector,
 * so strategies don't interfere with each other. This allows easy comparison
 * of different strategies on the same historical data.
 */
class BacktestEngine {
private:
	std::vector<Tick> data;                              // Regular trade ticks for standard strategies
	std::vector<QuoteTick> quoteData;                     // Quote ticks (bid/ask) for quote-based strategies
	std::vector<std::unique_ptr<StrategyContext>> strategies;  // All registered strategies
	
public:
	/**
	 * @brief Sets the market data (regular ticks) to backtest on (copy version)
	 * 
	 * @param ticks Vector of trade ticks
	 */
	void setTickData(std::vector<Tick>& ticks);

	/**
	 * @brief Sets the market data (regular ticks) to backtest on (move version)
	 * 
	 * More efficient than copy version - moves the data instead of copying.
	 * 
	 * @param ticks Vector of trade ticks (will be moved)
	 */
	void setTickData(std::vector<Tick>&& ticks);

	/**
	 * @brief Sets the quote data (bid/ask ticks) to backtest on (copy version)
	 * 
	 * @param quoteTicks Vector of quote ticks
	 */
	void setTickData(std::vector<QuoteTick>& quoteTicks);

	/**
	 * @brief Sets the quote data (bid/ask ticks) to backtest on (move version)
	 * 
	 * @param quoteTicks Vector of quote ticks (will be moved)
	 */
	void setTickData(std::vector<QuoteTick>&& quoteTicks);

	/**
	 * @brief Registers a strategy to be backtested
	 * 
	 * Creates a StrategyContext for the strategy with its own OrderManager
	 * and StatsCollector. The strategy will run in parallel with other strategies.
	 * 
	 * @param name Unique identifier for this strategy instance
	 * @param strategy The strategy implementation (moved into the engine)
	 * @param tf Time frame this strategy operates on
	 * @param initialCash Starting capital for this strategy
	 */
	void addStrategy(const std::string& name, std::unique_ptr<Strategy> strategy, const TimeFrame& tf, double initialCash);

	/**
	 * @brief Runs all registered strategies in parallel and collects results
	 * 
	 * For each strategy:
	 * 1. Sets up the OrderManager connection
	 * 2. Registers statistics collection
	 * 3. Spawns a worker thread
	 * 4. In the thread: processes all ticks, calls strategy callbacks
	 * 5. Collects and reports statistics
	 * 6. Optionally exports results to CSV files
	 * 
	 * All strategies run in parallel threads, so they execute simultaneously
	 * on the same market data for fair comparison.
	 * 
	 * @param saveToCSV If true, exports PnL and statistics to CSV files
	 * @param verbose If true, prints detailed progress and results to console
	 */
	void runAll(const bool saveToCSV = false, const bool verbose = false);
};