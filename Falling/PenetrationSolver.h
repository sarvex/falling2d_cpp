#include <vector>
#include "ContactGenerator.h"
#include "Shapes.h"
#include "island.h"

class PenetrationSolver
{
private:
public:
	static void solve(std::vector<Contact *> &scs);
	static void solve(Island *isl, bool implode);
	static void applyPositionChange(Contact *c,float *ch,float *ah);
	static void applyPositionChangePerLevel(Contact *c,float *ch,float *ah, bool implode);
	static void solveRelax(std::vector<Contact *> &scs);
	static void applyPositionChangeRelax(Contact *c,float *ch);
};
