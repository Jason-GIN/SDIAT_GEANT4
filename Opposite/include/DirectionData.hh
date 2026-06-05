#ifndef Test02DirectionData_h
#define Test02DirectionData_h 1

#include "G4ThreeVector.hh"
#include <cstddef>

namespace Test02
{

class DirectionData {
public:
    
    static constexpr std::size_t NUM_DIRECTIONS = 13963;
    
    static const G4ThreeVector* getDirectionS();
    static G4ThreeVector getDirection(std::size_t index);
    
    static std::size_t getNumDirections() { return NUM_DIRECTIONS; }
    
    static G4ThreeVector getRandomDirection();


private:
    
    static const G4ThreeVector directions[NUM_DIRECTIONS];

};

}

#endif
