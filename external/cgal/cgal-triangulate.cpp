/******************************************************************************
* This file is part of CYTools.
*
* CYTools is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CYTools is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CYTools.  If not, see <https://www.gnu.org/licenses/>.
******************************************************************************/

#define CGAL_EIGEN3_ENABLED
#include <CGAL/Epick_d.h>
#include <CGAL/Regular_triangulation.h>
#include <CGAL/IO/Triangulation_off_ostream.h>
#include <CGAL/assertions.h>
#include <cstdlib>
#include <ctype.h>
#include <string.h>
const bool useHeights = true; // Toggle between using heights or weights
typedef CGAL::Epick_d< CGAL::Dynamic_dimension_tag >    K;
typedef CGAL::Regular_triangulation<K>                  T;
typedef K::Point_d                                      Bare_point;
typedef K::Weighted_point_d                             Weighted_point;
typedef CGAL::Regular_triangulation_traits_adapter<K>   RK;
typedef RK::Compute_coordinate_d                        Ccd;

std::vector<double> readWeights(std::istream& ist){
  char c;
  double i;
  std::vector<double> array;
  ist >> std::ws >> c;
  if (c == '(') {
    while (ist >> std::ws >> c) {
      if (isspace(c)) {
        continue;
      }
      if (c == ')') {
        break;
      }
      if (c == ',') {
        continue;
      }
      if(isdigit(c) || c == '-'){
        ist.putback(c);
        ist >> i;
        array.push_back(i);
      }
      else {
        std::cerr << "Error reading " << (useHeights ? "heights" : "weights") << ": Not an integer." << std::endl;
        ist.clear(std::ios::failbit);
        array.clear();
        return array;
      }
    }
  }
  else {
    std::cerr << "Error reading " << (useHeights ? "heights" : "weights") << ": Missing ``(''." << std::endl;
    ist.clear(std::ios::failbit);
    array.clear();
    return array;
  }
  ist.clear(std::ios::goodbit);
  return array;
}

std::vector<int> readPoint(std::istream& ist){
  char c;
  int i;
  int D = -1;
  std::vector<int> point;
  ist >> std::ws >> c;
  if (c == '[') {
    while (ist >> std::ws >> c) {
      if (isspace(c)) {
        continue;
      }
      if (c == ']') {
        break;
      }
      if (c == ',') {
        continue;
      }
      if(isdigit(c) || c == '-'){
        ist.putback(c);
        ist >> i;
        point.push_back(i);
      }
      else {
        std::cerr << "Error reading point: Not an integer." << std::endl;
        ist.clear(std::ios::failbit);
        point.clear();
        return point;
      }
    }
  }
  else {
    std::cerr << "Error reading point: Missing ``[''." << std::endl;
    ist.clear(std::ios::failbit);
    point.clear();
    return point;
  }
  ist.clear(std::ios::goodbit);
  if (D == -1) {
    D = point.size();
  }
  else if (point.size() != D){
    std::cerr << "Error reading point: Inconsistent dimension." << std::endl;
    point.clear();
  }
  return point;
}

std::vector<std::vector<int> > readPoints(std::istream& ist){
  char c;
  std::vector<std::vector<int> > points;
  std::vector<int> tmp_point;

  ist >> std::ws >> c;
  if (c == '['){
    while (ist >> std::ws >> c){
      if (isspace(c)) {
        continue;
      }
      if (c == ']') {
        break;
      }
      if (c == ',') {
        continue;
      }
      ist.putback(c);
      tmp_point = readPoint(ist);
      if (tmp_point.size() > 0){
        points.push_back(tmp_point);
        tmp_point.clear();
      }
      else {
        std::cerr << "Error reading point list" << std::endl;
        points.clear();
        return points;
      }
    }
  }
  else{
    std::cerr << "Error reading point list: Missing ``[''." << std::endl;
    return points;
  }
  ist.clear(std::ios::goodbit);
  return points;
}

int main(int argc, char *argv[])
{
  // Define some objects for later
  RK traits = RK();
  const Ccd ccd = traits.compute_coordinate_d_object();

  // Read point list
  std::vector<std::vector<int> > points = readPoints(std::cin);
  int nPoints = points.size();
  if(nPoints == 0){
    std::cerr << "Error: points list was empty" << std::endl;
    return 1;
  }
  int D = points[0].size();
  // Read weights or heights
  std::vector<double> weights = readWeights(std::cin);
  if(nPoints != weights.size()){
    std::cerr << (useHeights ? "Heights" : "Weights") << " not specified or size mismatch. Computing Delaunay triangulation..." << std::endl;
    weights.clear();
  }
  // If using heights, convert them into weights
  if(useHeights && weights.size() != 0){
    for(int i = 0; i < nPoints; i++){
      double h0 = 0;
      for(int j = 0; j < D; j++){
        h0 += points[i][j]*points[i][j];
      }
      weights[i] = h0 - weights[i];
    }
  }

  // Produce triangulation
  std::vector<Weighted_point> w_points;
  w_points.reserve(nPoints);
  for(int i = 0; i < nPoints; i++) {
    Bare_point p(points[i].begin(), points[i].end());
    Weighted_point wp(p,(weights.size() == 0 ? 0 : weights[i]));
    w_points.push_back(wp);
  }

  // Define triangulation object
  T t(D);
  CGAL_assertion(t.empty());

  // Insert the points in the triangulation
  t.insert(w_points.begin(), w_points.end());

  // Check if the triangulation is ok
  CGAL_assertion( t.is_valid() );

  // Match vertices to indices in the order they were given
  std::map<T::Vertex_handle, int> index_of_vertex;
  for(T::Vertex_iterator it = t.vertices_begin(); it != t.vertices_end(); ++it){
    if(t.is_infinite(it))
      continue;
    std::vector<int> vert(D,0);
    for(int i = 0; i < D; i++){
      vert[i] = CGAL::to_double(ccd(it->point(), i));
    }
    index_of_vertex[it] = std::distance(points.begin(), std::find(points.begin(), points.end(), vert));
  }

  // Print the simplices (and weights)
  printf("[");
  for(T::Finite_full_cell_iterator it = t.finite_full_cells_begin(); it != t.finite_full_cells_end(); ++it){
    for(int i = 0; i < D+1; i++){
      if(i == 0) printf("[%i,",index_of_vertex[it->vertex(i)]);
      else if(i == D) printf("%i]",index_of_vertex[it->vertex(i)]);
      else printf("%i,",index_of_vertex[it->vertex(i)]);
    }
    if(it != t.finite_full_cells_end() && std::next(it) != t.finite_full_cells_end()) printf(",");
  }
  printf("]\n");

  return 0;

}
