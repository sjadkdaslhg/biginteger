#include "biginteger.hpp"
#include <iostream>

int main() {
    std::string a_str, b_str;
    std::cin >> a_str >> b_str;
    BigInteger a{a_str}, b{b_str};
    std::cout << a + b << '\n';
}