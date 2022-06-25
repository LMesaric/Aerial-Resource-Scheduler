#pragma once

#include <cassert>
#include <iostream>
#include <ostream>
#include <type_traits>
#include <vector>
#include <span>

#define assert_x_bounds assert(((void) "Index out of bounds", (x) < (theX)))


template<typename T>
class Proxy2 {
    std::size_t theX;
    std::size_t theY;

    std::span<T> theData;

public:
    Proxy2(T *aData, std::size_t x, std::size_t y) : theX{x}, theY{y}, theData{aData, x * y} {}

    inline std::span<const T> operator[](std::size_t x) const noexcept {
        assert_x_bounds;
        return std::span{theData.data() + x * theY, theY};
    }

    inline std::span<T> operator[](std::size_t x) noexcept {
        assert_x_bounds;
        return std::span{theData.data() + x * theY, theY};
    }
};


template<typename T>
class Matrix2 {
    std::vector<T> theData{};

    std::size_t theX{};
    std::size_t theY{};

public:
    Matrix2() = default;

    Matrix2(std::size_t x, std::size_t y) : theX{x}, theY{y} {
        theData.resize(theX * theY);
    }

    inline std::span<const T> operator[](std::size_t x) const noexcept {
        assert_x_bounds;
        return std::span<const T>{theData.data() + x * theY, theY};
    }

    inline std::span<T> operator[](std::size_t x) noexcept {
        assert_x_bounds;
        return std::span<T>{theData.data() + x * theY, theY};
    }

    inline void fill(const T &aValue) {
        std::fill(theData.begin(), theData.end(), aValue);
    }

    [[nodiscard]] std::size_t getX() const {
        return theX;
    }

    [[nodiscard]] std::size_t getY() const {
        return theY;
    }

    [[nodiscard]] const std::vector<T> &getRawData() const {
        return theData;
    }

    friend std::ostream &operator<<(std::ostream &os, const Matrix2<T> &aMatrix) {
        for (std::size_t x = 0; x < aMatrix.theX; ++x) {
            for (std::size_t y = 0; y < aMatrix.theY; ++y) {
                if constexpr (std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t>) {
                    os << +aMatrix[x][y];
                } else {
                    os << aMatrix[x][y];
                }
                os << ' ';
            }
            os << std::endl;
        }
        return os;
    }
};


template<typename T>
class Matrix3 {
    std::vector<T> theData{};

    std::size_t theX{};
    std::size_t theY{};
    std::size_t theZ{};

    std::size_t theYZ{theY * theZ};

public:
    Matrix3() = default;

    Matrix3(std::size_t x, std::size_t y, std::size_t z) :
            theX{x}, theY{y}, theZ{z} {
        theData.resize(theX * theY * theZ);
    }

    inline Proxy2<const T> operator[](std::size_t x) const noexcept {
        assert_x_bounds;
        return Proxy2<const T>{theData.data() + x * theYZ, theY, theZ};
    }

    inline Proxy2<T> operator[](std::size_t x) noexcept {
        assert_x_bounds;
        return Proxy2<T>{theData.data() + x * theYZ, theY, theZ};
    }

    inline void fill(const T &aValue) {
        std::fill(theData.begin(), theData.end(), aValue);
    }

    [[nodiscard]] std::size_t getX() const {
        return theX;
    }

    [[nodiscard]] std::size_t getY() const {
        return theY;
    }

    [[nodiscard]] std::size_t getZ() const {
        return theZ;
    }

    [[nodiscard]] const std::vector<T> &getRawData() const {
        return theData;
    }

    friend std::ostream &operator<<(std::ostream &os, const Matrix3<T> &aMatrix) {
        for (std::size_t x = 0; x < aMatrix.theX; ++x) {
            os << "x = " << x << std::endl;
            for (std::size_t y = 0; y < aMatrix.theY; ++y) {
                for (std::size_t z = 0; z < aMatrix.theZ; ++z) {
                    if constexpr (std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t>) {
                        os << +aMatrix[x][y][z];
                    } else {
                        os << aMatrix[x][y][z];
                    }
                    os << ' ';
                }
                os << std::endl;
            }
            os << std::endl;
        }
        return os;
    }
};
