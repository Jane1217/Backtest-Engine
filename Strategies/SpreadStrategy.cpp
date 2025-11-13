#include "SpreadStrategy.h"

/**
 * @brief Sets the OrderManager for submitting orders
 */
void SpreadStrategy::setOrderManager(OrderManager* om) {
	orderManager = om;
}

/**
 * @brief Main strategy logic - implements market making with spread trading
 * 
 * Strategy steps:
 * 1. Calculate the bid-ask spread
 * 2. Check if spread is large enough (>= minSpread)
 * 3. Calculate quote prices:
 *    - Buy quote: bid - offset (trying to buy below market)
 *    - Sell quote: ask + offset (trying to sell above market)
 * 4. Place LIMIT buy order if position is not too large (< 5.0)
 * 5. Place LIMIT sell order if position is not too negative (> -5.0)
 * 
 * The strategy continuously places LIMIT orders on both sides, hoping to:
 * - Buy at a discount (below bid) and sell at a premium (above ask)
 * - Profit from the spread when orders are filled
 * 
 * Position limits prevent excessive exposure:
 * - Won't buy more if already long 5+ shares
 * - Won't sell more if already short 5+ shares
 */
void SpreadStrategy::onTick(const QuoteTick& tick) {
	// Calculate the bid-ask spread (difference between ask and bid)
	double spread = tick.ask - tick.bid;
	
	// Get current position to check position limits
	double position = orderManager->getPosition();

	// Only trade if spread is large enough to be profitable
	// If spread is too small, we skip this tick
	if (spread < minSpread) return;

	// Calculate our quote prices (where we want to place orders)
	// We place orders slightly away from market to improve fill probability
	double bidQuote = tick.bid - offset;  // Buy order: below bid (trying to buy cheap)
	double askQuote = tick.ask + offset;   // Sell order: above ask (trying to sell expensive)

	// Place LIMIT BUY order if we're not too long already
	// Position limit: won't buy if position >= 5.0 (avoid excessive long exposure)
	if (position < 5.0) {
		Order buy = Order{
			.side = Order::Side::BUY,       // Buy side
			.type = OrderType::LIMIT,      // LIMIT order (executes when price drops to bidQuote)
			.timestamp = tick.timestamp,   // Current timestamp
			.volume = orderSize,            // Order volume
			.price = bidQuote               // Limit price (below current bid)
		};
		orderManager->submit(buy);

		// Console output (disabled when running from web interface)
		{
			std::lock_guard<std::mutex> lock(globalPrintMutex);
			std::cout << "[SPREAD] Placing LIMIT BUY @ " << bidQuote << "\n";
		}
	}

	// Place LIMIT SELL order if we're not too short already
	// Position limit: won't sell if position <= -5.0 (avoid excessive short exposure)
	if (position > -5.0) {
		Order sell = Order{
			.side = Order::Side::SELL,     // Sell side
			.type = OrderType::LIMIT,        // LIMIT order (executes when price rises to askQuote)
			.timestamp = tick.timestamp,     // Current timestamp
			.volume = orderSize,            // Order volume
			.price = askQuote                // Limit price (above current ask)
		};
		orderManager->submit(sell);

		// Console output (disabled when running from web interface)
		{
			std::lock_guard<std::mutex> lock(globalPrintMutex);
			std::cout << "[SPREAD] Placing LIMIT SELL @ " << askQuote << "\n";
		}
	}

	// Debug output (commented out, but can be enabled for debugging)
	/*std::lock_guard<std::mutex> lock(globalPrintMutex);
	std::cout << "[SPREAD] Pos : " << position << " | Spread : " << spread 
	          << " | BidQ " << bidQuote << " | AskQ " << askQuote << "\n";*/
}
