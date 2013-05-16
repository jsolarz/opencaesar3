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
//
// Copyright 2012-2013 Gregoire Athanase, gathanase@gmail.com

#include "oc3_house.hpp"

#include "oc3_tile.hpp"
#include "oc3_house_level.hpp"
#include "oc3_gui_info_box.hpp"
#include "oc3_scenario.hpp"
#include "oc3_exception.hpp"
#include "oc3_workerhunter.hpp"
#include "oc3_immigrant.hpp"
#include "oc3_market.hpp"
#include "oc3_constructionmanager.hpp"
#include "oc3_resourcegroup.hpp"

class House::Impl
{
public:
  int picIdOffset;
  char desirability;
};

House::House(const int houseId) : Building( B_HOUSE ), _d( new Impl )
{
   _houseId = houseId;
   _d->picIdOffset = ( rand() % 10 > 6 ? 1 : 0 );
   HouseSpecHelper& helper = HouseSpecHelper::getInstance();
   _houseLevel = helper.getHouseLevel( houseId );
   _houseLevelSpec = &helper.getHouseLevelSpec(_houseLevel);
   _nextHouseLevelSpec = &helper.getHouseLevelSpec(_houseLevel+1);
   _name = _houseLevelSpec->getLevelName();
   _currentHabitants = 0;
   _d->desirability = -3;
   _fireLevel = 90;

   _goodStore.setMaxQty(10000);  // no limit
   _goodStore.setMaxQty(G_WHEAT, 100);
   _goodStore.setMaxQty(G_FISH, 0);
   _goodStore.setMaxQty(G_MEAT, 0);
   _goodStore.setMaxQty(G_FRUIT, 0);
   _goodStore.setMaxQty(G_VEGETABLE, 0);
   _goodStore.setMaxQty(G_POTTERY, 0);
   _goodStore.setMaxQty(G_FURNITURE, 0);
   _goodStore.setMaxQty(G_OIL, 0);
   _goodStore.setMaxQty(G_WINE, 0);

   // init the service access
   for (int i = 0; i<S_MAX; ++i)
   {
      // for every service type
      ServiceType service = ServiceType(i);
      _serviceAccessMap[service] = 0;
   }

   _update();
}

void House::timeStep(const unsigned long time)
{
   // _goodStockList[G_WHEAT]._currentQty -= _currentHabitants;  // to do once every month!
   if (time % 16 == 0)
   {
      // consume services
      for (int i = 0; i < S_MAX; ++i)
      {
         ServiceType service = (ServiceType) i;
         _serviceAccessMap[service] = std::max(_serviceAccessMap[service] - 1, 0);
      }

      // consume goods
      for (int i = 0; i < G_MAX; ++i)
      {
         GoodType goodType = (GoodType) i;
         int qty = std::max(_goodStore.getCurrentQty(goodType) - 1, 0);
         _goodStore.setCurrentQty(goodType, qty);
      }
   }

   if( time % 64 == 0 )
   {
     bool validate = _houseLevelSpec->checkHouse(*this);
     if (!validate)
     {
       levelDown();
     }
     else
     {
       validate = _nextHouseLevelSpec->checkHouse(*this);
       if( validate && _currentHabitants > 0 )
       {
          levelUp();
       }
     }

     _freeWorkersCount = _currentHabitants;

     int homeless = math::clamp( _currentHabitants - _maxHabitants, 0, 0xff );

     if( homeless > 0 )
     {
       _currentHabitants = math::clamp( _currentHabitants, 0, _maxHabitants );

       City& city = Scenario::instance().getCity();
       ImmigrantPtr im = Immigrant::create( city, BuildingPtr( this ), homeless );
     }
   }

   if( _currentHabitants > 0 )
     Building::timeStep( time );
}

SimpleGoodStore& House::getGoodStore()
{
   return _goodStore;
}


HouseLevelSpec& House::getLevelSpec()
{
   return *_houseLevelSpec;
}

void House::_tryUpdate_1_to_11_lvl( int level4grow, int startSmallPic, int startBigPic, const char desirability )
{       
  if( getSize() == 1 )
  {
    City& city = Scenario::instance().getCity();
    Tilemap& tmap = city.getTilemap();
    PtrTilesList tiles = tmap.getFilledRectangle( getTile().getIJ(), Size(2) );   
    bool mayGrow = true;

    for( PtrTilesList::iterator it=tiles.begin(); it != tiles.end(); it++ )
    {
      if( *it == NULL )
      {
        mayGrow = false;   //some broken, can't grow
        break;
      }

      HousePtr house = (*it)->get_terrain().getOverlay().as<House>();
      if( house != NULL && house->getLevelSpec().getHouseLevel() == level4grow )
      {
        if( house->getSize() > 1 )  //bigger house near, can't grow
        {
          mayGrow = false;
          break;
        }            
      }
      else
      {
        mayGrow = false; //no house near, can't grow
        break;
      }
    }

    if( mayGrow )
    {
      int sumHabitants = getNbHabitants();
      int sumFreeWorkers = _freeWorkersCount;
      PtrTilesList::iterator delIt=tiles.begin();
      delIt++; //don't remove himself
      for( ; delIt != tiles.end(); delIt++ )
      {
        HousePtr house = (*delIt)->get_terrain().getOverlay().as<House>();
        if( house.isValid() )
        {
          house->deleteLater();
          house->_currentHabitants = 0;
          sumHabitants += house->getNbHabitants();
          sumFreeWorkers += house->_freeWorkersCount;
        }
      }

      _currentHabitants = sumHabitants;
      _freeWorkersCount = sumHabitants;
      _houseLevelSpec = &HouseSpecHelper::getInstance().getHouseLevelSpec(_houseLevel);
      _nextHouseLevelSpec = &HouseSpecHelper::getInstance().getHouseLevelSpec(_houseLevel+1);

      _update();
      _updateDesirabilityInfluence( false );
      _size++;
      build( getTile().getIJ() );      
    }
  }

  _updateDesirabilityInfluence( false );
  _d->desirability = desirability;       
  _updateDesirabilityInfluence( true );

  bool bigSize = getSize() > 1;
  _houseId = bigSize ? startBigPic : startSmallPic; 
  _d->picIdOffset = bigSize ? 0 : ( (rand() % 10 > 6) ? 1 : 0 );
}


void House::levelUp()
{
  _houseLevel++;   
  _d->picIdOffset = 0;
     
  switch (_houseLevel)
  {
  case 1:
     _houseId = 1;
     _d->desirability = -3;
     break;
  case 2: _tryUpdate_1_to_11_lvl( 1, 1, 5, -3);
  break;
  
  case 3: _tryUpdate_1_to_11_lvl( 2, 3, 6, -3 ); 
  break;
  
  case 4: _tryUpdate_1_to_11_lvl( 3, 7, 11, -2 );
  break;
  
  case 5: _tryUpdate_1_to_11_lvl( 4, 9, 12, -2 );
  break;

  case 6: _tryUpdate_1_to_11_lvl( 5, 13, 17, -2 );
  break;

  case 7: _tryUpdate_1_to_11_lvl( 6, 15, 18, -2 );
  break;

  case 8: _tryUpdate_1_to_11_lvl( 7, 19, 23, -1 );
  break;

  case 9: _tryUpdate_1_to_11_lvl( 8, 21, 24, -1 );
  break;

  case 10: _tryUpdate_1_to_11_lvl( 9, 25, 29, 0 );
  break;

  case 11: _tryUpdate_1_to_11_lvl( 10, 27, 30, 0 );
  break;
  }

  _houseLevelSpec = &HouseSpecHelper::getInstance().getHouseLevelSpec(_houseLevel);
  _nextHouseLevelSpec = &HouseSpecHelper::getInstance().getHouseLevelSpec(_houseLevel+1);

  _update();
}

void House::_tryDegrage_11_to_2_lvl( int smallPic, int bigPic, const char desirability )
{
  bool bigSize = getSize() > 1;
  _houseId = bigSize ? bigPic : smallPic;
  _d->picIdOffset = bigSize ? 0 : ( rand() % 10 > 6 ? 1 : 0 );

  _updateDesirabilityInfluence( false );
  _d->desirability = desirability;
  _updateDesirabilityInfluence( true );
}

void House::levelDown()
{
   _houseLevel--;
   _houseLevelSpec = &HouseSpecHelper::getInstance().getHouseLevelSpec(_houseLevel);
   _nextHouseLevelSpec = &HouseSpecHelper::getInstance().getHouseLevelSpec(_houseLevel+1);

   switch (_houseLevel)
   {
   case 1:
     {
       _houseId = 1;
       _d->picIdOffset = ( rand() % 10 > 6 ? 1 : 0 );

       City& city = Scenario::instance().getCity();
       Tilemap& tmap = city.getTilemap();   

       if( getSize() > 1 )
       {
         _updateDesirabilityInfluence( false );

         PtrTilesList tiles = tmap.getFilledRectangle( getTile().getIJ(), Size(2) );      
         PtrTilesList::iterator it=tiles.begin();
         int peoplesPerHouse = getNbHabitants() / 4;
         _currentHabitants = peoplesPerHouse;
         it++; //no destroy himself
         for( ; it != tiles.end(); it++ )
         {
           HousePtr house = ConstructionManager::getInstance().create( B_HOUSE ).as<House>();
           house->build( (*it)->getIJ() );
           house->_currentHabitants = peoplesPerHouse;
           house->_update();
         }

         _size = 1;
         _updateDesirabilityInfluence( true );
       }
     }
   break;
   
   case 2: _tryDegrage_11_to_2_lvl( 1, 5, -3 );
   break;
   
   case 3: _tryDegrage_11_to_2_lvl( 3, 6, -3 );
   break;
   
   case 4: _tryDegrage_11_to_2_lvl( 7, 11, -2 );
   break;
   
   case 5: _tryDegrage_11_to_2_lvl( 9, 12, -2 );
   break;

   case 6: _tryDegrage_11_to_2_lvl( 13, 17, -2 );
   break;

   case 7: _tryDegrage_11_to_2_lvl( 15, 18, -2 );
   break;

   case 8: _tryDegrage_11_to_2_lvl( 19, 23, -1 );
   break;

   case 9: _tryDegrage_11_to_2_lvl( 21, 23, -1 );
   break;

   case 10: _tryDegrage_11_to_2_lvl( 25, 29, 0 );
   break;

   case 11: _tryDegrage_11_to_2_lvl( 27, 30, 0 );
   break;
   }

   _update();
}

void House::buyMarket( ServiceWalkerPtr walker)
{
   // std::cout << "House buyMarket" << std::endl;
   MarketPtr market = walker->getBase().as<Market>();
   GoodStore& marketStore = market->getGoodStore();

   SimpleGoodStore &houseStore = getGoodStore();
   for (int i = 0; i < G_MAX; ++i)
   {
      GoodType goodType = (GoodType) i;
      int houseQty = houseStore.getCurrentQty(goodType);
      int houseSafeQty = _houseLevelSpec->computeMonthlyConsumption(*this, goodType)
                         + _nextHouseLevelSpec->computeMonthlyConsumption(*this, goodType);
      int marketQty = marketStore.getCurrentQty(goodType);
      if (houseQty < houseSafeQty && marketQty > 0)
      {
         int qty = std::min(houseSafeQty - houseQty, marketQty);
         GoodStock stock(goodType, qty);
         marketStore.retrieve(stock, qty);
         houseStore.store(stock, qty);
      }
   }
}

void House::applyService( ServiceWalkerPtr walker )
{
  Building::applyService(walker);  // handles basic services, and remove service reservation

  ServiceType service = walker->getService();
  switch (service)
  {
  case S_WELL:
  case S_FOUNTAIN:
  case S_TEMPLE_NEPTUNE:
  case S_TEMPLE_CERES:
  case S_TEMPLE_VENUS:
  case S_TEMPLE_MARS:
  case S_TEMPLE_MERCURE:
  case S_DOCTOR:
  case S_BARBER:
  case S_BATHS:
  case S_HOSPITAL:
  case S_SCHOOL:
  case S_LIBRARY:
  case S_COLLEGE:
  case S_THEATER:
  case S_AMPHITHEATER:
  case S_COLLOSSEUM:
  case S_HIPPODROME:
  case S_FORUM:
    setServiceAccess(service, 100);
    break;
  case S_MARKET:
    buyMarket(walker);
    break;
  case S_SENATE:
  case S_TEMPLE_ORACLE:  
  case S_ENGINEER:
  case S_PREFECT:
  case S_MAX:
    break;
  case S_WORKERS_HUNTER:
    {
      if( !_freeWorkersCount )
        break;

      SmartPtr< WorkersHunter > hunter = walker.as<WorkersHunter>();
      if( hunter.isValid() )
      {
          int hiredWorkers = math::clamp( _freeWorkersCount, 0, hunter->getWorkersNeeded() );
          _freeWorkersCount -= hiredWorkers;
          hunter->hireWorkers( hiredWorkers );
      }
    }
  break;
  }
}

float House::evaluateService(ServiceWalkerPtr walker)
{
   float res = 0.0;
   ServiceType service = walker->getService();
   if (_reservedServices.count(service) == 1)
   {
      // service is already reserved
      return 0.0;
   }

   switch(service)
   {
   case S_ENGINEER:
   {
      res = _damageLevel;
   }
   break;
   case S_PREFECT:
     {
       res = _fireLevel;
     }
   break;

   case S_MARKET:
     {
       MarketPtr market = walker->getBase().as<Market>();
       GoodStore &marketStore = market->getGoodStore();
       SimpleGoodStore &houseStore = getGoodStore();
       for (int i = 0; i < G_MAX; ++i)
       {
          GoodType goodType = (GoodType) i;
          int houseQty  = houseStore.getCurrentQty(goodType);
          int houseSafeQty = _houseLevelSpec->computeMonthlyConsumption(*this, goodType)
                             + _nextHouseLevelSpec->computeMonthlyConsumption(*this, goodType);
          int marketQty = marketStore.getCurrentQty(goodType);
          if (houseQty < houseSafeQty && marketQty > 0)
          {
             res += std::min(houseSafeQty - houseQty, marketQty);
          }
       }
     }
   break;

   case S_WORKERS_HUNTER:
     {
        res = (float)_freeWorkersCount;
        _reservedServices.erase( S_WORKERS_HUNTER );
     }
   break;

   default:
     {
       return _houseLevelSpec->evaluateServiceNeed(*this, service);
     }
   break;
   }

   // std::cout << "House evaluateService " << service << "=" << res << std::endl;

   return res;
}

bool House::hasServiceAccess(const ServiceType service)
{
   bool res = (_serviceAccessMap[service] > 0);
   return res;
}

int House::getServiceAccess(const ServiceType service)
{
   int res = _serviceAccessMap[service];
   return res;
}

void House::setServiceAccess(const ServiceType service, const int access)
{
   _serviceAccessMap[service] = access;
}

int House::getNbHabitants()
{
   return _currentHabitants;
}

int House::getMaxHabitants()
{
   return _maxHabitants;
}

int House::collectTaxes()
{
   int res = 0;
   if (_serviceAccessMap[S_FORUM] > 0)
   {
      // this house pays taxes
      res = _houseLevelSpec->getTaxRate() * _currentHabitants;
   }
   return res;
}

void House::_update()
{
    Uint8 picId = ( _houseId == smallHovel && _currentHabitants == 0 ) ? 45 : (_houseId + _d->picIdOffset); 
    setPicture( Picture::load( ResourceGroup::housing, picId ) );
    _size = (_picture->get_surface()->w+2)/60;
    _maxHabitants = _houseLevelSpec->getMaxHabitantsByTile() * _size * _size;
}

Uint8 House::getMaxDistance2Road() const
{
  return 2;
}

void House::addHabitants( const Uint8 newHabitCount )
{
  _currentHabitants = (std::min)( _currentHabitants + newHabitCount, _maxHabitants );
  _update();
}

void House::destroy()
{
  int lostPeoples = _currentHabitants;
  _currentHabitants = _maxHabitants;

  if( lostPeoples > 0 )
  {
    City& city = Scenario::instance().getCity();
    Immigrant::create( city, BuildingPtr( this ), lostPeoples );
  }

  Building::destroy();
}

bool House::isWalkable() const
{
  return (_houseId == smallHovel && _currentHabitants == 0) ? true : false;
}

char House::getDesirabilityInfluence() const
{
  return _d->desirability;
}