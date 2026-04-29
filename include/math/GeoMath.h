#pragma once
#include <cmath>

// 二维向量基础结构体
struct Vec2 {
    double x, y;

    constexpr Vec2(double _x = 0.0, double _y = 0.0) : x(_x), y(_y) {}

    // 基本算术运算符重载
    [[nodiscard]] constexpr Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
    [[nodiscard]] constexpr Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
    [[nodiscard]] constexpr Vec2 operator*(double s) const { return { x * s, y * s }; }
    [[nodiscard]] constexpr Vec2 operator/(double s) const { return { x / s, y / s }; }

    inline Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    inline Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }

    // 浮点数比较，使用 1e-6 作为容差
    [[nodiscard]] constexpr bool operator==(const Vec2& v) const {
        const double dx = x - v.x;
        const double dy = y - v.y;
        const double absDx = dx < 0.0 ? -dx : dx;
        const double absDy = dy < 0.0 ? -dy : dy;
        return absDx < 1e-6 && absDy < 1e-6;
    }

    // 向量长度及平方
    [[nodiscard]] inline double lengthSquared() const { return x * x + y * y; }
    [[nodiscard]] inline double length() const { return std::sqrt(lengthSquared()); }

    // 归一化向量，处理除零风险
    [[nodiscard]] inline Vec2 normalize() const {
        double len = length();
        return (len < 1e-8) ? Vec2(0, 0) : Vec2(x / len, y / len);
    }

    // 点乘与叉乘
    [[nodiscard]] constexpr double dot(const Vec2& v) const { return x * v.x + y * v.y; }
    [[nodiscard]] constexpr double cross(const Vec2& v) const { return x * v.y - y * v.x; }

    // 获取逆时针旋转 90 度的垂直向量
    [[nodiscard]] constexpr Vec2 perpendicular() const { return { -y, x }; }

    // 计算与另一个点的距离
    [[nodiscard]] inline double distanceTo(const Vec2& v) const { return (*this - v).length(); }
};

// 计算向量 OA 与 OB 的二维叉乘
// 结果大于0表示左转，小于0表示右转，等于0表示共线。常用于凸包计算。
[[nodiscard]] constexpr double crossProduct(const Vec2& O, const Vec2& A, const Vec2& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}