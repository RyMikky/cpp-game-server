#pragma once

#include <optional>
#include <algorithm>

namespace geom {

    // вектор
    struct Vec2D {

        Vec2D() = default;
        Vec2D(double x_, double y_) : x(x_), y(y_) {}

        double x = 0.0;
        double y = 0.0;

        Vec2D& operator*=(double scale) {
            x *= scale;
            y *= scale;
            return *this;
        }
    };

    inline bool operator==(const Vec2D& lhs, const Vec2D& rhs) {
        return (lhs.x == rhs.x) && (lhs.y == rhs.y);
    }

    inline bool operator!=(const Vec2D& lhs, const Vec2D& rhs) {
        return !(lhs == rhs);
    }

    inline Vec2D operator*(Vec2D vec, double scale) {
        return vec *= scale;
    }

    inline Vec2D operator*(double scale, Vec2D vec) {
        return vec *= scale;
    }

    // точка 
    struct Point2D {

        Point2D() = default;
        Point2D(double x_, double y_) : x(x_), y(y_){}
        
        double x = 0.0;
        double y = 0.0;

        Point2D& operator+=(const Point2D& rhs) {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        Point2D& operator+=(const Vec2D& rhs) {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }
    };

    inline bool operator==(const Point2D& lhs, const Point2D& rhs) {
        return (lhs.x == rhs.x) && (lhs.y == rhs.y);
    }

    inline bool operator!=(const Point2D& lhs, const Point2D& rhs) {
        return !(lhs == rhs);
    }

    inline Point2D operator+(Point2D point, const Point2D& vec) {
        return point += vec;
    }

    inline Point2D operator+(const Point2D& vec, Point2D point) {
        return point += vec;
    }

    struct Rect {
        // прямоугольник 
        double x, y;
        double w, h;
    }; 

    struct LineSegment {
    // Предполагаем, что x1 <= x2
    double x1, x2;
    };

    inline std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2) {
        double left = std::max(s1.x1, s2.x1);
        double right = std::min(s1.x2, s2.x2);

        if (right < left) {
            return std::nullopt;
        }

        // Здесь использована возможность C++-20 - объявленные 
        // инициализаторы (designated initializers).
        // Узнать о ней подробнее можно на сайте cppreference:
        // https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers
        return LineSegment{.x1 = left, .x2 = right};
    } 

    // Используем тот факт, что прямоугольники пересекаются, если пересекаются их проекции на оси

    // Вычисляем проекции на оси
    inline LineSegment ProjectX(Rect r) {
        return LineSegment{.x1 = r.x, .x2 = r.x + r.w};
    }

    inline LineSegment ProjectY(Rect r) {
        return LineSegment{.x1 = r.y, .x2 = r.y + r.h};
    }

    inline std::optional<Rect> Intersect(Rect r1, Rect r2) {
        auto px = Intersect(ProjectX(r1), ProjectX(r2));
        auto py = Intersect(ProjectY(r1), ProjectY(r2));

        if (!px || !py) {
            return std::nullopt;
        }

        // Составляем из проекций прямоугольник
        return Rect{.x = px->x1, .y = py->x1, 
                    .w = px->x2 - px->x1, .h = py->x2 - py->x1};
    }

} // namespace geom 