#pragma once

#include <ostream>


// TODO Compare implementation to pow 2 sizes and bit shifts.

template<typename T>
class Matrix2 {
private:
    std::vector<T> theData{};

    std::size_t theX;
    std::size_t theY;

public:
    Matrix2(const std::size_t x, const std::size_t y) :
            theX{x}, theY{y} {
        theData.resize(theX * theY);
    }

    inline T const &operator()(const std::size_t x, const std::size_t y) const {
        if (x >= theX || y >= theY) {
            // TODO throw exception
        }
        return theData[x * theY + y];
    }

    inline T &operator()(const std::size_t x, const std::size_t y) {
        return const_cast<T &>(std::as_const(*this)(x, y));
    }

    [[nodiscard]] std::size_t getX() const {
        return theX;
    }

    [[nodiscard]] std::size_t getY() const {
        return theY;
    }

    friend std::ostream &operator<<(std::ostream &os, const Matrix2 &aMatrix) {
        for (std::size_t x = 0; x < aMatrix.theX; ++x) {
            for (std::size_t y = 0; y < aMatrix.theY; ++y) {
                os << +aMatrix(x, y) << ' ';
            }
            os << std::endl;
        }
        return os;
    }
};


template<typename T>
class Matrix3 {
private:
    std::vector<T> theData{};

    std::size_t theX;
    std::size_t theY;
    std::size_t theZ;

    std::size_t theYZ{theY * theZ};

public:
    Matrix3(const std::size_t x, const std::size_t y, const std::size_t z) :
            theX{x}, theY{y}, theZ{z} {
        theData.resize(theX * theY * theZ);
    }

    inline const T &operator()(const std::size_t x, const std::size_t y, const std::size_t z) const {
        if (x >= theX || y >= theY || z >= theZ) {
            // TODO throw exception
        }
        return theData[x * theYZ + y * theZ + z];
    }

    inline T &operator()(const std::size_t x, const std::size_t y, const std::size_t z) {
        return const_cast<T &>(std::as_const(*this)(x, y, z));
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

    friend std::ostream &operator<<(std::ostream &os, const Matrix3 &aMatrix) {
        for (std::size_t x = 0; x < aMatrix.theX; ++x) {
            os << "x = " << x << std::endl;
            for (std::size_t y = 0; y < aMatrix.theY; ++y) {
                for (std::size_t z = 0; z < aMatrix.theZ; ++z) {
                    os << +aMatrix(x, y, z) << ' ';
                }
                os << std::endl;
            }
            os << std::endl;
        }
        return os;
    }
};
