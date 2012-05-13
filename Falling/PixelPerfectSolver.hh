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
#include "CollisionDetector.hh"
#include "Shapes.hh"
#include "PixelPerfectShape.hh"

namespace Falling
{
    class PixelPerfectSolver : CollisionDetector
    {
	private:
	    PixelPerfectShape *pps;
	    Shape * s;

	    bool _solve(std::vector<ContactBackup *> &res);
	public:
	    PixelPerfectSolver(PixelPerfectShape *ps, Shape *s);
	    bool canDestroy();
    };
}
