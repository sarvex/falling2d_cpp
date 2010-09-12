#include "stdafx.h"
#include "Disk.h"

Disk::Disk(Point2D &pt, float dradius, bool fixed)
{
	parent = this;
	fixedobj = fixed;
	radius = dradius;
	margin = 0.04f;
	radiuslessM = radius - margin;
	t = GeometryHelper::Transformation2D(Vector2D (pt), 0);
	obb = new OBB(Point2D(-radius, radius),Point2D(radius, radius),Point2D(radius, -radius),Point2D(-radius, -radius), this, radius * radius, 0);
	otree = new OBBtree(0, 0, obb);
}

void Disk::updateAABB()
{
	Vector2D p = getCenter();
	aabb_xm = -radius + p.getX();
	aabb_xM = radius + p.getX();
	aabb_ym = -radius + p.getY();
	aabb_yM = radius + p.getY();
}

float Disk::getBoundingSphereSqRadius() 
{ return radius * radius; }

float Disk::getSurface()
{ return (float)M_PI * radius * radius; }

float Disk::getInertiaMomentum(float m)
{ return m * radius * radius / 2; }

Vector2D Disk::getCenter()
{ return t.getU(); }

int Disk::getSupportPoint(Vector2D &d, Point2D *res)
{
	Vector2D v = getCenter();
	v += d.direction() * radiuslessM;
	*res =  Point2D(v.getX(), v.getY()); 
	return 0; 
}
int Disk::getSupportPoint(Vector2D &d, Point2D *res, int optimisationId)
{	  
	Vector2D v = getCenter();
	v += d.direction() * radiuslessM;
	*res =  Point2D(v.getX(), v.getY()); 
	return 0; 
}