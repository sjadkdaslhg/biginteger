#pragma once


#include <cstdint>
#include <vector>
#include <string>
#include <cctype>
#include <stdexcept>
#include <ostream>
#include <utility>


/**
 * BigInteger 类
 * 可以表示任意精度的整数
 * @author Hu
 */
class BigInteger {
    /**
     * 符号位，
     * -1 表示负整数
     * 0 表示整数 0
     * 1 表示正整数
     */
    int sign;

    /*
     * 绝对值
     * 使用小端序
     * num[0] 表示最低位
     */
    std::vector<uint32_t> num;

public:
    /**
     * 字符串构造函数
     * 接受十进制整数的字符串表示
     * val[0] 可以使用 '-' 或 '+' 指定符号
     * @param val 十进制整数的字符串表示
     */
    explicit BigInteger(const std::string& val);

    /**
     * 将 BigInteger 转换回字符串十进制表示的整数
     * @return 整数的字符串十进制表示
     */
    [[nodiscard]] std::string toString() const;

    /**
     * 重载 += 运算符
     * @param right 右操作数
     * @return 调用 operator+= 对象的引用
     */
    BigInteger& operator+=(const BigInteger& right);

    /**
     * 重载 -= 运算符
     * @param right 右操作数
     * @return 调用 operator-= 对象的引用
     */
    BigInteger& operator-=(const BigInteger& right);


    /**
     * 重载 *= 运算符
     * @param right 右操作数
     * @return 调用 operator *= 对象的引用
     */
    BigInteger& operator*=(const BigInteger& right);

    // 拷贝构造函数
    BigInteger(const BigInteger& other);

    // 其他简单运算符重载
    BigInteger& operator=(const BigInteger& other);

    BigInteger& operator=(BigInteger&& other) noexcept;

    friend bool operator==(const BigInteger& left, const BigInteger& right);

    friend bool operator<(const BigInteger& left, const BigInteger& right);
};


inline BigInteger::BigInteger(const std::string& val) {
    // 空字符串表示 0
    if (val.empty()) {
        this->sign = 0;
        return;
    }

    // 处理 val[0] 表示符号的情况
    size_t start = 0;
    this->sign = 1;
    if (val[0] == '-') {
        this->sign = -1;
        start = 1;
    } else if (val[0] == '+')
        start = 1;
    else if (!std::isdigit(static_cast<unsigned char>(val[0])))
        throw std::invalid_argument("Illegal embedded sign character");

    // 检查所有字符
    for (size_t i = start; i < val.length(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(val[i])))
            throw std::invalid_argument("Illegal character");

    // 跳过高位的 0
    while (start < val.length() && val[start] == '0')
        ++start;
    if (start == val.length()) {
        this->sign = 0;
        return;
    }

    // 霍纳法则
    // 先取出最高位放入 num[0]
    // 之后每次取出 9 位
    // 与 num 表示的临时数字的 10^9 倍相加
    num.reserve((val.length() - start) / 9 + 1);
    size_t first = (val.length() - start) % 9;
    if (first == 0)
        first = 9;
    num.push_back(static_cast<uint32_t>(std::stoull(val.substr(start, first))));
    start += first;
    while (start < val.length()) {
        uint64_t temp = std::stoull(val.substr(start, 9));
        start += 9;
        for (uint32_t& limb : num) {
            temp += static_cast<uint64_t>(limb) * static_cast<uint64_t>(1000000000);
            limb = static_cast<uint32_t>(temp); // static_cast<uint32_t> 等价于对 2^32 取余
            temp >>= 32; // 右移 32 位等价于除以 2^32
        }
        if (temp)
            num.push_back(static_cast<uint32_t>(temp));
    }
}


inline std::string BigInteger::toString() const {
    if (sign == 0)
        return "0";

    // 将 2^32 进制表示的整数转换为 10^9 进制表示
    // 对 10^9 进行长除法
    std::vector<uint32_t> copy = num; // 数组会被改变
    std::vector<uint32_t> blocks;
    while (!copy.empty()) {
        uint64_t temp = 0;
        for (int64_t i = static_cast<int64_t>(copy.size()) - 1; i >= 0; --i) {
            const uint64_t current = (temp << 32) | copy[i]; // (temp << 32) | copy[i] 等价于 (temp << 32) + copy[i]
            copy[i] = static_cast<uint32_t>(current / static_cast<uint64_t>(1000000000));
            temp = current % static_cast<uint64_t>(1000000000);
        }
        blocks.push_back(static_cast<uint32_t>(temp));
        while (!copy.empty() && copy.back() == 0)
            copy.pop_back();
    }

    // blocks 数组存放了整数的十进制表示每 9 位的切分
    // 将 blocks 数组转换为字符串
    std::string result;
    if (sign < 0)
        result += '-';
    result += std::to_string(blocks.back()); // 最高位不需要补 0
    for (int64_t i = static_cast<int64_t>(blocks.size()) - 2; i >= 0; --i) {
        std::string temp = std::to_string(blocks[i]);
        result.append(9 - temp.size(), '0');
        result += temp;
    }
    return result;
}


/**
 * 辅助函数
 * 计算两个 num 数组的和
 * left 的 num 数组会被修改
 * @param left 左操作数的 num 数组
 * @param right 右操作数的 num 数组
 */
inline void addToLeft(std::vector<uint32_t>& left, const std::vector<uint32_t>& right) {
    size_t idx = 0;
    uint64_t temp = 0;
    while (idx < left.size()) {
        temp += static_cast<uint64_t>(left[idx]);
        if (idx < right.size())
            temp += static_cast<uint64_t>(right[idx]);
        left[idx] = static_cast<uint32_t>(temp);
        temp >>= 32;
        ++idx;
    }
    while (idx < right.size()) {
        temp += static_cast<uint64_t>(right[idx]);
        left.push_back(static_cast<uint32_t>(temp));
        temp >>= 32;
        ++idx;
    }
    while (temp) {
        left.push_back(static_cast<uint32_t>(temp));
        temp >>= 32;
        ++idx;
    }
}


/**
 * 辅助函数
 * 直接计算两个 num 数组的乘积
 * left 的 num 数组会被修改
 * @param left 左操作数的 num 数组
 * @param right 右操作数的 num 数组
 */
inline void ordinaryMultiplyToLeft(std::vector<uint32_t>& left, const std::vector<uint32_t>& right) {
    std::vector<uint32_t> result(left.size() + right.size(), 0);
    for (size_t i = 0; i < right.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < left.size(); j++) {
            const uint64_t temp = static_cast<uint64_t>(left[j]) * static_cast<uint64_t>(right[i]) + carry + static_cast<uint64_t>(result[i + j]);
            result[i + j] = static_cast<uint32_t>(temp);
            carry = temp >> 32;
        }
        result[i + left.size()] += carry;
    }
    left = std::move(result);
}


/**
 * 辅助函数
 * 比较 left 和 right 的绝对值大小
 * @param left 左操作数的 num 数组
 * @param right 右操作数的 num 数组
 * @return int8_t 类型整数，值为 -1 或 0 或 1，符号与 |left| - |right| 的符号相同
 */
inline int8_t compareNum(const std::vector<uint32_t>& left, const std::vector<uint32_t>& right) {
    if (left.size() < right.size())
        return -1;
    if (left.size() > right.size())
        return 1;
    for (size_t i = left.size(); i-- > 0; )
        if (left[i] != right[i])
            return left[i] < right[i] ? -1 : 1;
    return 0;
}


/**
 * 辅助函数
 * 计算两个 num 数组的差
 * left 的绝对值必须大于 right
 * left 的 num 数组会被修改
 * @param left 左操作数的 num 数组
 * @param right 右操作数的 num 数组
 */
inline void subtractToLeft(std::vector<uint32_t>& left, const std::vector<uint32_t>& right) {
    bool borrow = false;
    for (size_t i = 0; i < left.size(); ++i) {
        const int64_t sub = i < right.size() ? right[i] : 0;
        const int64_t current = static_cast<int64_t>(left[i]) - sub - static_cast<int64_t>(borrow);
        left[i] = static_cast<uint32_t>(current);
        borrow = current < 0;
    }
    while (!left.empty() && left.back() == 0)
        left.pop_back();
}


inline BigInteger& BigInteger::operator+=(const BigInteger& right) {
    // 根据 this 和 right 的符号分类处理
    if (right.sign == 0)
        return *this;
    if (sign == 0) {
        num = right.num;
        sign = right.sign;
        return *this;
    }
    if (sign == right.sign) {
        addToLeft(num, right.num);
        return *this;
    }

    // this 和 right 正负不同的情况最复杂
    // 结果的符号由绝对值大小决定
    const int8_t cmp = compareNum(num, right.num);
    if (cmp == 0) {
        num.clear();
        sign = 0;
        return *this;
    }
    if (cmp > 0) {
        subtractToLeft(num, right.num);
        return *this;
    }

    // this 的绝对值小于 right 的绝对值
    // 进行了数组复制
    std::vector<uint32_t> result = right.num;
    subtractToLeft(result, num);
    num = result;
    sign = right.sign;
    return *this;
}


inline BigInteger& BigInteger::operator-=(const BigInteger& right) {
    // 与 operator+= 类似
    if (right.sign == 0)
        return *this;
    if (sign == 0) {
        num = right.num;
        sign = -right.sign;
        return *this;
    }
    if (sign != right.sign) {
        addToLeft(num, right.num);
        return *this;
    }
    const int8_t cmp = compareNum(num, right.num);
    if (cmp == 0) {
        num.clear();
        sign = 0;
        return *this;
    }
    if (cmp > 0) {
        subtractToLeft(num, right.num);
        return *this;
    }
    std::vector<uint32_t> result = right.num;
    subtractToLeft(result, num);
    num = result;
    sign = -right.sign;
    return *this;
}


inline BigInteger& BigInteger::operator*=(const BigInteger& right) {
    if (sign == 0 || right.sign == 0) {
        num.clear();
        sign = 0;
        return *this;
    }
    ordinaryMultiplyToLeft(num, right.num);
    sign = sign * right.sign;
    return *this;
}


inline BigInteger operator+(BigInteger left, const BigInteger& right) {
    left += right;
    return left;
}

inline BigInteger operator-(BigInteger left, const BigInteger& right) {
    left -= right;
    return left;
}

inline BigInteger operator*(BigInteger left, const BigInteger& right) {
    left *= right;
    return left;
}


inline BigInteger::BigInteger(const BigInteger& other) {
    sign = other.sign;
    num = other.num;
}

// 重载 = 运算符
inline BigInteger& BigInteger::operator=(const BigInteger& other) {
    if (this == &other)
        return *this;
    sign = other.sign;
    num = other.num;
    return *this;
}

inline BigInteger& BigInteger::operator=(BigInteger&& other) noexcept {
    if (this == &other)
        return *this;
    sign = other.sign;
    num = std::move(other.num);
    other.sign = 0;
    return *this;
}


// 重载 << 运算符
inline std::ostream& operator<<(std::ostream& os, const BigInteger& obj) {
    os << obj.toString();
    return os;
}

// 重载 == 运算符
inline bool operator==(const BigInteger& left, const BigInteger& right) {
    if (left.sign != right.sign)
        return false;
    if (left.num.size() != right.num.size())
        return false;
    for (size_t i = 0; i < left.num.size(); ++i)
        if (left.num[i] != right.num[i])
            return false;
    return true;
}

// 重载 != 运算符
inline bool operator!=(const BigInteger& left, const BigInteger& right) {
    return !(left == right);
}

// 重载 < 运算符
inline bool operator<(const BigInteger& left, const BigInteger& right) {
    if (left.sign < right.sign)
        return true;
    if (left.sign > right.sign)
        return false;
    if (left.sign > 0)
        return compareNum(left.num, right.num) < 0;
    return compareNum(left.num, right.num) > 0;
}

// 重载其他比较运算符
// 都通过 operator<< 实现
inline bool operator>(const BigInteger& left, const BigInteger& right) {
    return right < left;
}
inline bool operator<=(const BigInteger& left, const BigInteger& right) {
    return !(left > right);
}
inline bool operator>=(const BigInteger& left, const BigInteger& right) {
    return !(left < right);
}
