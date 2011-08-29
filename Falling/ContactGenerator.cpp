/* Copyright (C) 2011 CROZET Sébastien
 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "stdafx.h"
#include "ContactGenerator.h"
#include <iostream>

namespace Falling
{
    void Contact::updateVelChange(Real t)
    {
        Real vFromAcc = s1->getParent()->getAcc() * t * normal;
        if(s2)
            vFromAcc -= s2->getParent()->getAcc() * t * normal;
        Real fakerest = 0.2;
        if(ABS(closingVelocity.getX()) < 0.1)
        {
            fakerest = 0;
        }
        desiredVelocityChange = -closingVelocity.getX() - fakerest /* restitution */ * (closingVelocity.getX() - vFromAcc);
    }
    
    void Contact::awakeIfNeeded()
    {
        if(!s2) return;
        bool s1sl = s1->getParent()->isSleeping();
        bool s2sl = s2->getParent()->isSleeping();
        if(s1sl || s2sl)
        {
            if(s1sl) // s2 not sleeping
                s1->getParent()->setAwake(true);
            if(s2sl)
                s2->getParent()->setAwake(true);
        }
    }
    
    void ContactGenerator::PrepareContactDatasForImpulseSolver(std::vector<Collision *> &collisions,Real)
    {
        for(unsigned int i=0; i<collisions.size(); i++)
        {
            Collision *c = collisions[i];
            int max = c->c.size();
            c->worstVelocityContact = 0,
            c->worstVelocityChangeAmount = 0.01;
            for(int j=0; j<max; j++)
            {
                Contact *cnt = c->cnts[j];
                /*
                 FIXME: use a more accurate value than -0.05…
                 */
                if(cnt->getPenetration() >= -0.05 && cnt->desiredVelocityChange > c->worstVelocityChangeAmount)
                {
                    c->worstVelocityChangeAmount = cnt->desiredVelocityChange;
                    c->worstVelocityContact = cnt;
                }
            }
        }
    }    
    
    void ContactGenerator::PrepareContactDatasInMatrix(Real dt, Collision *c, Real *&J, Real *&bounds, Real *&zeta, Real *&lambda, int *&idx)
    {
        for(std::vector<ContactBackup*>::iterator j = c->c.begin(); j != c->c.end(); j++)
        {
            ContactBackup *cb = *j;
            Shape *a = c->sa;
            Shape *b = c->sb;
            Point2D middle = Point2D::getMiddle(a->toGlobal(cb->relPtA), b->toGlobal(cb->relPtB));
            Vector2D norm = cb->normal;
            
            
            if(a->isFixed())
            {
                a = b;
                b = 0;
                norm.reflect();
            }
            else if(b->isFixed())
                b = 0;
            // get tangeant for friction
            Vector2D tangeant = Vector2D(-norm.getY(),norm.getX());
            // now calculate relative points of contact.
            Vector2D relp1 = a->toTranslatedInv(middle);
            Real relative_velocity = -(a->getParent()->getV() * norm - relp1.cross(Vector2D(0,0,a->getParent()->getOmega())) * norm);
            Vector2D relp2;
            if(b)
            {    
                relp2 = b->toTranslatedInv(middle);
                relative_velocity += b->getParent()->getV() * norm - relp2.cross(Vector2D(0,0,b->getParent()->getOmega())) * norm;
            }
            
            /*
             Prepare contact for the LCP solver.
             This is a normal constsraint: J = (-n -(r_1 * n) n (r_2 * n))
             */
            *(J++) = norm.getX();
            *(J++) = norm.getY();
            *(J++) = relp1.perp(norm); // perpendicular product to keep the z component only
            *(bounds++) = 0;
            *(bounds++) = MACHINE_MAX; // infinite
            *(idx++) = a->getParent()->getIslandIndex();
            if(b)
            {
                *(idx++) = b->getParent()->getIslandIndex();
                *(J++) = -norm.getX();
                *(J++) = -norm.getY();
                *(J++) = -relp2.perp(norm);
            }
            else
            {
                *(idx++) = -1;
                *(J++) = 0;
                *(J++) = 0;
                *(J++) = 0;
            }
            /*
             Coefficient to correct the penetration.
             Don't correct anything when it's visibly unnoticeable (<= 2 * PROXIMITY_AWARENESS)
             */
            Real extra_v = 0;
            if(cb->depth > 2 * PROXIMITY_AWARENESS)
                extra_v += 0.8 / dt * cb->depth; // apply an artificial force proportional to the penetration depth
            if(relative_velocity * relative_velocity > 2.0f * SLEEPLIMIT) // use a coefficient of restitution of 0 when the closing velocity is too small => better stability in stacks.
                extra_v += 1 / dt * relative_velocity * 0.5; // 0.5 = coefficient of restitution.
            *(zeta++) = extra_v;
            *(J++) = -tangeant.getX();
            *(J++) = -tangeant.getY();
            *(J++) = -relp1.perp(tangeant); // perpendicular product to keep the z component only
            
            *(bounds++) = -0.5 * (a->getParent()->getM() / a->get_total_number_of_contacts() + (b ? b->getParent()->getM() / b->get_total_number_of_contacts() : 0.0)) * G;
            *(bounds++) = 0.5 * (a->getParent()->getM() / a->get_total_number_of_contacts() + (b ? b->getParent()->getM() / b->get_total_number_of_contacts() : 0.0)) * G;
            *(idx++) = a->getParent()->getIslandIndex();
            if(b)
            {
                *(idx++) = b->getParent()->getIslandIndex();
                *(J++) = tangeant.getX();
                *(J++) = tangeant.getY();
                *(J++) = relp2.perp(tangeant);
            }
            else
            {
                *(idx++) = -1;
                *(J++) = 0;
                *(J++) = 0;
                *(J++) = 0;
            }
            *(zeta++) = 0.; // friction doesn't work.
            *(lambda++) = cb->lambda; // warm start the solver
            *(lambda++) = cb->frictionlambda; // warm start the solver
        }
    }
    
    void ContactGenerator::DeduceContactsDatas(std::vector<Collision *> &collisions, std::vector<Contact *> &cts,Real dt)
    {
        for(unsigned int i=0; i<collisions.size(); i++)
        {
            Collision *c = collisions[i];
            if(c->sa->getParent()->isSleeping() && c->sb->getParent()->isSleeping())
                continue; // do not overwrite cached datas
            int max = c->c.size();
            c->worstContact = 0;
            c->worstPenetrationAmount = 0.01;
            c->worstVelocityContact = 0;
            c->worstVelocityChangeAmount = 0.01;
            if(max)
            {
                c->cnts = new Contact *[max];
                for(int j=0; j<max; j++)
                {
                    Contact *cnt = new Contact();
                    Point2D absA = c->sa->toGlobal(c->c[j]->relPtA);
                    Point2D absB = c->sb->toGlobal(c->c[j]->relPtB);
                    cnt->s1 = c->sa;
                    cnt->s2 = c->sb;
                    cnt->absoluteContactPoint= Point2D::getMiddle(absA, absB);
                    cnt->normal = c->c[j]->normal;
                    cnt->setPenetration(c->c[j]->depth - 2.0 * PROXIMITY_AWARENESS); // normalise normal and return penetration depth
                    if(cnt->s1->isFixed())
                    {
                        cnt->s1 = cnt->s2;
                        cnt->s2 = 0;
                        cnt->normal.reflect();
                    }
                    else if(cnt->s2->isFixed())
                        cnt->s2 = 0;
                    // get tangeant
                    cnt->tangeant = Vector2D(-cnt->normal.getY(),cnt->normal.getX());
                    // now calculate closing velocity
                    cnt->relContactPoint[0] = cnt->s1->toTranslatedInv(cnt->absoluteContactPoint);
                    if(cnt->s2)
                        cnt->relContactPoint[1] =
                        cnt->s2->toTranslatedInv(cnt->absoluteContactPoint);
                    RigidBody *ra = cnt->s1->getParent();
                    Vector2D lin1 =
                    cnt->toLocal(Vector2D(0,0,ra->getOmega()).cross(cnt->relContactPoint[0])
                                 + ra->getV());
                    Vector2D lin2;
                    RigidBody *rb = (cnt->s2?cnt->s2->getParent():0);
                    cnt->lin1 = lin1.magnitude();
                    cnt->lin2 = 0;
                    if(rb)
                    {
                        lin2 = cnt->toLocal(
                                            Vector2D(0,0,rb->getOmega()).cross(cnt->relContactPoint[1])
                                            + rb->getV());
                        cnt->lin2 = lin2.magnitude();
                        cnt->closingVelocity = lin1 - lin2;
                    }
                    else if(cnt->s2)
                    {
                        Real lin2nfixed = (cnt->toLocal(Vector2D(0,0,cnt->s2->getParent()->getOmega()).cross(cnt->relContactPoint[1]) + cnt->s2->getParent()->getV())).magnitude();
                        cnt->closingVelocity = lin1 - lin2nfixed;
                    }
                    else cnt->closingVelocity = lin1;
                    cnt->updateVelChange(dt);
                    Vector2D dvel = ((cnt->relContactPoint[0] ^ cnt->normal) * ra->getInvI())^cnt->relContactPoint[0];
                    cnt->angin[0] = dvel * cnt->normal;
                    cnt->linin[0] = ra->getInvM();
                    cnt->dvel = cnt->angin[0] + cnt->linin[0];
                    Vector2D dvely = ((cnt->relContactPoint[0] ^ cnt->tangeant) * ra->getInvI())^cnt->relContactPoint[0];
                    cnt->dvely = dvely * cnt->tangeant + ra->getInvM();
                    if(cnt->s2)
                    {
                        dvel = ((cnt->relContactPoint[1] ^ cnt->normal) * cnt->s2->getParent()->getInvI())^cnt->relContactPoint[1];
                        cnt->angin[1] = dvel * cnt->normal;
                        cnt->linin[1] = cnt->s2->getParent()->getInvM();
                        cnt->dvel += cnt->angin[1] + cnt->linin[1];
                        dvely = ((cnt->relContactPoint[1] ^ cnt->tangeant) * cnt->s2->getParent()->getInvI())^cnt->relContactPoint[1];
                        cnt->dvely += dvely * cnt->tangeant + cnt->s2->getParent()->getInvM();
                    }
                    cnt->totalInertia = 1.0 / (cnt->angin[0] + cnt->linin[0] + (rb?cnt->angin[1] + cnt->linin[1] :0.0));
                    cnt->unitlinmov[0] = cnt->linin[0] * cnt->totalInertia;
                    cnt->unitangmov[0] = (((cnt->relContactPoint[0] ^ cnt->normal) * ra->getInvI()).getZ())*cnt->totalInertia;
                    if(rb)
                    {
                        cnt->unitlinmov[1] = cnt->linin[1] * cnt->totalInertia;
                        cnt->unitangmov[1] = (((cnt->relContactPoint[1] ^ cnt->normal) * rb->getInvI()).getZ())*cnt->totalInertia;
                    }
                    cts.push_back(cnt);
                    c->cnts[j] = cnt;
                    if(cnt->getPenetration() > c->worstPenetrationAmount)
                    {
                        c->worstPenetrationAmount = cnt->getPenetration();
                        c->worstContact = cnt;
                    }
                    if(cnt->desiredVelocityChange > c->worstVelocityChangeAmount)
                    {
                        c->worstVelocityChangeAmount = cnt->desiredVelocityChange;
                        c->worstVelocityContact = cnt;
                    }
                }
            }
            else
                c->cnts = 0;
        }
    }
    
}
