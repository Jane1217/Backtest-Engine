#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <cmath>
#include <numeric>

/**
 * @brief Type alias for statistics output - maps metric names to their values
 * 
 * Example: {"Sharpe": 1.5, "MaxDrawdown": -0.15, "TotalReturn": 0.25}
 */
using StatsMap = std::unordered_map<std::string, double>;

/**
 * @brief Type alias for a function that computes a statistic
 * 
 * Statistics are computed lazily - the function is called when computeStats() is invoked.
 * This allows statistics to be registered before all data is collected.
 */
using StatsFunction = std::function<double()>;

/**
 * @brief Collects and computes performance statistics for a strategy
 * 
 * The StatsCollector tracks:
 * - PnL series: Portfolio value over time
 * - Returns series: Percentage returns between consecutive PnL values
 * - Custom statistics: User-defined metrics (Sharpe ratio, max drawdown, etc.)
 * 
 * Statistics are computed on-demand when computeStats() is called, allowing
 * for efficient collection during backtesting and flexible metric calculation.
 */
class StatsCollector {
private:
	double initialPnL;                                    // Starting portfolio value
	std::vector<double> pnlSeries;                        // Portfolio value at each tick
	std::vector<double> returnsSeries;                    // Returns between consecutive ticks
	std::unordered_map<std::string, StatsFunction> statsFunction;  // Registered statistics calculators
	
public:
	/**
	 * @brief Constructs a StatsCollector with zero initial PnL
	 */
	StatsCollector() : initialPnL(0.0) {}

	/**
	 * @brief Exports the PnL series to a CSV file
	 * 
	 * Creates a CSV file with two columns: index and PnL value.
	 * Useful for plotting equity curves or analyzing performance over time.
	 * 
	 * @param filename Output CSV file path
	 */
	void exportPnLToCSV(const std::string& filename) const;

	/**
	 * @brief Exports computed statistics to a CSV file
	 * 
	 * Creates a CSV file with two columns: metric name and value.
	 * 
	 * @param filename Output CSV file path
	 * @param metrics Map of statistic names to values
	 */
	void exportStatsToCSV(const std::string& filename, const StatsMap& metrics) const;

	/**
	 * @brief Records a new PnL value and updates the returns series
	 * 
	 * Called for each tick during backtesting to track portfolio value over time.
	 * Also calculates the return (percentage change) from the previous PnL.
	 * 
	 * @param pnl Current portfolio value (cash + position value)
	 */
	void recordPnL(double pnl);

	/**
	 * @brief Registers a custom statistic calculator
	 * 
	 * Statistics are computed lazily - the function is stored and called later
	 * when computeStats() is invoked. This allows statistics to access the
	 * full PnL and returns series after all data has been collected.
	 * 
	 * @param name Name of the statistic (e.g., "Sharpe", "MaxDrawdown")
	 * @param function Function that computes the statistic value
	 */
	void addStat(std::string name, StatsFunction function);

	/**
	 * @brief Computes all registered statistics
	 * 
	 * Calls each registered statistic function and returns a map of results.
	 * This is typically called once at the end of backtesting after all
	 * PnL values have been recorded.
	 * 
	 * @return Map of statistic names to computed values
	 */
	StatsMap computeStats();

	/**
	 * @brief Gets the initial PnL (starting portfolio value)
	 * 
	 * @return Initial portfolio value
	 */
	const double getInitialPnL() const;

	/**
	 * @brief Gets the PnL series (portfolio value over time)
	 * 
	 * @return Reference to the vector of PnL values
	 */
	const std::vector<double>& getPnLSeries() const;

	/**
	 * @brief Gets the returns series (percentage returns over time)
	 * 
	 * @return Reference to the vector of return values
	 */
	const std::vector<double>& getReturnsSeries() const;
};