cmake_minimum_required(VERSION 3.15)



# Example: Data WebSocket
add_executable(symbol_update symbol_update.c)
target_link_libraries(symbol_update PRIVATE fyers-api-c)

# Example: Order WebSocket
add_executable(on_general on_general.c)
target_link_libraries(on_general PRIVATE fyers-api-c)

# Example: Market Data - Index Update
add_executable(index_update index_update.c)
target_link_libraries(index_update PRIVATE fyers-api-c)

# Example: Market Data - Lite Symbol Update
add_executable(lite_symbol_update lite_symbol_update.c)
target_link_libraries(lite_symbol_update PRIVATE fyers-api-c)

# Example: Market Data - Depth Update
add_executable(depth_update depth_update.c)
target_link_libraries(depth_update PRIVATE fyers-api-c)

# Example: Market Data - Unsubscribe Update
add_executable(unsubscribe_update unsubscribe_update.c)
target_link_libraries(unsubscribe_update PRIVATE fyers-api-c)

# Example: Order Socket - OnOrders
add_executable(on_orders on_orders.c)
target_link_libraries(on_orders PRIVATE fyers-api-c)

# Example: Order Socket - OnPosition
add_executable(on_position on_position.c)
target_link_libraries(on_position PRIVATE fyers-api-c)

# Example: Order Socket - OnTrade
add_executable(on_trade on_trade.c)
target_link_libraries(on_trade PRIVATE fyers-api-c)

# Example: Auth Example
add_executable(sample_code sample_code.c)
target_link_libraries(sample_code PRIVATE fyers-api-c)

# Example: C++ Test
add_executable(cpp_test cpp_test.cpp)
target_link_libraries(cpp_test PRIVATE fyers-api-c)
set_target_properties(cpp_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

