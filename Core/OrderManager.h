#pragma once

#include <cstdint>
#include <vector>

#include "Tick.h"

/**
 * @brief Types of orders that can be placed
 * 
 * MARKET: Execute immediately at the current market price
 * LIMIT: Execute only when price reaches the specified limit price
 */
enum class OrderType { MARKET, LIMIT };

/**
 * @brief Represents a trading order (buy or sell request)
 * 
 * An order is a request to buy or sell a certain volume at a certain price.
 * Orders can be market orders (execute immediately) or limit orders
 * (execute only when price conditions are met).
 */
struct Order {
	enum class Side { BUY, SELL };  // Whether this is a buy or sell order
	Side side;                      // The side of this order
	OrderType type;                 // MARKET (immediate) or LIMIT (conditional)
	uint64_t timestamp;             // When this order was created
	double volume;                  // Number of shares/contracts to trade
	double price;                   // Price for LIMIT orders, or execution price for MARKET orders
};

/**
 * @brief Manages order execution, position tracking, and portfolio accounting
 * 
 * The OrderManager is responsible for:
 * 1. Receiving orders from strategies
 * 2. Executing orders when conditions are met (immediately for MARKET, when price crosses for LIMIT)
 * 3. Tracking current position (how many shares we own)
 * 4. Tracking cash balance
 * 5. Calculating profit and loss (PnL)
 * 
 * Each strategy has its own OrderManager instance, so strategies don't
 * interfere with each other's positions or cash.
 */
class OrderManager {
private:
	std::vector<Order> pendingOrders;  // LIMIT orders waiting for price conditions
	double position = 0.0;              // Current position (positive = long, negative = short)
	double cash;                        // Available cash balance
	
public:
	/**
	 * @brief Constructs an OrderManager with initial cash
	 * 
	 * @param cash Starting cash balance for this strategy
	 */
	OrderManager(int cash) : cash(cash) {}

	/**
	 * @brief Submits an order for execution
	 * 
	 * MARKET orders are executed immediately.
	 * LIMIT orders are added to pendingOrders and executed when price conditions are met.
	 * 
	 * @param order The order to submit
	 */
	void submit(Order& order);

	/**
	 * @brief Executes an order immediately
	 * 
	 * Updates position and cash based on the order:
	 * - BUY: Increase position, decrease cash
	 * - SELL: Decrease position, increase cash
	 * 
	 * @param order The order to execute
	 */
	void execute(Order& order);

	/**
	 * @brief Processes a tick and checks if any pending LIMIT orders should execute
	 * 
	 * For each pending LIMIT order:
	 * - BUY orders execute when tick.price <= order.price (price dropped to our buy level)
	 * - SELL orders execute when tick.price >= order.price (price rose to our sell level)
	 * 
	 * @param tick The current market tick
	 */
	void handleTick(const Tick& tick);

	/**
	 * @brief Processes a quote tick and checks if any pending LIMIT orders should execute
	 * 
	 * Similar to handleTick but uses bid/ask prices from the order book:
	 * - BUY orders execute when order.price >= quote.ask (we can buy at ask price)
	 * - SELL orders execute when order.price <= quote.bid (we can sell at bid price)
	 * 
	 * This is more realistic for strategies that need to see the spread.
	 * 
	 * @param quote The current quote tick with bid/ask prices
	 */
	void handleTick(const QuoteTick& quote);

	/**
	 * @brief Calculates current profit and loss (PnL)
	 * 
	 * PnL = cash + (position * current_price)
	 * This represents the total portfolio value if we closed all positions now.
	 * 
	 * @param lastPrice The current market price to value the position
	 * @return Total portfolio value (cash + position value)
	 */
	double getPnL(double lastPrice) const;

	/**
	 * @brief Gets the current position (number of shares owned)
	 * 
	 * @return Current position (positive = long, negative = short, 0 = flat)
	 */
	double getPosition() const;
};