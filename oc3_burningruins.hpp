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

#ifndef __OPENCAESAR3_BURNINGRUINS_H_INCLUDE_
#define __OPENCAESAR3_BURNINGRUINS_H_INCLUDE_

#include "oc3_service_building.hpp"

class BurningRuins : public ServiceBuilding
{
public:
    BurningRuins();
    BurningRuins* clone() const;

    void deliverService();
    void timeStep(const unsigned long time);
    void burn();
    void build(const TilePos& pos );
    bool isWalkable() const;
    void destroy();

    float evaluateService(ServiceWalker &walker);
    void applyService(ServiceWalker &walker);
};

class BurnedRuins : public Building
{
public:
  BurnedRuins();
  LandOverlay* clone() const;

  void timeStep(const unsigned long time);
  bool isWalkable() const;
  void build( const TilePos& pos );
};


#endif