#include "stdafx.h"
#include "Polygon2D.h"
#include "GeometryHelper.h"
#include <vector>

#pragma region Polygon2D
Polygon2D::Polygon2D(Point2D p[], int nbpts, Point2D *hpts[], int nbholes, int hnbrpts[], int mergetype, Vector2D position, bool useCentroid, float orientation, float mass, bool isimmediatemass, bool fixed)
	: ixm(0), ixM(0), iym(0), iyM(0)
{
	Tesselator tess;
	Point2D **subpolys;
	int *nbptssubpolys;
	fixedobj = fixed;
	int *holesnbrpts = new int[nbholes];
	Point2D **holespts = new Point2D*[nbholes];
	// simplify
	// nbrPts = nbpts;
	nbrPts = simplifyToProper(p, nbpts, &points, 0.05f);
	for(int i = 0; i < nbholes; i++)
		holesnbrpts[i] = simplify(hpts[i], hnbrpts[i], &(holespts[i]), 0.05f);
	//scalepts(points, nbrPts, 1000.f);
	t = GeometryHelper::Transformation2D(Vector2D(),0);
	// calculate convex decomposition
	nbrSubShapes = tess.initAndRun(mergetype, points, nbrPts, holespts, nbholes , holesnbrpts, &subpolys, &nbptssubpolys);
	//for(int i = 0; i < nbrSubShapes; i++)
	//	scalepts(subpolys[i], nbptssubpolys[i], 0.001f);
	// calculate centroid and surface of all subpolygons
	float totalSurface = 0;
	Point2D totalCentroid(0,0);
	// build implicit polygons
	subShapes = new ImplicitPolygon2D *[nbrSubShapes];
	for(int i = 0; i < nbrSubShapes; i++)
	{		  
		subShapes[i] = new ImplicitPolygon2D(subpolys[i], nbptssubpolys[i], this, i);
		totalSurface += subShapes[i]->getSurface();
		Point2D centr= subShapes[i]->getCentroid();
		// TODO: remove test
		//if(!(centr.getX() == centr.getX() && centr.getY() == centr.getY()))
		//	while (true);
		// end todo
		totalCentroid += subShapes[i]->getCentroid() * subShapes[i]->getSurface();
	}
	totalCentroid = Point2D(totalCentroid.getX() / totalSurface, totalCentroid.getY() / totalSurface);
	for(int i = 0; i < nbrSubShapes; i++)
		subShapes[i]->translateCentroid(Vector2D(-totalCentroid.getX(), -totalCentroid.getY(), 0));
	// translate points
	for(int i = 0; i < nbrSubShapes; i++)
	{
		for(int j = 0; j < nbptssubpolys[i]; j++)
			subpolys[i][j] = subpolys[i][j] - totalCentroid;
	}
	// build obb tree
	buildOBBtree();
	// adjust position
	t.setTeta(orientation);
	t.setU((useCentroid)?Vector2D(totalCentroid):position);

	delete holesnbrpts;
}

void Polygon2D::scalepts(Point2D *pts, int n, float scalefactor)
{
	for(int i = 0; i < n; i++)
	{
		pts[i] = pts[i] * scalefactor;
	}
}

void Polygon2D::updateAABB()
{
	Vector2D vx(1,0);
	Vector2D vy(0,1);
	Point2D pt;
	float m = chull->getMargin();
	ixM = chull->getSupportPoint(vx,&pt,ixM);
	aabb_xM = pt.getX() + m;
	ixm = chull->getSupportPoint(vx.reflexion(),&pt,ixm);
	aabb_xm = pt.getX() - m;
	iyM = chull->getSupportPoint(vy,&pt,iyM);
	aabb_yM = pt.getY() + m;
	iym = chull->getSupportPoint(vy.reflexion(),&pt,iym);
	aabb_ym = pt.getY() - m;
}

void Polygon2D::buildOBBtree()
{	
	// divide space		  
	int id = 0;
	std::vector<ImplicitPolygon2D *> set;
	for(int i = 0; i < nbrSubShapes; i++)
		set.push_back(subShapes[i]);
	buildOBBtree(&otree, set, id);
}

void Polygon2D::buildOBBtree(OBBtree **o, std::vector<ImplicitPolygon2D*> &polyset, int &id)
{		
	id++;
	int s = (int)polyset.size();
	if(s == 1)
	{
		// build obb	
		// if id == 1, the object's convex hull has been calculated, so store it
		if(id == 1)
			chull = new ImplicitPolygon2D(polyset[0]->getPts(), polyset[0]->getNbrPts(),this,1);
		//
		*o = new OBBtree(0,0, polyset[0]->getOBB());
		polyset.clear();
		return;
	}
	Vector2D ya(0,-1);
	std::vector<Point2D> chpts;
	// calc subshapes' convex hull
	Point2D miny, studiedpt;
	polyset[0]->getSupportPoint(ya, &miny);
	miny = polyset[0]->toLocal(miny);
	for(int i = 1; i < s; i++)
	{
		Point2D tmp;	
		polyset[i]->getSupportPoint(ya, &tmp);	   
		tmp = polyset[i]->toLocal(tmp);
		if(tmp.getY() < miny.getY())
			miny = tmp;
		else if(tmp.getY() == miny.getY() && tmp.getX() < miny.getX())
			miny = tmp;
	}				
	studiedpt = miny;
	do
	{
		if(chpts.size() >= 2)
		{
			if(abs(studiedpt.isLeftTo(chpts[chpts.size()-2], chpts[chpts.size()-1])) > 0.000001f &&
				abs(studiedpt.isLeftTo(chpts[0], chpts[1])) > 0.000001f &&
				abs(studiedpt.isLeftTo(chpts[0], chpts[chpts.size()-1])) > 0.000001f) 
				chpts.push_back(studiedpt);
			else printf("ah voila");
		}
		else
			chpts.push_back(studiedpt);
		Point2D rp = polyset[0]->rightTgtPt(studiedpt);
		for(int i = 1; i < s; i++)
		{
			// TODO: remove test
			int iii;
			for(iii = 0; iii < polyset[i]->nbrPts; iii++)
			{
				if(polyset[i]->pts[iii].exactEquals(studiedpt))
					break;
			}
			if(iii == polyset[i]->nbrPts && Point2D::pointInPolygon(studiedpt, polyset[i]->pts, polyset[i]->nbrPts))
				while(true);
			// end TODO
			Point2D tmp = polyset[i]->rightTgtPt(studiedpt); 
			// TODO: remove test
			if(tmp.exactEquals(miny))
				printf("aienratenrtaueausieasrieanietaunietars\n");
			// end todo
			if(tmp.exactEquals(studiedpt))
				continue;
			float lt = tmp.isLeftTo(studiedpt, rp); 
			if(lt < 0)
				rp = tmp;	
			else if(lt == 0)
			{
				// handle three-colinear-points case
				Vector2D rs(studiedpt, rp);
				Vector2D ts(studiedpt, tmp);
				if(ts * ts < rs * rs) // nearest point is selected
					rp = tmp;
			} 
		}	
		studiedpt = rp;
	} while(studiedpt.getX() != miny.getX() || studiedpt.getY() != miny.getY());
	int chn = (int)chpts.size();
	Point2D *ch = new Point2D[chn];
	for(int i = 0; i < chn; i++)
		ch[i] = chpts[i];
	// build obb	
	// if id == 1, the object's convex hull has been calculated, so store it
	if(id == 1)
	{
		// TODO: remove test
		Point2D *out;
		simplifyToProper(ch,chn, &out,0);
		// end todo
		chull = new ImplicitPolygon2D(ch,chn,this,1);
	}
	//
	*o = new OBBtree(0, 0, ImplicitPolygon2D::buildOBB(ch, chn, chull, -1));
	chpts.clear();
	OBBtree *oo = *o;

	// divide space		  
	std::vector<ImplicitPolygon2D *> rightset;
	std::vector<ImplicitPolygon2D *> leftset;
	Point2D ao;
	Point2D as;
	oo->o->getMedialAxis(&ao, &as);
	for(int i = 0; i < s; i++)
	{
		Vector2D vp = polyset[i]->getCenter();
		Point2D p(vp.getX(), vp.getY());
		if(p.isLeftTo(ao, as) < 0)
			leftset.push_back(polyset[i]);
		else
			rightset.push_back(polyset[i]);
	}
	if(rightset.size()==0)
	{
		int ls = ((int)leftset.size()) / 2;
		while(leftset.size() != ls)
		{
			rightset.push_back(leftset[leftset.size()-1]);
			leftset.pop_back();
		}
	}
	else if(leftset.size() == 0)
	{	 
		int ls = ((int)rightset.size()) / 2;
		while(rightset.size() != ls)
		{
			leftset.push_back(rightset[rightset.size()-1]);
			rightset.pop_back();
		}
	}
	// rego	
	polyset.clear();
	if(rightset.size()!=0)
		buildOBBtree(&(oo->r), rightset, id);
	if(leftset.size()!=0)
		buildOBBtree(&(oo->l), leftset, id);
}
int Polygon2D::simplifyToProper(Point2D *in, int n, Point2D **out,  float tolerence)
{
	Point2D *pt = new Point2D[n];
	int s = 0;
	int p=n-2, np=n-1, nnp=0;
	while(nnp<n)
	{
		float xa =in[p].getX(),
		      xc = in[np].getX(),
		      xb = in[nnp].getX(),
		      ya = in[p].getY(),
		      yc = in[np].getY(),
		      yb = in[nnp].getY();
		if(!in[nnp].isInLine(in[np], in[p]))
		{
			pt[s] = in[np];
			s++;
			p = np;
			np = nnp;
		}
		else
		{
			printf("Was not proper\n");
			np = nnp;	 
		}
		nnp++;
	}
	*out = new Point2D[s];
	for(int i=0;i<s;i++)
		(*out)[i] = pt[i];
	delete[] pt;
    return s;
}

int Polygon2D::simplify(Point2D *in, int n, Point2D **out,  float tolerence)
{
	Point2D *pt = new Point2D[n];
	int s = 0;
	int p=n-2, np=n-1, nnp=0;
	while(nnp<n)
	{
		float xa =in[p].getX(),
		      xc = in[np].getX(),
		      xb = in[nnp].getX(),
		      ya = in[p].getY(),
		      yc = in[np].getY(),
		      yb = in[nnp].getY();
		if(abs(xa*yc-xa*yb+xb*ya-xb*yc+xc*yb-xc*ya) > 2.f * tolerence)
		{
			pt[s] = in[np];
			s++;
			p = np;
			np = nnp;
		}
		else
			np = nnp;	 
		nnp++;
	}
	*out = new Point2D[s];
	for(int i=0;i<s;i++)
		(*out)[i] = pt[i];
	delete[] pt;
    return s;
}

float Polygon2D::getUnitInertiaMomentum(Point2D *points, int nbpts, Vector2D &axisTranslate)
{									  
	float num = 0, denum = 0;
	int l = nbpts, j = l - 1;
	for (int i = 0; i < l; i++)
	{
		Vector2D pn = Vector2D(points[j]) + axisTranslate;
		Vector2D pn1 = Vector2D(points[i]) + axisTranslate;
		float f = pn1.cross(pn).magnitude();
		denum += f;
		num += f * (pn1.dot(pn1) + pn1.dot(pn) + pn.dot(pn));
		j = i;
	}
	return (denum == 0) ? 0 : abs(num / (6 * denum));
}

float Polygon2D::getInertiaMomentum(float density)
{ 
	float totalInertia = 0;
	for(int i = 0; i < nbrSubShapes; i++)
		totalInertia += subShapes[i]->getUnitInertiaMomentum() * density;
	return totalInertia;
}

float Polygon2D::getSurface()
{
	float totalSurface = 0;
	for(int i = 0; i < nbrSubShapes; i++)
		totalSurface += subShapes[i]->getSurface();
	return totalSurface;
}

#pragma region Static methods
	struct PointWithNext
	{
		Point2D pt;
		struct PointWithNext *next, *homolog;
		float bparam;
		inline PointWithNext(Point2D pt)
		{ this->pt = pt; next = this; homolog = 0; bparam = -1; }
	};
int Polygon2D::getUncrossedPolygon(Point2D *pts, int nb, Point2D **res)
{
	// create initial chain
		// find an external point
	int mostext = 0;
	for(int i = 1; i < nb; i++)
	{
		if(pts[i].getY() < pts[mostext].getY())
			mostext = i;
		i++;
	}
		// create chain with mostext at initial point
	struct PointWithNext *head = new struct PointWithNext(pts[mostext]);
	struct PointWithNext *prev = head;
	int j = (mostext == nb - 1) ? 0	: mostext + 1;
	while(j != mostext)
	{
		prev->next = new struct PointWithNext(pts[j]);
		prev = prev->next;
		if(j == nb - 1)
			j = 0;														
		else
			j++;
	}
	prev->next = head; // make it circular
	// find all intersection and insert them in the chain
	struct PointWithNext *currref   = head, *ncurrref;
	struct PointWithNext *currscan, *ncurrscan;	
	int nbinter = 0;
	do
	{
		// find next currref  
		ncurrref = currref->next;
		while(ncurrref->homolog != 0)
			ncurrref = ncurrref->next;
		if(ncurrref == head)
			break; // don't test last edge as base
		// find currscan	  
		currscan = ncurrref->next;	 // skip next epge
		while(currscan->homolog != 0)
			currscan = currscan->next;
		// find intersections
		while(currscan != head)
		{
			// find next currscan
			ncurrscan = currscan->next;
			while(ncurrscan->homolog != 0)
				ncurrscan = ncurrscan->next;
			if(ncurrscan == currref)
				break;
			// find intersection
			Point2D inter;
			float bparamscan;
			float bparamref = Point2D::intersectSegments(currref->pt,ncurrref->pt,currscan->pt, ncurrscan->pt,&inter,&bparamscan);
			if(bparamref != -1)
			{			
				nbinter += 2;
				// if intersects, insert in chain sorting by the barycentric coord
				struct PointWithNext *inter1 = new struct PointWithNext(inter);	  
				struct PointWithNext *inter2 = new struct PointWithNext(inter); 
				inter1->bparam = bparamref;
				inter2->bparam = bparamscan;
				inter1->homolog = inter2;
				inter2->homolog = inter1;
				// insert 1 in chain
				struct PointWithNext *previnsert = currref;
				while(previnsert->next->homolog != 0 && previnsert->next->bparam < bparamref)
					previnsert = previnsert->next;
				inter1->next = previnsert->next;
				previnsert->next = inter1;
				// insert 2 in chain
				previnsert = currscan;
				while(previnsert->next->homolog != 0 && previnsert->next->bparam < bparamscan)
					previnsert = previnsert->next;
				inter2->next = previnsert->next;
				previnsert->next = inter2;
				// end of insersion

			}
			// else no intersection
			currscan = ncurrscan;
		}
		currref = ncurrref;
	}
	while(currref != head);
	// now parc links and revert links
	currref = head->next; // skip head: cannot be an intersection point
	while(currref != head)
	{
		if(currref->homolog != 0)
		{   
			struct PointWithNext *prevparc = currref->next;
			struct PointWithNext *parc = prevparc->next;
			// invert links to homolog	
			while(parc != currref->homolog)
			{					
				struct PointWithNext *n = parc->next;
				parc->next = prevparc;
				prevparc = parc;
				parc = n;
			}
			currref->next->next = currref->homolog;
			currref->next = prevparc;
			currref->homolog->homolog = 0; // never reinterpret it again
			currref->homolog = 0; // never reinterpret it again
		}
		currref = currref->next;
	}

	// the polygon is now uncrossed, copy the result  
	// and free memory
	currref = head;
	int l = nbinter + nb; 
    *res = new Point2D[l];
	int i = 0;
	while(i<l)
	{
		struct PointWithNext *toDestroy;
		toDestroy = currref;
		(*res)[i] = currref->pt;
		currref = currref->next;
		// free
		delete toDestroy;
		i++;
	}
	return l;
}
/////////////////////////////////////////////////
//				MELKMAN ALGORITHM			   //
// Builds convex hull of current polygon's	   //
// boundary's polyline representation.		   //
// Only handles simple polygon. (i.e. w/o	   //
// self intersection.)						   //
// Outputs an indexes-array instead of		   //
// Point2D array.							   //
// Returns the number of indexes in the array. //
/////////////////////////////////////////////////
int Polygon2D::buildConvexHull(Point2D *pts, int nbPts,Point2D ** outHull)
{
	int res;
	int *D=new int[2*nbPts+1];
	int bot = nbPts - 2, top = bot + 3;
	D[bot] = D[top] = 2;
	if(pts[2].isLeftTo(pts[0],pts[1]) > 0)
	{
		D[bot+1] = 0;
		D[bot+2] = 1;
	}
	else
	{
		D[bot+2] = 0;
		D[bot+1] = 1;
	}
	int i=3;
	while(i < nbPts)
	{
		Point2D p = pts[i];
		if(p.isLeftTo(pts[D[bot]],pts[D[bot + 1]]) <= 0 
		   || p.isLeftTo(pts[D[top - 1]],pts[D[top]]) <= 0)
		{
			while(p.isLeftTo(pts[D[bot]],pts[D[bot + 1]]) <= 0)
				bot++;
			bot--;
			D[bot] = i;

			while(p.isLeftTo(pts[D[top - 1]],pts[D[top]]) <= 0)
				top--;
			top++;
			D[top] = i;
		}
		i++;
	}
	res = top - bot;
	*outHull = new Point2D[res];
	for(i=bot;i<=top;i++)
		(*outHull)[i - bot] =  pts[D[i]]; 
	delete[] D;
	return res;
}
float Polygon2D::getSurface(Point2D *in, int n)
{	  
	int l = n;
	float a = 0;
	int j = l - 1;
	for (int i = 0; i < l; i++)
	{
		a += in[j].getX() * in[i].getY() - in[j].getY() * in[i].getX();
		j = i;
	}
	return abs(a / 2);
}

Point2D Polygon2D::getCentroid(Point2D *in, int n)
{  return Polygon2D::getCentroid(in, n, Polygon2D::getSurface(in, n)); }

Point2D Polygon2D::getCentroid(Point2D *in, int n,float aire)
{
	int l = n;
	float ax = 0, ay = 0;
	int j = l - 1;
	for (int i = 0; i < l; i++)
	{
		float ix = in[i].getX(), iy = in[i].getY(), jx = in[j].getX(), jy = in[j].getY();
		float det = (jx * iy - jy * ix);
		ax += (jx + ix) * det;
		ay += (jy + iy) * det;
		j = i;
	}
	return Point2D(ax / (6 * aire), ay / (6 * aire));
}
#pragma endregion
#pragma endregion

#pragma region ImplicitPolygon2D
ImplicitPolygon2D::ImplicitPolygon2D(Point2D *globalPts, int n, Polygon2D *p, int id) 
{	
	/*
	for(int i = 0; i < n; i++)
	{
		Point2D tmp = globalPts[i];
		globalPts[i] = globalPts[n - 1 - i];
		globalPts[n - 1 - i] = tmp;
	}
	*/
	// TODO: remove test
	int j = n-1;
	for(int i = 0; i < n; i++)
	{
		for(int k = i+1; k < n-2; k++)
		{
			Point2D useless;
			float param;
			if(Point2D::intersectSegments(globalPts[j], globalPts[i], globalPts[k], globalPts[k+1], &useless, &param) != -1)
			{
				while(true);
			}
		}
	}
	//
	nbrPts = n;
	parent = p; 
	margin = PROXIMITY_AWARENESS + 0.01f;
	pts = globalPts;		
	center = Polygon2D::getCentroid(pts, n);
	obb = ImplicitPolygon2D::buildOBB(pts, n, this, id);
	radius = sqrt(_getBoundingSphereSqRadius());
	surface = Polygon2D::getSurface(pts, n);
	unitInertia = Polygon2D::getUnitInertiaMomentum(pts, n, Vector2D(-center.getX(), -center.getY(),0)) * surface;
}

void ImplicitPolygon2D::translateCentroid(Vector2D &u)
{
	center += u;
	obb->translate(u);
}

float ImplicitPolygon2D::_getBoundingSphereSqRadius()
{
	float res = 0;
	for(int i = 0; i<nbrPts; i++)
	{
		Vector2D v(center, pts[i]);
		float vv = v * v;
		if(vv > res)
			res = vv;
	}
	return res;
}

float ImplicitPolygon2D::getBoundingSphereRadius()
{ return radius; }

Point2D ImplicitPolygon2D::rightTgtPt(Point2D &ref)
{	   	
	int a = 0, b = nbrPts - 1, ires;
	ires = 0;
	// TODO: remove: brute force test
	if(pts[a].exactEquals(ref))
		ires = 1;
	else
	{
		for(a = ires; a < nbrPts; a++)
		{
			if(!pts[ires].exactEquals(ref))
			{
				if(pts[a].isLeftTo(ref, pts[ires]) < 0)
					ires = a;
			}
			else 
				ires = a + 1;
		}
	}

	/*
	int aref, bref;
	if(pts[a].exactEquals(ref))
		ires = 1;
    else if(pts[b].equals(ref))
		ires = a;
	else if(pts[a].isLeftTo(ref, pts[a+1]) < 0	&& pts[a].isLeftTo(ref, pts[b]) <= 0)
		ires = a;  	
	else if(pts[b].isLeftTo(ref, pts[a]) < 0	&& pts[b].isLeftTo(ref, pts[b - 1]) <= 0)
		ires = b;
	else
	{
		while(true)
		{
			bool arrived = true;
			int c = (a + b)/2;
			if(c == ires2)
			{
				aref = a;
				bref = b;
			}
						
			bool dnc = pts[c].isLeftTo(ref, pts[c+1]) < 0;   
			if(pts[c].equals(ref))
			{
				ires = c + 1;	
				break;
			}
			if(dnc	&& pts[c].isLeftTo(ref, pts[c-1]) <= 0)
			{
				ires = c;
				break;
			}
			bool upa = pts[a].isLeftTo(ref, pts[a+1])  > 0;
			if(upa)
			{
				if(dnc)
					b = c;
				else
				{
					if(pts[c].isLeftTo(ref, pts[a])  > 0)
						b = c;
					else
						a = c;
				}
			}
			else
			{
				if(!dnc)
					a = c;
				else
				{   
					if(pts[c].isLeftTo(ref, pts[a]) < 0)
						b = c;
					else
						a = c;
				}
			}
		}
	}
	//*/
	return pts[ires];
} 

int ImplicitPolygon2D::naiveClimb(int ibase, int imax, Vector2D &v)
{
	float lastDot = -FLT_MAX, dx = v.getX(), dy = v.getY();
	int i, ires = ibase;
	for(i = ibase; i <= imax; i++)
	{
		float dot = pts[i].getX() * dx + pts[i].getY() * dy;
		//if(dot < lastDot)
		//	break;
		if(dot > lastDot)
		{
			ires = i;
			lastDot = dot;
		}
	}
	return ires;
}

////////////////////////////////////////////
//			 BINARY SEARCH                //
//									      //
// Retrieves s(d) suporting point of      //
// current convex polygon using binary    //
// search. Algorithm is O(log(n))         //
////////////////////////////////////////////
int ImplicitPolygon2D::getSupportPoint(Vector2D &od, Point2D *res)
{		   
	Vector2D d = toRotatedInv(od);
	int ires;
	if(nbrPts < 8) // n - 1 < 3 * log(n) pour n < 8
		ires = naiveClimb(0, nbrPts - 1, d);
	else
	{
		int a = 0, b = nbrPts - 1;
		float dx = d.getX(), dy = d.getY();
		Vector2D va = Vector2D(pts[0], pts[1]),
				 vb = Vector2D(pts[b], pts[0]),
				 vb_1 = Vector2D(pts[b-1],pts[b]); 
		bool upa = va.dot(d) > 0;
		bool upb = vb.dot(d) > 0;
		if(upb)
		{
			if(!upa)
			{
				*res = toGlobal(pts[0]);
				return 0;
			}
		}
		else if(vb_1.dot(d) > 0)
		{
			*res = toGlobal(pts[b]);
			return b;
		}
		while(true)
		{
			int c = (a + b)/2;
			Vector2D vc(pts[c], pts[c+1]),
					 vc_1(pts[c-1],pts[c]);
						
			bool upc = vc.dot(d) > 0;
			if(!upc	&& vc_1.dot(d) > 0)
			{
				ires = c;
				break;
			}

			if(!upa)
			{
				if(upc)
				{
					a = c;
					upa = upc;
				}
				else
				{
					Vector2D vac(pts[a],pts[c]);
					if(vac.dot(d) < 0)
					{
						a = c;
						upa = upc;
					}
					else
						b = c;
				}
			}
			else
			{
				if(!upc)
					b = c;
				else
				{   
					Vector2D vac(pts[a],pts[c]);
					if(vac.dot(d) < 0)
						b = c;
					else
					{
						a = c;
						upa = upc;
					}
				}
			}
		}
	}	   
	*res = toGlobal(pts[ires]);
	return ires;
}

////////////////////////////////////////////
//			 HILL CLIMBING                //
//									      //
// Retrieves s(d) suporting point of      //
// current convex polygon using Hill      //
// Climbing. Algorithm is O(n).           //
// Setting o with last int returned by	  //
// any getSupportPoint overload may allow //
// this algorithm to run in near-constant //
// time (geometric coherence).            //
////////////////////////////////////////////
int ImplicitPolygon2D::getSupportPoint(Vector2D &od, Point2D *res, int o)
{
	Vector2D d = toRotatedInv(od);
	int n = tmod(o + 1, nbrPts);
	int p = tmodinv(o - 1, nbrPts);
	float dx = d.getX(),
		  dy = d.getY();									   
	float doto = pts[o].getX() * dx + pts[o].getY() * dy;
	float dotp = pts[p].getX() * dx + pts[p].getY() * dy;
	float dotn = pts[n].getX() * dx + pts[n].getY() * dy;
	if(dotn > doto  || dotp > doto)
	{
		if(dotn < dotp)
		{
			do
			{   
				dotn = dotp;
				o = p;
				ilow(p);
				dotp = pts[p].getX() * dx + pts[p].getY() * dy;
			} while(dotp >= dotn); 
		}
		else
		{
			do
			{   
				dotp = dotn;
				o = n;
				ihig(n);
				dotn = pts[n].getX() * dx + pts[n].getY() * dy;
			} while(dotn >= dotp);
		}
	}
	*res = toGlobal(pts[o]);
	return o;
}

//////////////////////////////////////////
//			ROTATIVE CALIPERS		    //
//								        //
// Calculate current convex polygon's   //
// OBB using O(n) rotative calipers     //
// algorithm.                           //
//////////////////////////////////////////
OBB *ImplicitPolygon2D::buildOBB(Point2D *pts, int nbrPts, ImplicitPolygon2D *parent, int id)
{		
	int t_p[4];   
	float t_teta[4];
	int obbl, obbd;
	float obblon, obblar;
	float obbTeta = 0;
	float teta = 0, minAire = FLT_MAX;
	float siteta = 0, coteta = 1;
	Vector2D calipers[4];
	calipers[0] = Vector2D(-1,0);
	calipers[1] = Vector2D(1,0);
	calipers[2] = Vector2D(0,1);
	calipers[3] = Vector2D(0,-1);
	for(int i = 0;i < 4;i++)
		t_p[i] = 0;
	// calcul de l'AABB
	for(int i = 1;i<nbrPts;i++)
	{
		Point2D p = pts[i];
		if(p.getX() > pts[t_p[2]].getX())
			t_p[2] = i;
		else if (p.getX() < pts[t_p[3]].getX())
			t_p[3] = i;
		
		if(p.getY() > pts[t_p[0]].getY())
			t_p[0] = i;
		else if (p.getY() < pts[t_p[1]].getY())
			t_p[1] = i;
	}
	for(int i=0; i<4; i++)
	{
		Vector2D vc = Vector2D(pts[t_p[i]],pts[tmod(t_p[i]+1,nbrPts)]);
		t_teta[i] = acos(vc.dot(calipers[i])/vc.magnitude());
	}
	do
	{
		Vector2D ud = Vector2D(pts[t_p[0]],pts[t_p[1]]);
		Vector2D rl = Vector2D(pts[t_p[2]],pts[t_p[3]]);
		float lon = abs(ud.dot(calipers[3]));
		float lar = abs(rl.dot(calipers[0]));
		float naire = lon * lar;
		if(naire < minAire)
		{
			minAire = naire;	  
			obbl = t_p[3];	  
			obbd = t_p[1];
			obblon = lon;
			obblar = lar;
			obbTeta = teta;
		}  

		float minTeta = 90;
		for(int i=0; i<4; i++)
		{
			while (t_teta[i] < 0.0000000001f)
			{
				t_p[i] = tmod(t_p[i] + 1,nbrPts);
				int id = t_p[i];
				Vector2D vc = Vector2D(pts[id],pts[tmod(id+1,nbrPts)]);
				t_teta[i] = acos(vc.dot(calipers[i])/vc.magnitude());
				// TODO: remove test
				if(t_teta[i] < 0.0000000001f)
					printf("mhhhh");
				// end todo
			}
			if(t_teta[i] < minTeta)
				minTeta = t_teta[i];							
		}
		teta += minTeta;		 
		coteta = cos(teta);
		siteta = -sin(teta); 
		calipers[0] = Vector2D(-coteta, siteta);
		calipers[1] = Vector2D(coteta, -siteta);
		calipers[2] = Vector2D(siteta, coteta);
		calipers[3] = Vector2D(-siteta, -coteta);
		for(int i = 0; i < 4; i++)
			t_teta[i]-= minTeta;
	} while(teta < M_PI / 2);
	// build OBB	   
	// on se base sur D et L
	Point2D origin;	   
	Vector2D repY = Vector2D(-sin(-obbTeta),-cos(-obbTeta));
	Vector2D repX = Vector2D(-repY.getY(),repY.getX());
	if(obbl == obbd)
		origin = pts[obbd];
	else
	{
		Vector2D dl = Vector2D(pts[obbl],pts[obbd]);
		float dotp	= abs(repY * dl);
 		origin = pts[obbl] + (repY * dotp);
	}
	float m = parent->getMargin();
	obblon +=  m;
	obblar += m;
	Point2D opp = origin - repY * obblon + repX * obblar;
	obblon +=  m;
	obblar += m;
	origin =  opp + repY * obblon - repX * obblar;
	return new OBB(origin, origin - repY * obblon,opp , origin + repX * obblar, parent, minAire, id);
}	
#pragma endregion