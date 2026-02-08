#pragma once

#include "MachinePalCommon.h"

// VibeSwap is a thin, in-memory Uniswap V2-style model. It tracks wallet balances for tokens
// and LP balances for each pair, then applies the same constant-product (x*y=k) math used by
// Uniswap V2 when adding liquidity and swapping:
// - Pairs are identified by sorted token order (token0/token1). Reserves are stored in that order.
// - Adding liquidity mints LP tokens proportional to contributions; the first mint locks
//   MINIMUM_LIQUIDITY to the burn address, matching Uniswap V2 behavior.
// - Removing liquidity burns LP tokens and returns proportional reserves, respecting minimums.
// - Swaps apply the 0.3% fee (997/1000), compute hop-by-hop amounts for a path, and update reserves.
// - The model only moves balances within this instance; it does not touch any on-chain state.
class VibeSwap {
public:
    // Ethereum-style address string and token symbol/identifier used by this in-memory model.
    using Address = std::string;
    using Token = std::string;

    // Result of addLiquidity, matching Uniswap V2 behavior (actual amounts deposited + LP minted).
    struct AddLiquidityResult {
        u256 amountA;
        u256 amountB;
        u256 liquidity;
    };

    // Result of removeLiquidity, matching Uniswap V2 behavior (amounts returned).
    struct RemoveLiquidityResult {
        u256 amountA;
        u256 amountB;
    };

    // Snapshot of a pair's state (sorted tokens, reserves, and total LP supply).
    struct PairSnapshot {
        Token token0;
        Token token1;
        u256 reserve0;
        u256 reserve1;
        u256 totalSupply;
    };

    // Construct an empty swap model with no pairs and no balances.
    VibeSwap();

    // Mint fungible tokens into a wallet (testing/funding helper; not a Uniswap V2 factory action).
    void mintToken(const Address &to, const Token &token, const u256 &amount);
    // Return ERC20-like balance for a wallet and token.
    u256 balanceOf(const Address &owner, const Token &token) const;
    // Return LP token balance for a wallet for the (tokenA, tokenB) pair.
    u256 lpBalanceOf(const Address &owner, const Token &tokenA, const Token &tokenB) const;

    // Read-only pair state, with tokens in sorted order.
    PairSnapshot getPairSnapshot(const Token &tokenA, const Token &tokenB) const;
    // Convenience accessor for reserves in sorted order (token0, token1).
    std::pair<u256, u256> getReserves(const Token &tokenA, const Token &tokenB) const;

    // Add liquidity following Uniswap V2 math: initial mint uses sqrt(amountA*amountB) - MINIMUM_LIQUIDITY,
    // subsequent mints are proportional to reserves. Reverts if min constraints are not met.
    AddLiquidityResult addLiquidity(const Address &provider,
                                   const Token &tokenA,
                                   const Token &tokenB,
                                   const u256 &amountADesired,
                                   const u256 &amountBDesired,
                                   const u256 &amountAMin,
                                   const u256 &amountBMin);

    // Remove liquidity, burning LP tokens and returning underlying tokens subject to min constraints.
    RemoveLiquidityResult removeLiquidity(const Address &provider,
                                          const Token &tokenA,
                                          const Token &tokenB,
                                          const u256 &liquidity,
                                          const u256 &amountAMin,
                                          const u256 &amountBMin);

    // Swap an exact input amount across the path; returns all hop amounts and credits `to`.
    std::vector<u256> swapExactTokensForTokens(const Address &trader,
                                               const u256 &amountIn,
                                               const u256 &amountOutMin,
                                               const std::vector<Token> &path,
                                               const Address &to);

    // Swap to receive an exact output amount across the path; debits `trader` up to amountInMax.
    std::vector<u256> swapTokensForExactTokens(const Address &trader,
                                               const u256 &amountOut,
                                               const u256 &amountInMax,
                                               const std::vector<Token> &path,
                                               const Address &to);

    // Uniswap V2 quote() helper: amountB = amountA * reserveB / reserveA.
    static u256 quote(const u256 &amountA, const u256 &reserveA, const u256 &reserveB);
    // Uniswap V2 getAmountOut with 0.3% fee (997/1000): amountOut = ...
    // Fee-adjusted invariant: (reserveIn * reserveOut) <= (reserveIn + amountInWithFee) * (reserveOut - amountOut),
    // where amountInWithFee = amountIn * 997 / 1000. This preserves x*y=k after fees are applied.
    u256 getAmountOut(const u256 &amountIn, const u256 &reserveIn, const u256 &reserveOut) const;
    // Uniswap V2 getAmountIn with 0.3% fee (997/1000): amountIn = ...
    u256 getAmountIn(const u256 &amountOut, const u256 &reserveIn, const u256 &reserveOut) const;

    // Per-hop outputs for a path given an exact input.
    std::vector<u256> getAmountsOut(const u256 &amountIn, const std::vector<Token> &path) const;
    // Per-hop inputs for a path to achieve an exact output.
    std::vector<u256> getAmountsIn(const u256 &amountOut, const std::vector<Token> &path) const;

private:
    // Pair data stored in sorted token order with LP balances for providers.
    // Math notes (Uniswap V2 style): reserves are (reserve0, reserve1) for (token0, token1) and
    // define the constant-product invariant k = reserve0 * reserve1. Liquidity (LP) supply tracks
    // pro-rata ownership of reserves. On first mint, liquidity = sqrt(amount0 * amount1)
    // minus MINIMUM_LIQUIDITY, which is locked to the burn address. On subsequent mints,
    // liquidity = min(amount0 * totalSupply / reserve0, amount1 * totalSupply / reserve1).
    // On burn, amounts returned are amount0 = liquidity * reserve0 / totalSupply and
    // amount1 = liquidity * reserve1 / totalSupply (after which totalSupply decreases).
    struct Pair {
        Token token0;
        Token token1;
        u256 reserve0{0};
        u256 reserve1{0};
        u256 totalSupply{0};
        std::map<Address, u256> lpBalances;
    };

    // Uniswap V2 fee parameters (0.3%) and minimum locked liquidity.
    static constexpr uint32_t kFeeNumerator = 997;
    static constexpr uint32_t kFeeDenominator = 1000;
    static constexpr uint32_t kMinimumLiquidity = 1000;
    // Burn address for the permanently locked MINIMUM_LIQUIDITY.
    static constexpr const char *kBurnAddress = "0x0000000000000000000000000000000000000000";

    // Sort tokens deterministically (mirrors Uniswap V2 token0/token1).
    static std::pair<Token, Token> sortTokens(const Token &tokenA, const Token &tokenB);
    // Integer square-root used for initial liquidity minting.
    static u256 integerSqrt(const u256 &value);

    // Deterministic key for pair map (token0, token1).
    std::string pairKey(const Token &token0, const Token &token1) const;
    // Get or create a pair in sorted order.
    Pair &getOrCreatePair(const Token &tokenA, const Token &tokenB);
    // Get an existing pair or throw if not present.
    Pair &getPairChecked(const Token &tokenA, const Token &tokenB);
    const Pair &getPairChecked(const Token &tokenA, const Token &tokenB) const;

    // Validate wallet balance before debiting.
    void ensureBalance(const Address &owner, const Token &token, const u256 &amount) const;
    // Move tokens from wallet to pool.
    void debit(const Address &owner, const Token &token, const u256 &amount);
    // Move tokens from pool to wallet.
    void credit(const Address &owner, const Token &token, const u256 &amount);

    // Execute swap given precomputed hop amounts (path size N yields amounts size N).
    std::vector<u256> swapWithKnownAmounts(const Address &trader,
                                           const std::vector<u256> &amounts,
                                           const std::vector<Token> &path,
                                           const Address &to);

    // Map of pairKey -> Pair state, and wallet balances per token.
    std::map<std::string, Pair> pairs_;
    std::map<Address, std::map<Token, u256>> balances_;
};
