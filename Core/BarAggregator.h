#pragma once

#include <optional>
#include "Tick.h"
#include "Bar.h"

/**
 * @brief Aggregates ticks into OHLCV bars based on time windows
 * 
 * The BarAggregator groups individual ticks into bars (candlesticks) based on
 * a time window. For example, with a 60-second window, all ticks within each
 * 60-second period are aggregated into a single bar.
 * 
 * How it works:
 * 1. Each tick's timestamp is rounded down to the nearest window boundary
 * 2. Ticks in the same window are aggregated into one bar
 * 3. When a tick arrives in a new window, the previous bar is returned
 * 
 * Bar aggregation:
 * - Open: First price in the window
 * - High: Highest price in the window
 * - Low: Lowest price in the window
 * - Close: Last price in the window
 * - Volume: Sum of all volumes in the window
 * 
 * Example: If windowSize = 60000 (60 seconds in milliseconds),
 * ticks at timestamps 1000, 1500, 2000 (all in same 60s window)
 * are aggregated into one bar.
 */
class BarAggregator {
private:
	uint64_t windowSize;              // Time window size in milliseconds (e.g., 60000 for 1 minute)
	uint64_t currentWindow = 0;        // Current window's start timestamp
	std::optional<Bar> currentBar;    // The bar being built for the current window
	
public:
	/**
	 * @brief Constructs a BarAggregator with the specified window size
	 * 
	 * @param windowSize Time window in milliseconds (e.g., 60000 for 1-minute bars)
	 */
	BarAggregator(uint64_t windowSize) : windowSize(windowSize) {}

	/**
	 * @brief Updates the aggregator with a new tick
	 * 
	 * If the tick belongs to the current window, it updates the current bar.
	 * If the tick belongs to a new window, it:
	 * 1. Returns the completed bar from the previous window
	 * 2. Starts a new bar for the new window
	 * 
	 * @param tick The new tick to process
	 * @return Completed bar if we moved to a new window, nullopt otherwise
	 */
	std::optional<Bar> update(const Tick& tick) {
		// Calculate which window this tick belongs to
		// Round down timestamp to nearest window boundary
		// Example: tick.timestamp=125000, windowSize=60000 -> tickWindow=120000
		uint64_t tickWindow = tick.timestamp / windowSize * windowSize;

		// Check if this tick starts a new window
		if (!currentBar || tickWindow != currentWindow) {
			// Save the completed bar from previous window (if any)
			std::optional<Bar> completed = currentBar;
			
			// Start a new bar for the new window
			// Initialize OHLC all to the first tick's price
			currentBar = Bar{
				.startTimestamp = tickWindow,                    // Window start time
				.endTimestamp = tickWindow + windowSize,         // Window end time
				.open = tick.price,                              // First price = open
				.high = tick.price,                               // First price = initial high
				.low = tick.price,                                // First price = initial low
				.close = tick.price,                              // First price = initial close
				.volume = tick.volume,                           // First tick's volume
			};
			currentWindow = tickWindow;

			// Return the completed bar from previous window (if any)
			return completed;
		}

		// Update the current bar with this tick
		// High: maximum of current high and this tick's price
		currentBar->high = std::max(currentBar->high, tick.price);
		// Low: minimum of current low and this tick's price
		currentBar->low = std::min(currentBar->low, tick.price);
		// Close: always the last price in the window
		currentBar->close = tick.price;
		// Volume: accumulate all volumes in the window
		currentBar->volume += tick.volume;

		// No completed bar yet (still in same window)
		return std::nullopt;
	}

	/**
	 * @brief Flushes the current incomplete bar
	 * 
	 * Returns the bar that's currently being built, even if the window isn't complete.
	 * Useful at the end of backtesting to get the final bar.
	 * 
	 * @return The current incomplete bar, or nullopt if no bar exists
	 */
	std::optional<Bar> flush() const {
		return currentBar;
	}
};