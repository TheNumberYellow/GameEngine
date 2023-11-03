#pragma once

#include "Math.h"

#include <sstream>  

struct Ray
{
    Ray() {}
    Ray(Vec3f point, Vec3f direction) : point(point), direction(direction) {}
    Vec3f point;
    Vec3f direction;
};

struct Line
{
    Line() {}
    Line(Vec3f point, Vec3f direction) : point(point), direction(direction) {}
    Vec3f point;
    Vec3f direction;
};

struct Plane
{
    Plane() {}
    Plane(Vec3f center, Vec3f normal) : center(center), normal(normal) {}
    Vec3f center;
    Vec3f normal;
};

struct LineSegment
{
    LineSegment() {}
    LineSegment(Vec3f a, Vec3f b) : a(a), b(b) {}
    Vec3f a, b;
};

struct Rect
{
    Rect() : location(Vec2f(0.0f, 0.0f)), size(Vec2f(0.0f, 0.0f)) {}

    Rect(Vec2f position, Vec2f size)
    {
        this->location = position;
        this->size = size;
    }

    bool Contains(Vec2f point)
    {
        return point.x > location.x && point.y > location.y &&
            point.x < (location + size).x && point.y < (location + size).y;
    }

    bool Contains(Rect other)
    {
        return (Contains(other.location) && Contains(other.location + other.size));
    }

    bool Overlaps(Rect other)
    {
        return (Contains(other.location) || Contains(other.location + other.size));
    }

    // Expand the rect to include this point
    void expand(Vec2f point)
    {
        if (point.x < location.x) location.x = point.x;
        if (point.y < location.y) location.y = point.y;

        if (point.x > (location.x + size.x)) size.x = point.x - location.x;
        if (point.y > (location.y + size.y)) size.y = point.y - location.y;
    }

    friend bool operator==(const Rect& lhs, const Rect& rhs)
    {
        return lhs.location == rhs.location && lhs.size == rhs.size;
    }

    Vec2f location, size;
};

struct Recti
{
    Recti() : location(Vec2i(0, 0)), size(Vec2i(0, 0)) {}
    Recti(Vec2i bottomLeft, Vec2i topRight)
    {
        location = bottomLeft;
        size = topRight - bottomLeft;
    }

    bool contains(Vec2i point)
    {
        return point.x > location.x && point.y > location.y &&
            point.x < (location + size).x&& point.y < (location + size).y;
    }

    Vec2i location, size;
};

struct Quad
{

};

struct AABB
{
    AABB() {}
    AABB(Vec3f min, Vec3f max) { this->min = min; this->max = max; }

    bool Contains(Vec3f p)
    {
        return p >= min && p <= max;
    }

    Vec3f min;
    Vec3f max;
};