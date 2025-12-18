#include "EIP3009Value.h"
#include "MachinePalCommon.h"

#include "Encoding.h"

EIP3009Value::EIP3009Value( const u256& val ) : value_( val ) {}

std::string EIP3009Value::toDecimal() const {
    return Encoding::u256ToDecimal( value_ );
}

std::string EIP3009Value::toDbString() const {
    return toDecimal();
}

EIP3009Value EIP3009Value::fromHexOrDecimal( const std::string& decStr ) {
    u256 val = Encoding::u256FromHexOrDecimal( decStr );
    return EIP3009Value( val );
}