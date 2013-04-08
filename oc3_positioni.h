// This file is part of openCaesar3.
//
// openCaesar3 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// openCaesar3 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with openCaesar3.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __OPENCAESAR3_POSITION_H_INCLUDED__
#define __OPENCAESAR3_POSITION_H_INCLUDED__

#include "oc3_vector2.h"

class Point : public Vector2<int>
{
public:
    Point( const int x, const int y ) : Vector2<int>( x, y ) {}
    Point() : Vector2<int>( 0, 0 ) {}

    Point operator+(const Point& other) const { return Point( x + other.x, y + other.y ); }
};

class PointF : public Vector2<float>
{

};

class TilePos : public Vector2<int>
{
public:
    TilePos( const int i, const int j ) : Vector2<int>( i, j ) {}
    TilePos() : Vector2<int>( 0, 0 ) {}

    int getI() const { return x; }
    int getJ() const { return y; }

    TilePos& operator=(const TilePos& other) { set( other.x, other.y ); return *this; }
    TilePos operator+(const TilePos& other) const { return TilePos( x + other.x, y + other.y ); }
};
#endif // __NRP_POSITION_H_INCLUDED__
