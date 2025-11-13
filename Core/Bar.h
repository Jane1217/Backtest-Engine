#pragma once

#include <cstdint>

/**
 * @brief Represents an OHLCV (Open, High, Low, Close, Volume) bar/candlestick
 * 
 * A Bar aggregates multiple ticks over a specific time period (e.g., 1 minute, 5 minutes)
 * into a single data point. This is the standard format for candlestick charts
 * and is commonly used in technical analysis strategies.
 * 
 * Bars are created by aggregating ticks within a time window:
 * - Open: First price in the period
 * - High: Highest price in the period
 * - Low: Lowest price in the period
 * - Close: Last price in the period
 * - Volume: Total volume traded in the period
 */
struct Bar {
	uint64_t startTimestamp;  // Start time of this bar (inclusive)
	uint64_t endTimestamp;    // End time of this bar (exclusive)
	double open;              // Opening price (first tick price in the period)
	double high;              // Highest price reached during this period
	double low;               // Lowest price reached during this period
	double close;             // Closing price (last tick price in the period)
	double volume;            // Total volume traded during this period
};