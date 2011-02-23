// Falling.cpp�: d�finit les fonctions export�es pour l'application DLL.
//

#include "stdafx.h"
#include "Falling.h"
#include "DoubleLinkedList.h"
World::World()
{ }
World::~World()
{ }
												
void World::addObject(RigidBody *s)
{
	addWaitingQueue.push(s);
}

void World::removeObject(RigidBody *s)
{	
	removeWaitingQueue.push(s);
}

void World::dumpAddDelete()
{				
	while(!removeWaitingQueue.empty())
	{
		RigidBody * r = removeWaitingQueue.top();
		ca.deleteObject(r->getShape());
		removeWaitingQueue.pop();
		std::remove(objs.begin(),objs.end(),r);
	}
	while(!addWaitingQueue.empty())
	{
		RigidBody * r = addWaitingQueue.top();
		ca.addObject(r->getShape());
		addWaitingQueue.pop();
		objs.push_back(r);
	}
}

void World::notifyObjectMoved(RigidBody *s)
{ 
	dumpAddDelete();
	ca.notifyObjectMoved(s->getShape()); 
}

void World::checkSleeps(float dt)
{
	for(unsigned int i = 0;i < objs.size();i++)
		if(!objs[i]->isSleeping())
			objs[i]->updateSleepState(dt);
}


// return type should be void
void World::solve(float dt)
{
	/*		  
	// TODO: REMOVE
	DoubleLinkedList<int> *s = new DoubleLinkedList<int>(0);
	DoubleLinkedList<int> *parc = s; 
	for(int i = 1; i < 6; i++)
	{
		parc->InsertAfter(i);
		parc = parc->getnext();
	} 
	parc = s;
	DoubleLinkedList<int> *cut1 = s->getnext(),
		*cut2 = s->getprev()->getprev();
	s->Split(cut1, cut2, 1000, 1000);
	parc = cut1;
	do
	{
		printf("%i - ", parc->getValue());
		parc = parc->getprev();
	} while (parc != cut1);
	printf("\n");		 
	parc = cut2;
	do
	{
		printf("%i - ", parc->getValue());
		parc = parc->getprev();
	} while (parc != cut2);
	 */
	// end TODO

	// add and remove objects now
	checkSleeps(dt);
	dumpAddDelete();
	VitessSolver::integrate(objs,dt);
	// solve distances (collision detection)
	std::vector<Collision *> colls;
	ca.solve(colls);
	if(colls.size())
	{
		std::vector<Contact *> ctcts;
		std::stack<Island *> isls,isls2;
		if(true)//penAlgorithm)
		{								
			// Build islands		
			Island::batchIslands(colls,isls);			
			//printf("NBR ISLS ==>  %i\n",isls.size());
				while(!isls.empty())
				{
					// TODO: ce transfert de pile est tr�s moche!
					isls2.push(isls.top());
					isls.top()->calculateStackLevels();
					isls.pop();
				}
				//		
				ContactGenerator::DeduceContactsDatas(colls,ctcts,dt);
				while(!isls2.empty())
				{
					PenetrationSolver::solve(isls2.top());
					isls2.pop();																									
				}
				PenetrationSolver::solve(ctcts);
		}
		else
		{								 
			ContactGenerator::DeduceContactsDatasOldAlgorithm(colls,ctcts,dt);	 
			PenetrationSolver::solve(ctcts);
		}
		ImpulseSolver::solve(ctcts,dt);
	}
}

