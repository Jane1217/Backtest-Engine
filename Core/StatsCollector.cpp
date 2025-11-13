#include <fstream>

#include "StatsCollector.h"

/**
 * @brief Exports PnL series to CSV file for plotting/analysis
 * 
 * Creates a CSV with two columns:
 * - Index: Sequential number (0, 1, 2, ...)
 * - PnL: Portfolio value at that index
 * 
 * This can be used to plot equity curves showing how portfolio value
 * changes over time during the backtest.
 */
void StatsCollector::exportPnLToCSV(const std::string& filename) const {
	std::ofstream file(filename);
	if (!file.is_open()) return;  // Silently fail if file can't be opened

	file << "Index,PnL\n";  // CSV header

	// Write each PnL value with its index
	for (size_t i = 0; i < pnlSeries.size(); i++) {
		file << i << "," << pnlSeries[i] << "\n";
	}

	file.close();
}

/**
 * @brief Exports computed statistics to CSV file
 * 
 * Creates a CSV with two columns:
 * - Metric: Name of the statistic (e.g., "Sharpe", "MaxDrawdown")
 * - Value: The computed value
 * 
 * Useful for comparing statistics across multiple strategies.
 */
void StatsCollector::exportStatsToCSV(const std::string& filename, const StatsMap& metrics) const {
	std::ofstream file(filename);
	if (!file.is_open()) return;  // Silently fail if file can't be opened

	file << "Metric,Value\n";  // CSV header

	// Write each statistic name and value
	for (const auto& [name, value] : metrics) {
		file << name << "," << value << "\n";
	}

	file.close();
}

/**
 * @brief Records a new PnL value and calculates the return
 * 
 * This is called for each tick during backtesting. It:
 * 1. Stores the first PnL as initialPnL (starting value)
 * 2. Calculates the return from previous PnL: (current - previous) / previous
 * 3. Stores both the PnL and the calculated return
 * 
 * The return calculation uses a small epsilon (1e-8) to avoid division by zero
 * if the previous PnL is exactly zero.
 */
void StatsCollector::recordPnL(double pnl) {
	// Store the first PnL as the initial value
	if (pnlSeries.empty()) {
		initialPnL = pnl;
	}

	// Calculate return from previous PnL (if we have previous data)
	if (!pnlSeries.empty()) {
		double prev = pnlSeries.back();
		// Return = (current - previous) / previous
		// Add epsilon to denominator to avoid division by zero
		returnsSeries.push_back((pnl - prev) / (std::abs(prev) + 1e-8));
	}
	
	// Store the new PnL value
	pnlSeries.push_back(pnl);
}

/**
 * @brief Registers a statistic calculator function
 * 
 * Statistics are stored as functions that will be called later when computeStats()
 * is invoked. This allows statistics to be registered before all data is collected,
 * and computed after all PnL values have been recorded.
 * 
 * If a statistic with the same name already exists, it is not overwritten.
 */
void StatsCollector::addStat(std::string name, StatsFunction function) {
	if (statsFunction.count(name)) return;  // Don't overwrite existing stats
	statsFunction[name] = std::move(function);
}

/**
 * @brief Computes all registered statistics
 * 
 * Calls each registered statistic function and collects the results.
 * Returns an empty map if there's insufficient data (< 2 PnL values needed
 * for most statistics).
 * 
 * @return Map of statistic names to computed values
 */
StatsMap StatsCollector::computeStats() {
	StatsMap results;
	
	// Need at least 2 data points to compute meaningful statistics
	if (pnlSeries.size() < 2) return results;

	// Call each registered statistic function and store the result
	for (const auto& [name, fn] : statsFunction) {
		results[name] = fn();
	}

	return results;
}

/**
 * @brief Returns the initial PnL (starting portfolio value)
 */
const double StatsCollector::getInitialPnL() const {
	return initialPnL;
}

/**
 * @brief Returns a reference to the PnL series
 * 
 * This allows statistic functions to access the full PnL history
 * for calculations (e.g., max drawdown needs to see all values).
 */
const std::vector<double>& StatsCollector::getPnLSeries() const {
	return pnlSeries;
}

/**
 * @brief Returns a reference to the returns series
 * 
 * This allows statistic functions to access the full returns history
 * for calculations (e.g., Sharpe ratio needs all returns).
 */
const std::vector<double>& StatsCollector::getReturnsSeries() const {
	return returnsSeries;
}
