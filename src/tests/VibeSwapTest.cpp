#define BOOST_TEST_MODULE VibeSwapTest

#include "MachinePalCommon.h"
#include "vibemarket/VibeSwap.h"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(add_liquidity_and_swap) {
    VibeSwap swap;
    VibeSwap::Token tokenA = "TOKENA";
    VibeSwap::Token tokenB = "TOKENB";
    EthAddress provider("0x0000000000000000000000000000000000000001");
    EthAddress trader("0x0000000000000000000000000000000000000002");

    swap.mintToken(provider, tokenA, 100000);
    swap.mintToken(provider, tokenB, 100000);
    swap.mintToken(trader, tokenA, 10000);

    auto result = swap.addLiquidity(provider, tokenA, tokenB,
                                    50000, 50000, 1, 1);

    BOOST_TEST(result.amountA == 50000);
    BOOST_TEST(result.amountB == 50000);
    BOOST_TEST(result.liquidity > 0);

    auto reserves = swap.getReserves(tokenA, tokenB);
    BOOST_TEST(reserves.first == 50000);
    BOOST_TEST(reserves.second == 50000);

    auto amounts = swap.swapExactTokensForTokens(trader, 1000, 1, {tokenA, tokenB}, trader);

    BOOST_TEST(amounts.size() == 2);
    BOOST_TEST(amounts.front() == 1000);
    BOOST_TEST(amounts.back() > 0);
    BOOST_TEST(swap.balanceOf(trader, tokenA) == 9000);
    BOOST_TEST(swap.balanceOf(trader, tokenB) == amounts.back());
}
