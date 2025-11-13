#include "BacktestEngine.h"
#include "Statistiques.h"

/**
 * @brief Constructs a StrategyContext with all necessary components
 * 
 * Initializes all members and moves the strategy into this context.
 * The OrderManager is initialized with the starting cash amount.
 */
StrategyContext::StrategyContext(const std::string& name,
	std::unique_ptr<Strategy> strategy,
	const TimeFrame& tf,
	const double initialCash) : name(name), strategy(std::move(strategy)), tf(tf), orderManager(initialCash) {
}

/**
 * @brief Sets market data by copying (less efficient)
 */
void BacktestEngine::setTickData(std::vector<Tick>& ticks) {
	data = ticks;
}

/**
 * @brief Sets market data by moving (more efficient, preferred)
 */
void BacktestEngine::setTickData(std::vector<Tick>&& ticks) {
	data = std::move(ticks);
}

/**
 * @brief Sets quote data by copying (less efficient)
 */
void BacktestEngine::setTickData(std::vector<QuoteTick>& quoteTicks) {
	quoteData = quoteTicks;
}

/**
 * @brief Sets quote data by moving (more efficient, preferred)
 */
void BacktestEngine::setTickData(std::vector<QuoteTick>&& quoteTicks) {
	quoteData = std::move(quoteTicks);
}

/**
 * @brief Registers a strategy to be backtested
 * 
 * Creates a new StrategyContext and adds it to the strategies list.
 * Each strategy gets its own OrderManager and StatsCollector.
 */
void BacktestEngine::addStrategy(const std::string& name, std::unique_ptr<Strategy> strategy, const TimeFrame& tf, double initialCash) {
	strategies.emplace_back(std::make_unique<StrategyContext>(name, std::move(strategy), tf, initialCash));
}

/**
 * @brief Runs all registered strategies in parallel threads
 * 
 * This is the main execution method. For each strategy:
 * 
 * 1. Setup phase:
 *    - Connect strategy to its OrderManager
 *    - Register statistics collection callbacks
 * 
 * 2. Execution phase (in worker thread):
 *    - Call strategy->onStart() for initialization
 *    - Process all ticks:
 *      * Call strategy->onTick() to let strategy make decisions
 *      * Call orderManager->handleTick() to execute pending orders
 *      * Record PnL for statistics
 *    - Call strategy->onEnd() for cleanup
 *    - Compute final statistics
 * 
 * 3. Reporting phase:
 *    - Print results if verbose mode
 *    - Export to CSV if requested
 * 
 * All strategies run in parallel, so execution time is roughly the time
 * of the slowest strategy, not the sum of all strategies.
 */
void BacktestEngine::runAll(const bool saveToCSV, const bool verbose) {
	// Setup and launch each strategy in its own thread
	for (auto& context : strategies) {
		// Give the strategy access to its OrderManager so it can submit orders
		context->strategy->setOrderManager(&context->orderManager);
		
		// Register statistics collection callbacks (for custom metrics)
		registerUserStats(context->statistics, context->tf);

		// Launch worker thread for this strategy
		// Capture context by value (ctx) to avoid lifetime issues
		context->worker = std::thread([&, ctx = context.get()] {
			// Initialize strategy
			ctx->strategy->onStart();

			// Check if this is a QuoteStrategy (needs bid/ask data)
			if (auto* quoteStrat = dynamic_cast<QuoteStrategy*>(ctx->strategy.get())) {
				// Process quote ticks for quote-based strategies
				if (!quoteData.empty()) {
					for (const QuoteTick& tick : quoteData) {
						// Let strategy analyze the quote and make trading decisions
						quoteStrat->onTick(tick);
						
						// Check if any pending LIMIT orders should execute
						ctx->orderManager.handleTick(tick);
						
						// Record PnL using mid-price (average of bid and ask)
						ctx->statistics.recordPnL(ctx->orderManager.getPnL((tick.bid + tick.ask) / 2.0));
					}
				}
			}
			// Regular strategy (uses trade ticks)
			else if (!data.empty()) {
				for (const Tick& tick : data) {
					// Let strategy analyze the tick and make trading decisions
					ctx->strategy->onTick(tick);
					
					// Check if any pending LIMIT orders should execute
					ctx->orderManager.handleTick(tick);
					
					// Record PnL using current tick price
					ctx->statistics.recordPnL(ctx->orderManager.getPnL(tick.price));
				}
			} else {
				throw std::runtime_error("No data available for backtest.");
			}

			// Finalize strategy
			ctx->strategy->onEnd();
			
			// Compute final statistics (Sharpe ratio, max drawdown, etc.)
			auto stats = ctx->statistics.computeStats();

			// Print results if verbose mode is enabled
			if (verbose) {
				std::lock_guard<std::mutex> lock(globalPrintMutex);
				std::cout << "[" << ctx->name << "] Final PnL: " << ctx->orderManager.getPnL(0.0) << "\n";
				for (const auto& [name, value] : stats) {
					std::cout << " - " + name << ": " << value << "\n";
				}
			}

			// Export results to CSV files if requested
			if (saveToCSV) {
				ctx->statistics.exportPnLToCSV(ctx->name + "_pnl.csv");
				ctx->statistics.exportStatsToCSV(ctx->name + "_statistics.csv", stats);
			}
		});
	}

	// Wait for all strategy threads to complete
	// This ensures we don't exit before all strategies finish
	for (auto& context : strategies) {
		if (context->worker.joinable()) {
			context->worker.join();
		}
	}
}
