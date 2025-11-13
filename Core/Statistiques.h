#pragma once

#include <cmath>
#include <numeric>

#include "TimeFrame.h"
#include "StatsCollector.h"

/**
 * @brief Registers standard performance statistics for a strategy
 * 
 * This function registers common trading performance metrics that will be
 * computed at the end of backtesting. All statistics are computed from the
 * PnL and returns series collected during backtesting.
 * 
 * Registered statistics:
 * - MeanReturn: Average return per period
 * - TotalReturn: Total return over the entire backtest period
 * - MaxDrawdown: Maximum peak-to-trough decline (as a percentage)
 * - AnnualizedVolatility: Annualized standard deviation of returns
 * - Sharpe: Sharpe ratio (risk-adjusted return, annualized)
 * - Sortino: Sortino ratio (similar to Sharpe but only penalizes downside volatility)
 * 
 * @param collector The StatsCollector to register statistics with
 * @param tf Time frame for annualization calculations
 * @param riskFreeRate Risk-free rate for Sharpe/Sortino calculations (default: 0.0)
 * @param periodsPerYear Number of trading periods per year (default: 252 trading days)
 */
inline void registerUserStats(StatsCollector& collector, TimeFrame tf, double riskFreeRate = 0.0, double periodsPerYear = 252.0) {
	// Calculate number of periods per year for annualization
	// Example: MINUTE time frame = 390 bars/day * 252 days/year = 98,280 periods/year
	double nPeriods = getTicksPerDay(tf) * periodsPerYear;

	/**
	 * Mean Return: Simple average of all period returns
	 * Formula: mean = sum(returns) / count(returns)
	 */
	collector.addStat("MeanReturn", [&collector]() {
		const auto& ret = collector.getReturnsSeries();
		if (ret.empty()) return 0.0;
		return std::accumulate(ret.begin(), ret.end(), 0.0) / ret.size();
	});

	/**
	 * Total Return: Overall return from start to end
	 * Formula: (final_pnl / initial_pnl) - 1
	 * Example: If you started with $10,000 and ended with $12,500, TotalReturn = 0.25 (25%)
	 */
	collector.addStat("TotalReturn", [&collector]() {
		const auto& pnl = collector.getPnLSeries();
		return (std::abs(pnl.front()) > 1e-8) ? pnl.back() / pnl.front() - 1.0 : 0.0;
	});

	/**
	 * Maximum Drawdown: Largest peak-to-trough decline
	 * 
	 * Tracks the highest peak value seen so far, and calculates the percentage
	 * drop from that peak. The maximum of all such drops is the max drawdown.
	 * 
	 * Formula: max((current_pnl - peak) / peak) over all periods
	 * Result is negative (e.g., -0.15 means 15% drawdown)
	 * 
	 * Example: If portfolio peaked at $12,000 and dropped to $10,200,
	 * drawdown = (10,200 - 12,000) / 12,000 = -0.15 (-15%)
	 */
	collector.addStat("MaxDrawdown", [&collector]() {
		const auto& pnlSeries = collector.getPnLSeries();
		if (pnlSeries.empty()) return 0.0;

		double peak = pnlSeries[0];  // Track the highest PnL seen so far
		double maxDD = 0.0;          // Track the worst drawdown

		for (double pnl : pnlSeries) {
			peak = std::max(peak, pnl);  // Update peak if we hit a new high
			// Calculate drawdown from peak (will be negative or zero)
			if (peak > 1e-8) maxDD = std::min(maxDD, (pnl - peak) / peak);
		}

		return maxDD;
	});

	/**
	 * Annualized Volatility: Standard deviation of returns, scaled to annual
	 * 
	 * Volatility measures how much returns vary. Higher volatility = more risk.
	 * Annualized by multiplying by sqrt(periods_per_year).
	 * 
	 * Formula: std_dev(returns) * sqrt(periods_per_year)
	 */
	collector.addStat("AnnualizedVolatility", [&collector, nPeriods]() {
		const auto& returns = collector.getReturnsSeries();
		if (returns.size() < 2) return 0.0;

		// Calculate mean return
		double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
		
		// Calculate variance: average of squared deviations from mean
		double var = 0.0;
		for (double ret : returns) var += (ret - mean) * (ret - mean);
		var /= returns.size();

		// Standard deviation = sqrt(variance), annualized
		return std::sqrt(var) * std::sqrt(nPeriods);
	});

	/**
	 * Sharpe Ratio: Risk-adjusted return metric
	 * 
	 * Measures excess return per unit of risk (volatility).
	 * Higher Sharpe = better risk-adjusted performance.
	 * 
	 * Formula: (mean_return - risk_free_rate) / volatility * sqrt(periods_per_year)
	 * 
	 * Interpretation:
	 * - < 1: Poor (returns don't compensate for risk)
	 * - 1-2: Good
	 * - 2-3: Very good
	 * - > 3: Excellent
	 */
	collector.addStat("Sharpe", [&collector, nPeriods, &riskFreeRate, &periodsPerYear]() {
			const auto& returns = collector.getReturnsSeries();
			if (returns.size() < 2) return 0.0;

			// Calculate mean return
			double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
			
			// Calculate variance
			double var = 0.0;
			for (double ret : returns) {
				var += (ret - mean) * (ret - mean);
			}
			var /= returns.size();

			// Sharpe = (excess return) / (volatility) * sqrt(periods)
			// Add small epsilon (1e-8) to avoid division by zero
			return (mean - riskFreeRate) / std::sqrt(var + 1e-8) * std::sqrt(nPeriods);
	});

	/**
	 * Sortino Ratio: Similar to Sharpe, but only penalizes downside volatility
	 * 
	 * Unlike Sharpe which uses all volatility, Sortino only considers
	 * volatility from negative returns (downside risk). This is often considered
	 * more relevant since investors typically care more about downside risk.
	 * 
	 * Formula: (mean_return - risk_free_rate) / downside_volatility * sqrt(periods_per_year)
	 * 
	 * Generally, Sortino will be higher than Sharpe for the same strategy
	 * (since it ignores upside volatility).
	 */
	collector.addStat("Sortino", [&collector, nPeriods, &riskFreeRate, &periodsPerYear]() {
		const auto& returns = collector.getReturnsSeries();
		double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
		
		// Calculate downside variance (only from negative returns)
		double downsideVar = 0.0;
		int downsideCount = 0;
		for (double ret : returns) {
			if (ret < 0) {
				downsideVar += ret * ret;  // Square of negative return
				downsideCount++;
			}
		}

		if (returns.size() < 2 || downsideCount == 0) return 0.0;

		downsideVar /= downsideCount;  // Average of squared downside returns

		// Sortino = (excess return) / (downside volatility) * sqrt(periods)
		return (mean - riskFreeRate) / std::sqrt(downsideVar + 1e-8) * std::sqrt(nPeriods);
	});
}