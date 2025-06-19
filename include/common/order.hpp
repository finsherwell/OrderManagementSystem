#include <cstdint>

#pragma pack(push, 1)

enum class Side : uint8_t  {BUY = 0, SELL = 1};
enum class OrderType : uint8_t {LIMIT = 0, MARKET = 1};

struct Order {
    char symbol[8];
    uint64_t order_id;
    Side side;
    OrderType order_type;
    double price;
    uint32_t quantity;
};

#pragma pack(pop)