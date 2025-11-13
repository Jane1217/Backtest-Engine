#pragma once

#include <cstdint>

/**
 * @brief Represents a single price tick (trade) in the market
 * 
 * A Tick is the most granular data point in financial markets, representing
 * a single executed trade at a specific price and volume at a specific time.
 * This is the fundamental building block for all market data analysis.
 */
struct Tick {
	uint64_t timestamp;  // Unix timestamp in milliseconds - when this trade occurred
	double price;        // The execution price of this trade
	double volume;       // The number of shares/contracts traded in this tick
};

/**
 * @brief Represents a quote tick with bid/ask prices (order book data)
 * 
 * Unlike a regular Tick which represents an executed trade, a QuoteTick
 * represents the current best bid and ask prices in the order book.
 * This is useful for strategies that need to see the spread between
 * buy and sell orders, or for simulating more realistic order execution.
 */
struct QuoteTick {
	uint64_t timestamp;  // Unix timestamp in milliseconds - when this quote was valid
	double bid;          // Best bid price (highest price buyers are willing to pay)
	double ask;          // Best ask price (lowest price sellers are willing to accept)
	double volume;       // Volume available at these bid/ask levels
};