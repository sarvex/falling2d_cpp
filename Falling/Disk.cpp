#include "stdafx.h"
#include "Disk.h"

Disk::Disk(const Point2D &pt, float dradius, bool fixed)
{
	parent = this;
	fixedobj = fixed;
	radius = dradius + PROXIMITY_AWARENESS;
	margin = radius;//0.04f;
	radiuslessM = 0;//radius - margin;
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

float Disk::getBoundingSphereRadius() const
{ return radius; }

float Disk::getSurface() const
{ return (float)M_PI * radius * radius; }

float Disk::getInertiaMomentum(float m) const
{ return m * radius * radius / 2; }

Vector2D Disk::getCenter() const
{ return t.getU(); }

int Disk::getSupportPoint(const Vector2D &, Point2D *res) const
{
	Vector2D v = getCenter();
	//v += d.direction() * radiuslessM;
	*res =  Point2D(v.getX(), v.getY()); 
	return 0; 
}
int Disk::getSupportPoint(const Vector2D &, Point2D *res, int) const
{	  
	Vector2D v = getCenter();
	//v += d.direction() * radiuslessM;
	*res =  Point2D(v.getX(), v.getY()); 
	return 0; 
}
