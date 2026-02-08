//
// Created by kladko on 08/02/26.
//

#include "VibeSwap.h"

namespace {
std::string toString(const VibeSwap::Token &token) {
    return token;
}
}

VibeSwap::VibeSwap() = default;

void VibeSwap::mintToken(const EthAddress &to, const Token &token, const u256 &amount) {
    if (token.empty()) {
        throw std::invalid_argument("Token cannot be empty");
    }
    balances_[to][token] += amount;
}

u256 VibeSwap::balanceOf(const EthAddress &owner, const Token &token) const {
    auto ownerIt = balances_.find(owner);
    if (ownerIt == balances_.end()) {
        return 0;
    }
    auto tokenIt = ownerIt->second.find(token);
    if (tokenIt == ownerIt->second.end()) {
        return 0;
    }
    return tokenIt->second;
}

u256 VibeSwap::lpBalanceOf(const EthAddress &owner, const Token &tokenA, const Token &tokenB) const {
    const auto &pair = getPairChecked(tokenA, tokenB);
    auto lpIt = pair.lpBalances.find(owner);
    if (lpIt == pair.lpBalances.end()) {
        return 0;
    }
    return lpIt->second;
}

VibeSwap::PairSnapshot VibeSwap::getPairSnapshot(const Token &tokenA, const Token &tokenB) const {
    const auto &pair = getPairChecked(tokenA, tokenB);
    return {pair.token0, pair.token1, pair.contractAddress, pair.reserve0, pair.reserve1, pair.totalSupply};
}

std::pair<u256, u256> VibeSwap::getReserves(const Token &tokenA, const Token &tokenB) const {
    const auto &pair = getPairChecked(tokenA, tokenB);
    return {pair.reserve0, pair.reserve1};
}

VibeSwap::AddLiquidityResult VibeSwap::addLiquidity(const EthAddress &provider,
                                                    const Token &tokenA,
                                                    const Token &tokenB,
                                                    const u256 &amountADesired,
                                                    const u256 &amountBDesired,
                                                    const u256 &amountAMin,
                                                    const u256 &amountBMin) {
    if (amountADesired == 0 || amountBDesired == 0) {
        throw std::invalid_argument("Desired amounts must be positive");
    }

    auto [token0, token1] = sortTokens(tokenA, tokenB);
    auto &pair = getOrCreatePair(token0, token1);

    u256 amount0Desired = (tokenA == token0) ? amountADesired : amountBDesired;
    u256 amount1Desired = (tokenA == token0) ? amountBDesired : amountADesired;
    u256 amount0Min = (tokenA == token0) ? amountAMin : amountBMin;
    u256 amount1Min = (tokenA == token0) ? amountBMin : amountAMin;

    u256 amount0 = 0;
    u256 amount1 = 0;
    u256 liquidity = 0;
    bool isFirstMint = (pair.totalSupply == 0);

    if (isFirstMint) {
        amount0 = amount0Desired;
        amount1 = amount1Desired;
        if (amount0 < amount0Min || amount1 < amount1Min) {
            throw std::runtime_error("Insufficient amounts for initial liquidity");
        }
        u256 rootK = integerSqrt(amount0 * amount1);
        if (rootK <= kMinimumLiquidity) {
            throw std::runtime_error("Insufficient liquidity minted");
        }
        liquidity = rootK - kMinimumLiquidity;
    } else {
        u256 amount1Optimal = quote(amount0Desired, pair.reserve0, pair.reserve1);
        if (amount1Optimal <= amount1Desired) {
            if (amount1Optimal < amount1Min) {
                throw std::runtime_error("Insufficient amount1 provided");
            }
            amount0 = amount0Desired;
            amount1 = amount1Optimal;
        } else {
            u256 amount0Optimal = quote(amount1Desired, pair.reserve1, pair.reserve0);
            if (amount0Optimal < amount0Min) {
                throw std::runtime_error("Insufficient amount0 provided");
            }
            amount0 = amount0Optimal;
            amount1 = amount1Desired;
        }

        u256 liquidity0 = (amount0 * pair.totalSupply) / pair.reserve0;
        u256 liquidity1 = (amount1 * pair.totalSupply) / pair.reserve1;
        liquidity = std::min(liquidity0, liquidity1);
        if (liquidity == 0) {
            throw std::runtime_error("Insufficient liquidity minted");
        }
    }

    ensureBalance(provider, token0, amount0);
    ensureBalance(provider, token1, amount1);
    debit(provider, token0, amount0);
    debit(provider, token1, amount1);

    if (isFirstMint) {
        pair.totalSupply = liquidity + kMinimumLiquidity;
        pair.lpBalances[kBurnAddress] += kMinimumLiquidity;
    } else {
        pair.totalSupply += liquidity;
    }

    pair.reserve0 += amount0;
    pair.reserve1 += amount1;
    pair.lpBalances[provider] += liquidity;

    u256 amountA = (tokenA == token0) ? amount0 : amount1;
    u256 amountB = (tokenA == token0) ? amount1 : amount0;
    return {amountA, amountB, liquidity};
}

VibeSwap::RemoveLiquidityResult VibeSwap::removeLiquidity(const EthAddress &provider,
                                                          const Token &tokenA,
                                                          const Token &tokenB,
                                                          const u256 &liquidity,
                                                          const u256 &amountAMin,
                                                          const u256 &amountBMin) {
    if (liquidity == 0) {
        throw std::invalid_argument("Liquidity must be positive");
    }

    auto &pair = getPairChecked(tokenA, tokenB);
    if (pair.totalSupply == 0) {
        throw std::runtime_error("Pair has no liquidity");
    }

    auto lpIt = pair.lpBalances.find(provider);
    if (lpIt == pair.lpBalances.end() || lpIt->second < liquidity) {
        throw std::runtime_error("Insufficient LP balance");
    }

    u256 amount0 = (liquidity * pair.reserve0) / pair.totalSupply;
    u256 amount1 = (liquidity * pair.reserve1) / pair.totalSupply;
    if (amount0 == 0 || amount1 == 0) {
        throw std::runtime_error("Insufficient liquidity burned");
    }

    auto [token0, token1] = sortTokens(tokenA, tokenB);
    u256 amountA = (tokenA == token0) ? amount0 : amount1;
    u256 amountB = (tokenA == token0) ? amount1 : amount0;
    if (amountA < amountAMin || amountB < amountBMin) {
        throw std::runtime_error("Minimum amount constraints not met");
    }

    pair.reserve0 -= amount0;
    pair.reserve1 -= amount1;
    pair.totalSupply -= liquidity;
    lpIt->second -= liquidity;

    credit(provider, token0, amount0);
    credit(provider, token1, amount1);

    return {amountA, amountB};
}

std::vector<u256> VibeSwap::swapExactTokensForTokens(const EthAddress &trader,
                                                     const u256 &amountIn,
                                                     const u256 &amountOutMin,
                                                     const std::vector<Token> &path,
                                                     const EthAddress &to) {
    auto amounts = getAmountsOut(amountIn, path);
    if (amounts.back() < amountOutMin) {
        throw std::runtime_error("Insufficient output amount");
    }
    return swapWithKnownAmounts(trader, amounts, path, to);
}

std::vector<u256> VibeSwap::swapTokensForExactTokens(const EthAddress &trader,
                                                     const u256 &amountOut,
                                                     const u256 &amountInMax,
                                                     const std::vector<Token> &path,
                                                     const EthAddress &to) {
    auto amounts = getAmountsIn(amountOut, path);
    if (amounts.front() > amountInMax) {
        throw std::runtime_error("Excessive input amount");
    }
    return swapWithKnownAmounts(trader, amounts, path, to);
}

u256 VibeSwap::quote(const u256 &amountA, const u256 &reserveA, const u256 &reserveB) {
    if (amountA == 0) {
        throw std::invalid_argument("Amount must be positive");
    }
    if (reserveA == 0 || reserveB == 0) {
        throw std::invalid_argument("Reserves must be positive");
    }
    return (amountA * reserveB) / reserveA;
}

u256 VibeSwap::getAmountOut(const u256 &amountIn, const u256 &reserveIn, const u256 &reserveOut) const {
    if (amountIn == 0) {
        throw std::invalid_argument("Amount in must be positive");
    }
    if (reserveIn == 0 || reserveOut == 0) {
        throw std::invalid_argument("Reserves must be positive");
    }

    u256 amountInWithFee = amountIn * kFeeNumerator;
    u256 numerator = amountInWithFee * reserveOut;
    u256 denominator = reserveIn * kFeeDenominator + amountInWithFee;
    return numerator / denominator;
}

u256 VibeSwap::getAmountIn(const u256 &amountOut, const u256 &reserveIn, const u256 &reserveOut) const {
    if (amountOut == 0) {
        throw std::invalid_argument("Amount out must be positive");
    }
    if (reserveIn == 0 || reserveOut == 0) {
        throw std::invalid_argument("Reserves must be positive");
    }
    if (amountOut >= reserveOut) {
        throw std::invalid_argument("Amount out exceeds reserves");
    }

    u256 numerator = reserveIn * amountOut * kFeeDenominator;
    u256 denominator = (reserveOut - amountOut) * kFeeNumerator;
    return numerator / denominator + 1;
}

std::vector<u256> VibeSwap::getAmountsOut(const u256 &amountIn, const std::vector<Token> &path) const {
    if (path.size() < 2) {
        throw std::invalid_argument("Path must have at least two tokens");
    }

    std::vector<u256> amounts(path.size());
    amounts[0] = amountIn;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        const auto &pair = getPairChecked(path[i], path[i + 1]);
        auto [token0, token1] = sortTokens(path[i], path[i + 1]);
        bool inputIsToken0 = (path[i] == token0);
        u256 reserveIn = inputIsToken0 ? pair.reserve0 : pair.reserve1;
        u256 reserveOut = inputIsToken0 ? pair.reserve1 : pair.reserve0;
        amounts[i + 1] = getAmountOut(amounts[i], reserveIn, reserveOut);
    }
    return amounts;
}

std::vector<u256> VibeSwap::getAmountsIn(const u256 &amountOut, const std::vector<Token> &path) const {
    if (path.size() < 2) {
        throw std::invalid_argument("Path must have at least two tokens");
    }

    std::vector<u256> amounts(path.size());
    amounts.back() = amountOut;
    for (size_t i = path.size() - 1; i > 0; --i) {
        const auto &pair = getPairChecked(path[i - 1], path[i]);
        auto [token0, token1] = sortTokens(path[i - 1], path[i]);
        bool inputIsToken0 = (path[i - 1] == token0);
        u256 reserveIn = inputIsToken0 ? pair.reserve0 : pair.reserve1;
        u256 reserveOut = inputIsToken0 ? pair.reserve1 : pair.reserve0;
        amounts[i - 1] = getAmountIn(amounts[i], reserveIn, reserveOut);
    }
    return amounts;
}

std::pair<VibeSwap::Token, VibeSwap::Token> VibeSwap::sortTokens(const Token &tokenA, const Token &tokenB) {
    if (tokenA.empty() || tokenB.empty()) {
        throw std::invalid_argument("Token cannot be empty");
    }
    if (tokenA == tokenB) {
        throw std::invalid_argument("Tokens must be distinct");
    }
    return (tokenA < tokenB) ? std::make_pair(tokenA, tokenB) : std::make_pair(tokenB, tokenA);
}

u256 VibeSwap::integerSqrt(const u256 &value) {
    if (value == 0) {
        return 0;
    }

    u256 low = 0;
    u256 high = 1;
    while (high * high <= value) {
        high <<= 1;
    }

    while (low + 1 < high) {
        u256 mid = (low + high) / 2;
        if (mid * mid <= value) {
            low = mid;
        } else {
            high = mid;
        }
    }

    return low;
}

std::string VibeSwap::pairKey(const Token &token0, const Token &token1) const {
    return toString(token0) + "|" + toString(token1);
}

EthAddress VibeSwap::makePairAddress(const Token &token0, const Token &token1) {
    std::string key = toString(token0) + "|" + toString(token1);
    std::hash<std::string> hasher;
    uint64_t h1 = hasher(key);
    uint64_t h2 = hasher("vibeswap:" + key);
    uint64_t h3 = hasher(key + ":vibeswap");

    std::array<uint8_t, 20> bytes{};
    for (size_t i = 0; i < 8; ++i) {
        bytes[i] = static_cast<uint8_t>((h1 >> (8 * (7 - i))) & 0xFF);
    }
    for (size_t i = 0; i < 8; ++i) {
        bytes[8 + i] = static_cast<uint8_t>((h2 >> (8 * (7 - i))) & 0xFF);
    }
    for (size_t i = 0; i < 4; ++i) {
        bytes[16 + i] = static_cast<uint8_t>((h3 >> (8 * (7 - i))) & 0xFF);
    }

    return EthAddress(bytes);
}

VibeSwap::Pair &VibeSwap::getOrCreatePair(const Token &tokenA, const Token &tokenB) {
    auto [token0, token1] = sortTokens(tokenA, tokenB);
    auto key = pairKey(token0, token1);
    auto [it, inserted] = pairs_.emplace(key, Pair{token0, token1, makePairAddress(token0, token1)});
    return it->second;
}

VibeSwap::Pair &VibeSwap::getPairChecked(const Token &tokenA, const Token &tokenB) {
    auto [token0, token1] = sortTokens(tokenA, tokenB);
    auto key = pairKey(token0, token1);
    auto it = pairs_.find(key);
    if (it == pairs_.end()) {
        throw std::runtime_error("Pair not found for tokens " + token0 + " and " + token1);
    }
    return it->second;
}

const VibeSwap::Pair &VibeSwap::getPairChecked(const Token &tokenA, const Token &tokenB) const {
    auto [token0, token1] = sortTokens(tokenA, tokenB);
    auto key = pairKey(token0, token1);
    auto it = pairs_.find(key);
    if (it == pairs_.end()) {
        throw std::runtime_error("Pair not found for tokens " + token0 + " and " + token1);
    }
    return it->second;
}

void VibeSwap::ensureBalance(const EthAddress &owner, const Token &token, const u256 &amount) const {
    if (balanceOf(owner, token) < amount) {
        throw std::runtime_error("Insufficient balance for token " + token);
    }
}

void VibeSwap::debit(const EthAddress &owner, const Token &token, const u256 &amount) {
    auto &balance = balances_[owner][token];
    if (balance < amount) {
        throw std::runtime_error("Insufficient balance for token " + token);
    }
    balance -= amount;
}

void VibeSwap::credit(const EthAddress &owner, const Token &token, const u256 &amount) {
    balances_[owner][token] += amount;
}

std::vector<u256> VibeSwap::swapWithKnownAmounts(const EthAddress &trader,
                                                 const std::vector<u256> &amounts,
                                                 const std::vector<Token> &path,
                                                 const EthAddress &to) {
    if (path.size() < 2) {
        throw std::invalid_argument("Path must have at least two tokens");
    }
    if (amounts.size() != path.size()) {
        throw std::invalid_argument("Amounts/path size mismatch");
    }

    ensureBalance(trader, path.front(), amounts.front());
    debit(trader, path.front(), amounts.front());

    for (size_t i = 0; i + 1 < path.size(); ++i) {
        auto &pair = getPairChecked(path[i], path[i + 1]);
        auto [token0, token1] = sortTokens(path[i], path[i + 1]);
        bool inputIsToken0 = (path[i] == token0);
        u256 amountIn = amounts[i];
        u256 amountOut = amounts[i + 1];

        u256 &reserveIn = inputIsToken0 ? pair.reserve0 : pair.reserve1;
        u256 &reserveOut = inputIsToken0 ? pair.reserve1 : pair.reserve0;

        if (amountOut >= reserveOut) {
            throw std::runtime_error("Insufficient liquidity for swap");
        }

        reserveIn += amountIn;
        reserveOut -= amountOut;
    }

    credit(to, path.back(), amounts.back());
    return amounts;
}
