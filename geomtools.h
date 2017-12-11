/*
  3dfier: takes 2D GIS datasets and "3dfies" to create 3D city models.
  
  Copyright (C) 2015-2016  3D geoinformation research group, TU Delft

  This file is part of 3dfier.

  3dfier is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  3dfier is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with 3difer.  If not, see <http://www.gnu.org/licenses/>.

  For any information or further details about the use of 3dfier, contact
  Hugo Ledoux 
  <h.ledoux@tudelft.nl>
  Faculty of Architecture & the Built Environment
  Delft University of Technology
  Julianalaan 134, Delft 2628BL, the Netherlands
*/

#ifndef geomtools_h
#define geomtools_h

#include "io.h"
#include "definitions.h"
#include <random>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Projection_traits_xy_3.h>
#include <CGAL/Triangulation_vertex_base_with_id_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polygon_2.h>

#include <vector>
#include <unordered_set>
#include <iterator>
#include <memory>
#include <boost/heap/binomial_heap.hpp>

struct PointXYHash
{
  std::size_t operator()(Point3 const& p) const noexcept
  {
    std::size_t h1 = std::hash<double>{}(bg::get<0>(p));
    std::size_t h2 = std::hash<double>{}(bg::get<1>(p));
    return h1 ^ (h2 << 1);
  }
};
struct PointXYEqual
{
  std::size_t operator()(Point3 const& p1, Point3 const& p2) const noexcept
  {
    auto ex = bg::get<0>(p1) == bg::get<0>(p2);
    auto ey = bg::get<1>(p1) == bg::get<1>(p2);
    return ex && ey;
  }
};
struct point_error {
  point_error(int i, double e) : index(i), error(e){}
  int index;
  double error;
};
struct compare_error {
  inline bool operator()
    (const point_error &e1 , const point_error &e2) const {
      return e1.error < e2.error;
}
};
namespace bh = boost::heap;
typedef bh::binomial_heap<point_error, bh::compare<compare_error>> Heap;
typedef Heap::handle_type heap_handle;

typedef CGAL::Exact_predicates_inexact_constructions_kernel			K;
typedef CGAL::Projection_traits_xy_3<K>								Gt;
typedef CGAL::Triangulation_vertex_base_with_id_2<Gt>				Vb;
struct FaceInfo2
{
  FaceInfo2() {}
  int nesting_level;
  bool in_domain() {
    return nesting_level % 2 == 1;
  }
  std::vector<heap_handle>* points_inside = nullptr;
  CGAL::Plane_3<K>* plane = nullptr;
};
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2, Gt>	Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<Gt, Fbb>		Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb>				Tds;
typedef CGAL::Exact_predicates_tag									Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<Gt, Tds, Itag>	CDT;
typedef CDT::Point													Point;
typedef CGAL::Polygon_2<Gt>											Polygon_2;

typedef std::map<CDT::Vertex_handle, double> Vertex_map; // a vertex and an error
double estimateZ_LIN(CDT &dt, CDT::Vertex_handle);
void updateMap(CDT &dt, Vertex_map &vmap, CDT::Vertex_handle v);  
void simplify(CDT &dt, double threshold);

inline double compute_error(Point &p, CDT::Face_handle &face);
void greedy_insert(CDT &T, const std::vector<Point3> &pts, double threshold);

std::string gen_key_bucket(Point2* p);
std::string gen_key_bucket(Point3* p);
std::string gen_key_bucket(Point3* p, int z);

struct ElevationLine {
  ElevationLine(Point p1, Point p2, double a) : p1(p1), p2(p2), angle(a){}
  Point p1,p2;
  double angle;
};

bool triangle_contains_segment(Triangle t, int a, int b);
std::vector<ElevationLine> getCDT(const Polygon2* pgn,
            const std::vector< std::vector<int> > &z, 
            std::vector< std::pair<Point3, std::string> > &vertices, 
            std::vector<Triangle> &triangles, 
            const std::vector<Point3> &lidarpts = std::vector<Point3>(),
            double tinsimp_threshold=0);

#endif /* geomtools_h */
