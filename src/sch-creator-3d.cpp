#include <sch-creator/sch-creator-3d.h>

namespace sch
{
SchCreator3D::SchCreator3D(double r, double R)
{
  _r = r;
  _R = R;
  _desiredAlpha = R - r;
  _epsilon = 1e-8;
} // SchCreator3D

/*
 *   Given a sphere, checks if all points are inside the sphere.
 *   Use this only to check if the algorithm is properly working.
 */
bool SchCreator3D::checkPointsInSphere(const Sphere & s)
{
  double distance, maxDistance = pow(_R, 2) * (1 + _epsilon);
  for(auto i = _vertexes.begin(); i != _vertexes.end(); i++)
  {
    // std::cout << " Distance: " << distance << std::endl;

    distance = (s.center - (*i)).squaredNorm();
    if(distance - maxDistance > _epsilon)
    {
      std::cout << std::setprecision(15);
      std::cout << " Done. Distance: " << distance;                                                           
      std::cout << " R: " << maxDistance << std::endl;
      std::cout << std::setprecision(5);                                            
      return false;
    }
  }
  return true;
} // checkPointsInSphere

bool SchCreator3D::checkPointsInSphere(size_t a, size_t b, size_t c, size_t d)
{
  Sphere s = findCircumSphere3(b,c,d);
  double distance = (s.center - _vertexes[a]).squaredNorm(); 
  return distance < (s.radius*s.radius);
}


/*
*   Given the indexes of the vertexes of the testing sphere and the dissapearing testing point,
*   gets the derivative with respect to the radius.
*/
bool SchCreator3D::getDerivative(size_t v1, size_t v2, size_t v3, size_t v4)
{
  // get the plane through the points
  SchCreator3D::Plane p = findPlaneBase(v1, v2, v3);

  // get the circle on the plane that goes thorugh the points
  Eigen::Vector2d circle2D = findCircleThroughPoints(v1, v2, v3,p);

  // get the thistance from the center of the circel to the sphere center
  double dToSphereCenter = sqrt(_alpha*_alpha - circle2D.squaredNorm());

  // convert the circle's center coordinates to 3D
  Eigen::Vector3d circle3D;
  circle3D(0) = _vertexes[v1](0) + circle2D.dot(p.base.col(0));
  circle3D(1) = _vertexes[v1](1) + circle2D.dot(p.base.col(1));
  circle3D(2) = _vertexes[v1](2) + circle2D.dot(p.base.col(2));

  // get the derivative of the sphere center coordinates
  Eigen::Vector3d dsc = (_alpha/dToSphereCenter)*p.normal;

  std::cout << ' ' << 2*((circle3D+dToSphereCenter*p.normal).dot(dsc)-_vertexes[v4].dot(dsc)-_alpha) << ' ';

  // Sphere s = findCircumSphere4(v1,v2,v3,v4);
  // Sphere s1 = findSphereThroughPoints(v1,p,circle2D,_alpha);
  // Sphere s2 = findSphereThroughPoints(v1,p,circle2D,_alpha-1e-8);
  // std::cout << "\nCC: " << s.center.transpose() << " r: " << s.radius
  //           << "\ns1: " << s1.center.transpose() << " r: " << s1.radius
  //           << "\ns2: " << s2.center.transpose() << " r: " << s2.radius << std::endl;
  // double d1 = (s1.center-_vertexes[v4]).squaredNorm(), d2 = (s2.center-_vertexes[v4]).squaredNorm();
  // std::cout << d1 - (_alpha*_alpha) << ' ' << d2 - (_alpha+1e-8)*(_alpha+1e-8) << std::endl;

  return 2*((circle3D+dToSphereCenter*p.normal).dot(dsc)-_vertexes[v4].dot(dsc)-_alpha) > 0;
} // getDerivative

/*
 *   Checks edges to find the largest distance between vertexes.
 *   Returns true if it's larger than two times alpha.
 */
bool SchCreator3D::findMaxDistance()
{
  double d;

  for(size_t i = 0; i < poly.edges_.size(); i++)
  {
    // get the length of current vertex
    d = poly.edges_[i].edge.norm();
    // update lardest distance
    if(maxBodyDistance < d) 
      maxBodyDistance = d;
    if(noise > d) 
      noise = d;
  }

  noise /= 1000;

  std::cout << "Maximum body distance: " << maxBodyDistance << std::endl;
  std::cout << "Minimum body distance: " << noise << std::endl;

  if(maxBodyDistance >= 2 * _desiredAlpha)
    return true;
  else
    return false;
} // SchCreator3D::findMaxDistance

double SchCreator3D::addNoise()
{
  int n = rand() % 10;
  if(n % 2 == 0)
    return -noise;
  else
    return noise;
} // addNoise

void SchCreator3D::initialize()
{
  Vector3 vertexTemp;
  Eigen::Vector3d vertex;
  size_t id, index = 0, torusIndex = 0, e1, e2, e3;
  std::vector<size_t> tempX;
  std::multimap<size_t, size_t> orderedEdges;
  std::map<size_t, size_t> edgesByKey;
  std::multimap<size_t, size_t> orderedFaces;


  // find the center of the point cloud
  initialCenter = Eigen::Vector3d(0,0,0);

  // create the vector of vertexes
  for(size_t i = 0; i < _numberOfVertexes; i++)
  {
    // read the vertex
    vertexTemp = poly.vertexes_[i]->getCoordinates();
    // convert to Eigen Vector
    vertex = Eigen::Vector3d(vertexTemp.m_x, vertexTemp.m_y,vertexTemp.m_z);
    // add vertex to vector
    _vertexes.push_back(vertex);

    initialCenter+=vertex;
  }

  // get the mean of the point cloud
  initialCenter/=int(_numberOfVertexes);

  size_t a,b,c;
  for(auto i = poly.triangles_.begin(); i != poly.triangles_.end(); i++)
  {
    a = (*i).a;
    b = (*i).b;
    c = (*i).c;
    // std::cout << a << ' ' << b << ' ' << c << " -->  ";

    // check triangle orientation
    orderTriangle(a,c,b);

    // get info. on the three edges
    id = getEdgeKey(a,b);
    e1 = id;
    orderedFaces.insert(std::make_pair(id,index));
    if(edgesByKey.insert(std::make_pair(id,torusIndex)).second)
    {
      // order edges by vertex
      orderedEdges.insert(std::make_pair(a,torusIndex));
      orderedEdges.insert(std::make_pair(b,torusIndex));
      torusIndex++;
    }

    id = getEdgeKey(b,c);
    e2 = id;
    orderedFaces.insert(std::make_pair(id,index));
    if(edgesByKey.insert(std::make_pair(id,torusIndex)).second)
    {
      // order edges by vertex
      orderedEdges.insert(std::make_pair(b,torusIndex));
      orderedEdges.insert(std::make_pair(c,torusIndex));
      torusIndex++;
    }

    id = getEdgeKey(c,a);
    e3 = id;
    orderedFaces.insert(std::make_pair(id,index));
    if(edgesByKey.insert(std::make_pair(id,torusIndex)).second)
    { 
      // order edges by vertex
      orderedEdges.insert(std::make_pair(c,torusIndex));
      orderedEdges.insert(std::make_pair(a,torusIndex));
      torusIndex++;
    }

    // add ccw triangle to the vector
    _SCHtriangles.push_back(SCHtriangle(a,b,c,
                            edgesByKey[e1],edgesByKey[e2],edgesByKey[e3]));
    
    //increase triangle index
    index++;
  }

  // get the edges
  edgesByKey.clear();
  for(auto i = _SCHtriangles.begin(); i != _SCHtriangles.end(); i++)
  {
     // get edge id
    id = getEdgeKey((*i).p1, (*i).p2);
    if(edgesByKey.insert(std::make_pair(id,torusIndex)).second)
    {
      // get faces
      auto it = orderedFaces.equal_range(id);
      for(auto j = it.first; j != it.second; j++)
        tempX.push_back((*j).second);
      // add edge to vector
      _SCHedges.push_back(SCHedge((*i).p1,(*i).p2,tempX[0],tempX[1]));
      tempX.clear();
    }

    // get edge id
    id = getEdgeKey((*i).p2, (*i).p3);
    if(edgesByKey.insert(std::make_pair(id,torusIndex)).second)
    {
      // get faces
      auto it = orderedFaces.equal_range(id);
      for(auto j = it.first; j != it.second; j++)
        tempX.push_back((*j).second);
      // add edge to vector
      _SCHedges.push_back(SCHedge((*i).p2,(*i).p3,tempX[0],tempX[1]));
      tempX.clear();
    }

    // get edge id
    id = getEdgeKey((*i).p3, (*i).p1);
    if(edgesByKey.insert(std::make_pair(id,torusIndex)).second)
    {
      // get faces
      auto it = orderedFaces.equal_range(id);
      for(auto j = it.first; j != it.second; j++)
        tempX.push_back((*j).second);
      // add edges to vector
      _SCHedges.push_back(SCHedge((*i).p3,(*i).p1,tempX[0],tempX[1]));
      tempX.clear();
    }
  }

  // get vertex neighbours
  for(size_t i = 0; i < _numberOfVertexes; i++)
  {
    // get all neighbours for the current vertex
    auto neighbours = orderedEdges.equal_range(i);
    // add them to a vector
    for(auto j = neighbours.first; j!= neighbours.second; j++)
      tempX.push_back((*j).second);
    // add SCHvertex to vector
    _SCHvertexes.push_back(SCHvertex(_vertexes[i],tempX));
    tempX.clear();
  }

} // initialize

/*
 *   Builds small spheres from the vertexes obtained with
 *   Polyhedron_algorithm. Radius is equal to _r
 */
void SchCreator3D::getSmallSpheres()
{
  std::cout << "Finding small spheres... ";
  size_t  index = 0;
  for(auto i = _SCHvertexes.begin(); i != _SCHvertexes.end(); i++)
  {
    if((*i).inHull)
    {
      // get small spheres
      _smallSpheres.push_back(Sphere((*i).vertex,_r));
      i->ssIndex = index;
      index++;
    }
  }
  std::cout << _smallSpheres.size() << ' ';
  std::cout << "Done." << std::endl;
} // getSmallSpheres

void SchCreator3D::updateVertexesIndex()
{
  size_t index = 0;
  for(size_t i = 0; i < _SCHvertexes.size(); i++)
  {
    v.push_back(index);
    if(_SCHvertexes[i].inHull)
      index++;
  }
}

double SchCreator3D::angleBetween(Eigen::Vector3d a, Eigen::Vector3d b)
{
  return 180 * std::atan2(a.normalized().cross(b.normalized()).norm(),
                    a.normalized().dot(b.normalized())) / PI;
}

/*
*   If true, the points are ccw, else, they are cw
*/
bool SchCreator3D::checkOrientation(size_t a, size_t b, size_t c)
{
  Sphere s = findSphereThroughPoints(a,b,c);
  Eigen::Vector3d ab = _vertexes[b] - _vertexes[a], ac = _vertexes[c] - _vertexes[a];
  Eigen::Vector3d Ca = _vertexes[a] - s.center, Ao = _vertexes[a] - initialCenter;

  // check if points are cw on the 2d plane
  Plane p = findPlaneBase(a,b,c);
    // find the 2D projection of the points using the base
  Eigen::Vector2d a2d(0, 0), b2d = p.base * ab, c2d = p.base * ac,
                  ab2d = b2d-a2d, ac2d = c2d - a2d; 
  Eigen::Matrix2d m;
  m << ab2d.transpose(), ac2d.transpose();
  // get the determinant of m
  double d = m.determinant();

  // check if the vectors form a to the  sphere center and the initial center
  // are in the same direction as the plane normal
  // std::cout << "\nDirection of normal and Ca: " << p.normal.dot(Ca.normalized()) << '\n'
  //           << "Direction of normal and Ao: " << p.normal.dot(Ao.normalized()) << '\n'
  //           << "Orientation of the triangle on the plane: " << d << std::endl;

  return !(d > 0 && ((p.normal.dot(Ca) > 0) && (p.normal.dot(Ao) > 0)));
} // checkOrientation

/*
 *   Builds the big spheres from the triangles obtained with
 *   Polyhedron_algorithms and adds them to a vector.
 *   Adds the circum radius and the triangle index to the heap.
 */
void SchCreator3D::getBigSpheres()
{
  std::cout << "Finding big spheres..." << std::endl;
  Sphere s;
  BigSphere bs;
  size_t index = _smallSpheres.size();

  // std::cout << "-------------------" << std::endl;

  for(auto i = _SCHtriangles.begin(); i != _SCHtriangles.end(); i++)
  {
    if(!(*i).inHull) continue;

    s = findSphereThroughPoints((*i).p1, (*i).p2, (*i).p3);
    bs = BigSphere(s,v[(*i).p1],v[(*i).p2],v[(*i).p3]);

    // std::cout << "-------------------" << std::endl;
    // add to big sphere vector
    _bigSpheres.push_back(bs);
    i->bsIndex = index;

    // increase the current triangle index
    index++;
  }
  std::cout << _bigSpheres.size();
  std::cout << " Done." << std::endl;
} // SchCreator3D::getBigSpheres

/*
 *   Given 3 points, the function returns the circumsphere
 */
SchCreator3D::Sphere SchCreator3D::findCircumSphere3(size_t a, size_t b, size_t c)
{
  // find vectors a to b and a to c
  Eigen::Vector3d ab, ac;
  ab = _vertexes[b] - _vertexes[a];
  ac = _vertexes[c] - _vertexes[a];

  // compute dot products
  double v11 = ab.dot(ab);
  double v22 = ac.dot(ac);
  double v12 = ab.dot(ac);

  double num = v11 * v22 - pow(v12, 2);
  double k1 = 0.5 * v22 * (v11 - v12) / num;
  double k2 = 0.5 * v11 * (v22 - v12) / num;

  Eigen::Vector3d center = _vertexes[a] + k1 * ab + k2 * ac;
  double radius = (k1 * ab + k2 * ac).norm();

  return Sphere(center, radius);
} // SchCreator3D::findCircumSphere3

/*
*   Given the indexes of four points, finds the circumsphere that
*   touches said indexes.
*/
SchCreator3D::Sphere SchCreator3D::findCircumSphere4(size_t a,
                                                     size_t b,
                                                     size_t c,
                                                     size_t d)
{
  // ussing crammer's rule find base matrix
  Eigen::Matrix4d T;
  T << _vertexes[a][0], _vertexes[a][1], _vertexes[a][2], 1,
       _vertexes[b][0], _vertexes[b][1], _vertexes[b][2], 1,
       _vertexes[c][0], _vertexes[c][1], _vertexes[c][2], 1,
       _vertexes[d][0], _vertexes[d][1], _vertexes[d][2], 1;

  // get squared norm values
  Eigen::Vector4d t;
  t << -_vertexes[a].squaredNorm(), -_vertexes[b].squaredNorm(),
       -_vertexes[c].squaredNorm(), -_vertexes[d].squaredNorm();

  // from base matrix get Mn
  Eigen::Matrix4d M1, M2, M3, M4;
  M1 << t, T.rightCols(3);
  M2 << T.col(0), t, T.rightCols(2);
  M3 << T.leftCols(2), t, T.col(3);
  M4 << T.leftCols(3), t;

  // get the determinants
  double Tdet = T.determinant();

  if(Tdet == 0) 
    Tdet = 1e-10;
  
  double D = M1.determinant() / Tdet, E = M2.determinant() / Tdet, 
         F = M3.determinant() / Tdet, G = M4.determinant() / Tdet;

  double radius = 0.5 * sqrt(pow(D, 2) + pow(E, 2) + pow(F, 2) - 4 * G);
  return Sphere(Eigen::Vector3d(-D / 2, -E / 2, -F / 2), radius);
} // findCircumSphere4


/*
 *   Given 3 points, the function returns the Sphere of radius R
 *   on whose surface lay the points.
 */
SchCreator3D::Sphere SchCreator3D::findSphereThroughPoints(size_t a, size_t b, size_t c)
{
  // get plane base and normal
  SchCreator3D::Plane p = findPlaneBase(a, c, b);
  Eigen::MatrixXd base = p.base;
  Eigen::Vector3d n = p.normal;

  // find the 2D projection of the points using the base
  Eigen::Vector3d ab = _vertexes[b] - _vertexes[a], ac = _vertexes[c] - _vertexes[a];
  Eigen::Vector2d a2d(0, 0), b2d = base * ab, c2d = base * ac;

  // get the center of the circle (in 2D)
  Eigen::Vector2d circleCenter2D = findCircleThroughPoints(a2d, b2d, c2d);

  // convert coordinates to 3D
  Eigen::Vector3d circleCenter3D;
  circleCenter3D(0) = _vertexes[a](0) + circleCenter2D.dot(base.col(0));
  circleCenter3D(1) = _vertexes[a](1) + circleCenter2D.dot(base.col(1));
  circleCenter3D(2) = _vertexes[a](2) + circleCenter2D.dot(base.col(2));

  // get sphere and circle radius
  // double circleRadius = circleCenter2D.norm();
  double sphereRadius = _desiredAlpha;

  // check that sphere's radius is larger than the circle's,
  // else, increase the sphere radius
  // while(sphereRadius < circleRadius) sphereRadius += 0.125;

  // get the distance from the center of the sphere
  double distanceFromCenter;
  if(circleCenter2D.norm() > sphereRadius)
    distanceFromCenter = 0;
  else
    distanceFromCenter = sqrt(pow(sphereRadius, 2) - circleCenter2D.squaredNorm());

  // find the center of the sphere
  Eigen::Vector3d sphereCenter = circleCenter3D + (distanceFromCenter + _epsilon) * n;

  // std::cout << sphereRadius << ' ' << sphereCenter[0] << ' ' << sphereCenter[1] << ' ' << sphereCenter[2] << std::endl;

  return Sphere(sphereCenter, _R);
} // SchCreator3D::findSphereThroughPoints

SchCreator3D::Sphere SchCreator3D::findSphereThroughPoints(const SchCreator3D::SCHtriangle &t)
{
  return findSphereThroughPoints(t.p1,t.p2,t.p3);
}

SchCreator3D::Sphere SchCreator3D::findSphereThroughPoints(size_t a,SchCreator3D::Plane p,
                                                           Eigen::Vector2d circleCenter2D)
{
  return findSphereThroughPoints(a,p,circleCenter2D,_alpha);
} // SchCreator3D::findSphereThroughPoints

SchCreator3D::Sphere SchCreator3D::findSphereThroughPoints(size_t a,SchCreator3D::Plane p,
                                                           Eigen::Vector2d circleCenter2D,
                                                           double R)
{
  // convert coordinates to 3D
  Eigen::Vector3d circleCenter3D;
  circleCenter3D(0) = _vertexes[a](0) + circleCenter2D.dot(p.base.col(0));
  circleCenter3D(1) = _vertexes[a](1) + circleCenter2D.dot(p.base.col(1));
  circleCenter3D(2) = _vertexes[a](2) + circleCenter2D.dot(p.base.col(2));

  // get sphere and circle radius
  // double circleRadius = circleCenter2D.norm();
  double sphereRadius = R;

  // check that sphere's radius is larger than the circle's,
  // else, increase the sphere radius
  // while(sphereRadius < circleRadius) sphereRadius += 0.125;

  // get the distance from the center of the sphere
  double distanceFromCenter = sqrt(pow(sphereRadius, 2) - circleCenter2D.squaredNorm());

  // find the center of the sphere
  Eigen::Vector3d sphereCenter = circleCenter3D + (distanceFromCenter + _epsilon) * p.normal;

  // std::cout << sphereRadius << ' ' << sphereCenter[0] << ' ' << sphereCenter[1] << ' ' << sphereCenter[2] << std::endl;

  return Sphere(sphereCenter, R);
} // SchCreator3D::findSphereThroughPoints

/*
 *   Given the index of 3 points, the function finds the plane that touches them
 *   and returns a matrix containing the plane's base vectors.
 */
SchCreator3D::Plane SchCreator3D::findPlaneBase(size_t a, size_t b, size_t c)
{
  // find the normal to the plane
  Eigen::Vector3d ab = _vertexes[b] - _vertexes[a], bc = _vertexes[c] - _vertexes[b];
  Eigen::Vector3d n = ab.cross(bc).normalized();

  // find base vector ex and ey
  Eigen::Vector3d ex = ab.cross(n).normalized();
  Eigen::Vector3d ey = n.cross(ex);
  Eigen::MatrixXd base(2, 3);
  base << ex.transpose(), ey.transpose();

  return Plane(base, n, a, b, c);
} // SchCreator3D::findPlaneBase

/*
 *   Given 3 points, the function returns the center of the circumcircle.
 *
 *   NOTE: Points must be 2D.
 */
Eigen::Vector2d SchCreator3D::findCircleThroughPoints(const Eigen::Vector2d & a,
                                                      const Eigen::Vector2d & b,
                                                      const Eigen::Vector2d & c)
{
  // Using crammer's rule, find the base matrix
  Eigen::MatrixXd A(3, 4);
  A << pow(a.norm(), 2), a[0], a[1], 1,
       pow(b.norm(), 2), b[0], b[1], 1,
       pow(c.norm(), 2), c[0], c[1], 1;

  // From the base matrix get M1n
  Eigen::Matrix3d M11 = A.block(0, 1, 3, 3), M12, M13, M14;
  M12 << A.leftCols(1), A.rightCols(2);
  M13 << A.leftCols(2), A.rightCols(1);
  M14 << A.leftCols(3);

  // compute the center's x and y coordinates
  Eigen::Vector2d center;
  center(0) = 0.5 * (M12.determinant() / M11.determinant());
  center(1) = -0.5 * (M13.determinant() / M11.determinant());

  return center;
} // SchCreator3D::findCircleThroughPoints

Eigen::Vector2d SchCreator3D::findCircleThroughPoints(size_t a,
                                                      size_t b,
                                                      size_t c,
                                                      SchCreator3D::Plane p)
{
  // find the 2D projection of the points using the base
  Eigen::Vector3d ab = _vertexes[b] - _vertexes[a], ac = _vertexes[c] - _vertexes[a];
  Eigen::Vector2d a2d(0, 0), b2d = p.base * ab, c2d = p.base * ac;
  findCircleThroughPoints(a2d, b2d, c2d);
  // Using crammer's rule, find the base matrix
  Eigen::MatrixXd A(3, 4);
  A << pow(a2d.norm(), 2), a2d[0], a2d[1], 1,
       pow(b2d.norm(), 2), b2d[0], b2d[1], 1,
       pow(c2d.norm(), 2), c2d[0], c2d[1], 1;

  // From the base matrix get M1n
  Eigen::Matrix3d M11 = A.block(0, 1, 3, 3), M12, M13, M14;
  M12 << A.leftCols(1), A.rightCols(2);
  M13 << A.leftCols(2), A.rightCols(1);
  M14 << A.leftCols(3);

  // compute the center's x and y coordinates
  Eigen::Vector2d center;
  center(0) = 0.5 * (M12.determinant() / M11.determinant());
  center(1) = -0.5 * (M13.determinant() / M11.determinant());

  return center;
} // SchCreator3D::findCircleThroughPoints

/*
 *   Get all torii, its cones and all big sphere plane normals.
 */
void SchCreator3D::getTorii()
{
  std::cout << "Finding torii...";
  size_t torusIndex = _smallSpheres.size() + _bigSpheres.size(),
         index = _smallSpheres.size();
  std::multimap<size_t, SCHplane> orderedPlanes;
  Eigen::Vector3d planeNormal;
  Face face;
  Torus torus;
  SCHplane plane;

  // go through all edges
  for(auto i = _SCHedges.begin(); i != _SCHedges.end(); i++)
  {
    // check if the edge is part of the hull
    if(i->inHull)
    {
      // get the torus
      torus = getTorus(_bigSpheres[_SCHtriangles[i->face1].bsIndex].s,_SCHvertexes[i->vertex1].ssIndex,_SCHvertexes[i->vertex2].ssIndex);
      // add torus to _torii
      _torii.push_back(torus);
      // get torus cones                torusIndex, pair(SCHcone,SCHcone)
      _toriiCones.insert(std::make_pair(torusIndex,getCones(_SCHvertexes[i->vertex1].ssIndex,_SCHvertexes[i->vertex2].ssIndex,torus.normal)));
      // set edge.torusIndex to torusIndex
      i->torusIndex = torusIndex;

      torusIndex++;
    }
  }

  for(auto i = _SCHtriangles.begin(); i != _SCHtriangles.end(); i++)
  {
    if(i->inHull)
    {
      // get the torus index
      torusIndex = _SCHedges[i->e1].torusIndex;

      // get the first plane normal
      planeNormal = getPlaneNormal(_SCHvertexes[(*i).p1].ssIndex, _SCHvertexes[(*i).p2].ssIndex, _bigSpheres[i->bsIndex].s.center);
      face.plane1 = std::make_pair(torusIndex, planeNormal);

      torusIndex = _SCHedges[i->e2].torusIndex;

      // get the second plane normal
      planeNormal = getPlaneNormal(_SCHvertexes[(*i).p2].ssIndex, _SCHvertexes[(*i).p3].ssIndex, _bigSpheres[i->bsIndex].s.center);
      face.plane2 = std::make_pair(torusIndex, planeNormal);

      torusIndex = _SCHedges[i->e3].torusIndex;

      // get the third plane normal
      planeNormal = getPlaneNormal(_SCHvertexes[(*i).p3].ssIndex, _SCHvertexes[(*i).p1].ssIndex, _bigSpheres[i->bsIndex].s.center);
      face.plane3 = std::make_pair(torusIndex, planeNormal);

      // add face to sphere normals
      _bigSphereNormals.insert(std::make_pair(i->bsIndex,face));
    }
  }

  index = _smallSpheres.size();
  for(auto i = _bigSphereNormals.begin(); i != _bigSphereNormals.end(); i++)
  {
    // add all big sphere normals to the map:
    // orderedPlanes<torus index,<triangle index,plane normal>>
    orderedPlanes.insert(std::make_pair((*i).second.plane1.first,
                                        std::make_pair(index,
                                                      (*i).second.plane1.second)));
    orderedPlanes.insert(std::make_pair((*i).second.plane2.first,
                                        std::make_pair(index,
                                                      (*i).second.plane2.second)));
    orderedPlanes.insert(std::make_pair((*i).second.plane3.first,
                                        std::make_pair(index,
                                                      (*i).second.plane3.second)));
    // all keys have exactly two SCHplane elements
    index++;
  }

  index = _smallSpheres.size() + _bigSpheres.size();
  for(auto i = _toriiCones.begin(); i != _toriiCones.end(); i++)
  {
    // get the iterator pointing to planes of torus i->first
    auto it = orderedPlanes.equal_range(i->first);

    // get the plane normal of the first plane
    plane = (*it.first).second;
    // advance the iterator to get second plane
    it.first++;
    // insert to _toriiPlanes
                                    // torus index
    _toriiPlanes.insert(std::make_pair(i->first,
         // first SCHplane, second SCHplane
         std::make_pair(plane,(*it.first).second)));
  }

  std::cout << " Done." << std::endl;
} // getTorii

/*
*   Builds a pair of SCHcones given two vertex indexes and a torus normal
*/
std::pair<SchCreator3D::SCHcone, SchCreator3D::SCHcone>
          SchCreator3D::getCones(size_t a,size_t b,const Eigen::Vector3d & n)
{
  SCHcone abCone, baCone;
  // compute the cosine
  double cosine = getCosine(a, b);
  // get cone [a,b]
  abCone.first = a;
  abCone.second = Cone(n, cosine);
  // get cone [b,a]
  baCone.first = b;
  baCone.second = Cone(-n, cosine);
  return std::make_pair(abCone, baCone);
}

/*
 *   Computes a Cone's cosine given the indexes of the vertices.
 */
double SchCreator3D::getCosine(size_t a, size_t b)
{
  return (_smallSpheres[a].center - _smallSpheres[b].center).norm() / (2 * _desiredAlpha);
}

/*
 *   Gets the normal of a plane given two point indexes and a point.
 */
Eigen::Vector3d SchCreator3D::getPlaneNormal(size_t a, size_t b, Eigen::Vector3d c)
{
  // find the normal to the plane
  Eigen::Vector3d ca = _smallSpheres[a].center - c, 
                  cb = _smallSpheres[b].center - c;
  return ca.cross(cb).normalized();
  // else return -ab.cross(bc).normalized();
}

Eigen::Vector3d SchCreator3D::getPlaneNormal(size_t a, size_t b, size_t f)
{
  // find the sphere center 
  Eigen::Vector3d c = findSphereThroughPoints(_SCHtriangles[f]).center;
  // find the normal to the plane
  Eigen::Vector3d ca = _vertexes[a] - c, 
                  cb = _vertexes[b] - c;
  // std::cout << c.transpose() << "\n"
  //           << ca.transpose() << "\n"
  //           << cb.transpose() << "\n";
  return ca.cross(cb).normalized();
}

/*
 *   Builds the Torus given a sphere and two points.
 */
SchCreator3D::Torus SchCreator3D::getTorus(const Sphere & s, size_t a, size_t b)
{
  Eigen::Vector3d center = (_smallSpheres[a].center + _smallSpheres[b].center) / 2;
  return Torus(center, (_smallSpheres[a].center - _smallSpheres[b].center).normalized(),
              (center - s.center).norm());
}

/*
 *   Finds a unique key/ID given two numbers.
 */
size_t SchCreator3D::getEdgeKey(size_t a, size_t b)
{
  return ((a < b) ? (a * _numberOfVertexes + b) : (b * _numberOfVertexes + a));
}

/*
 *   Finds a unique key/ID given an edge index.
 */
size_t SchCreator3D::getEdgeKey(size_t edge)
{
  SCHedge e = _SCHedges[edge];
  return ((e.vertex1 < e.vertex2) ? (e.vertex1 * _numberOfVertexes + e.vertex2) : (e.vertex2 * _numberOfVertexes + e.vertex1));
}

/*
 *   Finds a key/ID given two numbers.
 */
size_t SchCreator3D::getKey(size_t a, size_t b)
{
  return (a * _numberOfVertexes + b);
}

/*
*   Finds the neighboring cones to every vertex.
*/
void SchCreator3D::getVertexNeighbours()
{
  size_t index = 0;
  Cone cone;
  std::multimap<size_t, size_t> orderedEdges;
  _vertexNeighbours.resize(_smallSpheres.size());
  std::vector<std::vector<size_t>> edgeIDs;
  edgeIDs.resize(_smallSpheres.size());
  std::set<size_t> neighbours;

  for(auto i = _SCHvertexes.begin(); i != _SCHvertexes.end(); i++)
  {
    if(!(*i).inHull) continue;
    for(auto j = (*i).neighbours.begin(); j != (*i).neighbours.end(); j++)
    {
      if(_SCHedges[*j].inHull)
      {
        edgeIDs[index].push_back(_SCHedges[*j].torusIndex);
      }
    }
    neighbours.clear();
    index++;
  }
  
  index = 0;

  for(auto i = _smallSpheres.begin(); i != _smallSpheres.end(); i++)
  {
      for(auto j = edgeIDs[index].begin(); j != edgeIDs[index].end(); j++)
      {
        if((*j) == _toriiCones[*j].first.first)
          cone = _toriiCones[*j].first.second;
        else
          cone = _toriiCones[*j].second.second;
        _vertexNeighbours[index].push_back(std::make_pair(*j, cone));
      }

    index++;
  }
} // getVertexNeighbours();

void SchCreator3D::getHeap()
{
  std::set<size_t> indexes;
  Sphere s;
  std::vector<SCHheap> tempHeap;

  size_t index = 0;
  for(auto i = _SCHtriangles.begin(); i != _SCHtriangles.end(); i++)
  {
    // get circum sphere
    s = findCircumSphere3((*i).p1, (*i).p2, (*i).p3);
    tempHeap.push_back(SCHheap(s.radius,type(triangle),index));
    // temp.push_back(SCHheap(s.radius,type(triangle),index));
    // std::cout << (*i).p1 << ' ' << (*i).p2 << ' ' << (*i).p3 << ' ' << s.radius << std::endl;

    index++;
  }

  index = 0;
  for(auto i = _SCHedges.begin(); i != _SCHedges.end(); i++)
  {
    // insert circumradius to the heap
    tempHeap.push_back(SCHheap(getEdgeHeap(*i),type(edge),index));
    // temp.push_back(SCHheap(getEdgeHeap(*i),type(edge),index));
    index++;
  }

  _heap = std::priority_queue<SCHheap>(tempHeap.begin(),tempHeap.end());
} // getHeap

double SchCreator3D::getEdgeHeap(const SchCreator3D::SCHedge &e)
{
  size_t vertex3, vertex4;
  Sphere s;
  // get the four vertexes
  vertex3 = findVertex(_SCHtriangles[e.face1],e.vertex1,e.vertex2);
  vertex4 = findVertex(_SCHtriangles[e.face2],e.vertex1,e.vertex2);
  // compute the circum sphere
  s = findCircumSphere4(e.vertex1,e.vertex2,vertex3,vertex4);
  std::cout << e.vertex1 << ' ' << e.vertex2 << ' ' << vertex3 << ' ' << vertex4
            << ' ' << s.radius << std::endl;
  return s.radius;
} // getEdgeHeap

size_t SchCreator3D::findVertex(const SCHtriangle &t,size_t a,size_t b)
{
  bool inSet;
  std::set<size_t> indexes;

  // add known vertexes to the set
  indexes.insert(a);
  indexes.insert(b);

  // add first sphere vertex
  inSet = indexes.insert(t.p1).second;
  // if p1 is not in the set, return p1
  if(inSet) return t.p1;
  // else, try adding p2 to the set
  else inSet = indexes.insert(t.p2).second;
  // if p2 isn't in the set, return p2
  if(inSet) return t.p2;
  // else, return p3
  else return t.p3;
} // findVertex

void SchCreator3D::swap(size_t &a, size_t &b)
{
  size_t temp = a;
  a = b;
  b = temp;
} // swap

void SchCreator3D::orderTriangle(size_t a, size_t &b, size_t &c)
{
  if(!checkOrientation(a,b,c))
    swap(b,c);
} // orderTriangle

bool SchCreator3D::checkLimitCase(size_t a, size_t b, size_t c, size_t d, double r)
{
  if(!checkPointsInSphere(a,b,c,d))
    {
      std::cout << "\nNo more vertexes can be removed. Stoping the algorithm at radius " 
                << r << "...\n" << std::endl;
      return true;
  }
  
  return false;
}

void SchCreator3D::changeTopology(SCHheap heap)
{
  size_t f1, f2;
  size_t a, b, c, d, i;
  SCHneighbours fg, hj;

  a = _SCHedges[heap.index].vertex1;
  b = _SCHedges[heap.index].vertex2;
  f1 = _SCHedges[heap.index].face1;
  f2 = _SCHedges[heap.index].face2;
  c = findVertex(f1,a,b);
  d = findVertex(f2,a,b);

  if(c == d)
  {
    std::cout << "Same triangle" << std::endl;
  }

  fg = findEdge(f1, f2, a, c, d);
  hj = findEdge(f1, f2, b, c, d);


  std::cout << "RELEVANT DATA" << std::endl;
  std::cout << a << ' ' << b << ' ' << c << ' ' << d << std::endl;
  std::cout << fg.first << ' ' << fg.second << ' ' << hj.first << ' ' << hj.second << std::endl;

  std::cout << "Check if cd have the same neighbour: ";
  if(!checkSameNeighbour(c,d,type(edge)))
  {
    std::cout << "0. \nCheck if f and h have the same neighbout that is not " << f1 << " or " << f2 << ": ";
    if(checkSameNeighbour(fg.first,hj.first,f1) && checkSameNeighbour(fg.first,hj.first,f2))
    {
      std::cout << "1. " << std::endl;
    } else
      std::cout << "0. " << std::endl;


    std::cout << "Invert Edge\n" << std::endl;
    invertEdge(heap,c,d,fg,hj);

  } else
  {
    i = findEdge(c,d);

    if(_activeVertexes == 4)
    {
      if(checkLimitCase(a,b,c,d,heap.radius))
      {
        if(checkLimitCase(b,a,c,d,heap.radius))
        {
          return;
        }
        else
        {
          std::cout << "Dissapear vertex. LIMIT CASE. " << b << '\n' << std::endl;
          dissapearVertex(heap,b,c,d,a,hj,fg,i);
        }
      } 
      else
      {
        std::cout << "Dissapear vertex. LIMIT CASE. " << a << '\n' << std::endl;
        dissapearVertex(heap,a,d,c,b,fg,hj,i);
      }
    } else
    {

      std::cout << "Check which vertex dissapears." << std::endl;
      std::cout << "Do f and i have the same face as neighbour?" << std::endl;


      if(checkSameNeighbour(fg.second,i,type(triangle)))
      {
        std::cout << " Active vertex neighbours: " << findActiveNeighbours(a) << std::endl;
        std::cout << "Check if a Dissapears under: " << std::endl;

        if(checkTorii(heap.index,fg.first,fg.second))
        {
          std::cout << "Dissapear vertex " << a << '\n' << std::endl;
          dissapearVertex(heap,a,d,c,b,fg,hj,i);
        }
        else
        {  
          std::cout << "Invert Edge. Torus Check. \n" << std::endl;
          invertEdge(heap,c,d,fg,hj);
        }
      } else
      {
        std::cout << "0. Active vertex neighbours: " << findActiveNeighbours(b) << std::endl;
        std::cout << "Check if b Dissapears under " << std::endl;
        std::cout << "is triangle " << a << ", "
                  << c << ", " << d << " ccw? " 
                  << checkOrientation(a,c,d) << std::endl;
        
        std::cout << "New check: " << std::endl;
        
        if(checkTorii(heap.index,hj.first,hj.second))
        {
          std::cout << "Dissapear vertex " << b << '\n' << std::endl;
          dissapearVertex(heap,b,c,d,a,hj,fg,i);
        }
        else
        {  
          std::cout << "Invert Edge. Torus Check. \n" << std::endl;
          invertEdge(heap,c,d,fg,hj);
        }
      }
    }
  }
} // changeTopology

size_t SchCreator3D::findCommonFace(size_t e1,size_t e2)
{
  std::set<size_t> faces;
  faces.insert(_SCHedges[e1].face1);
  faces.insert(_SCHedges[e1].face2);

  if(faces.insert(_SCHedges[e2].face1).second)
    return _SCHedges[e2].face1;
  else if(faces.insert(_SCHedges[e2].face2).second)
    return _SCHedges[e2].face2;
  else 
    return _SCHtriangles.size();
}

size_t SchCreator3D::findActiveNeighbours(size_t v)
{
  size_t activeNeighbours = 0;
  for(auto i = _SCHvertexes[v].neighbours.begin(); 
      i !=  _SCHvertexes[v].neighbours.end(); i++)
  {
    if(_SCHedges[*i].inHull) activeNeighbours++;
  }
  return activeNeighbours;
}

void SchCreator3D::dissapearUnderEdge(SchCreator3D::SCHheap heap)
{
  std::cout << "\nDissapear under edge. ";
  // a is the vertex to dissapear. b and c are its immediate neighbours
  size_t a,b,c,d,e;
  size_t e1 = _SCHtriangles[heap.index].e1,
         e2 = _SCHtriangles[heap.index].e2,
         e3 = _SCHtriangles[heap.index].e3, e4;
  // triangle in heap
  size_t f1 = heap.index, f3, f4;
  // get the missing neighbour triangles
  size_t f2 = _SCHedges[e1].face1 == f1 ? _SCHedges[e1].face2 : _SCHedges[e1].face1;
  size_t temp = _SCHedges[e2].face1 == f1 ? _SCHedges[e2].face2 : _SCHedges[e2].face1;
  // if f2 and f3 are the same, the vertex in common is the vertex to dissapear
  if(f2 == temp)
  {
    a = findVertex(e1,e2);
    c = _SCHedges[e1].vertex1 == a ? _SCHedges[e1].vertex2 : _SCHedges[e1].vertex1;
    b = _SCHedges[e2].vertex1 == a ? _SCHedges[e2].vertex2 : _SCHedges[e2].vertex1;
    f3 = _SCHedges[e3].face1 == f1 ? _SCHedges[e3].face2 : _SCHedges[e3].face1;
    e4 = findEdge(f2,e1,e2);
  } else
  {
    f3 = temp;
    // get the face for the last edge
    temp = _SCHedges[e3].face1 == f1 ? _SCHedges[e3].face2 : _SCHedges[e3].face1;
    // if f2 and temp are the same, the vertex in common is the vertex to dissapear
    if(f2 == temp)
    {
      a = findVertex(e1,e3);
      b = _SCHedges[e1].vertex1 == a ? _SCHedges[e1].vertex2 : _SCHedges[e1].vertex1;
      c = _SCHedges[e3].vertex1 == a ? _SCHedges[e3].vertex2 : _SCHedges[e3].vertex1;
      f2 = temp;
      e4 = findEdge(f2,e1,e3);
    } else
    {
      // else, the vertex in common for f3 and temp is the vertex to dissapear
      a = findVertex(e2,e3);
      b = _SCHedges[e3].vertex1 == a ? _SCHedges[e3].vertex2 : _SCHedges[e3].vertex1;
      c = _SCHedges[e2].vertex1 == a ? _SCHedges[e2].vertex2 : _SCHedges[e2].vertex1;
      f2 = f3;
      f3 = temp; 
      e4 = findEdge(f2,e2,e3);
    }
  }

  // find the missing vertex from f3
  d = findVertex(f3,b,c);

  // find the face conecting bce/f4 and e
  f4 = _SCHedges[e4].face1 == f2 ? _SCHedges[e4].face2 : _SCHedges[e4].face1;
  e = findVertex(f4,b,c);

  std::cout << "RELEVANT DATA\n" << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << std::endl;
  std::cout << f1 << ' ' << f2 << ' ' << f3 << ' ' << f4 << std::endl;
  std::cout << e1 << ' ' << e2 << ' ' << e3 << ' ' << e4 << std::endl;
  

  // remove vertex from hull
  _SCHvertexes[a].removeFromHull();


  // remove triangle from hull
  _SCHtriangles[f1].removeFromHull();
  // remove triangle from hull
  _SCHtriangles[f2].removeFromHull();
  // remove triangle from hull
  _SCHtriangles[f3].removeFromHull();
  // remove triangle from hull
  _SCHtriangles[f4].removeFromHull();
  // remove triangles and their edges from hull
  // removeTriangle(f1);
  // removeTriangle(f2);
  // removeTriangle(f3);
  // removeTriangle(f4);

  // get edges for face f3 and f4
  SCHneighbours ef3 = findEdges(f3,d,b,e2);
  SCHneighbours ef4 = findEdges(f4,e,b,e4);

  std::cout << ef3.first << ' ' << ef3.second << ' ' << ef4.first << ' ' << ef4.second << std::endl;

  size_t triangleIndex = _SCHtriangles.size(), 
         edgeIndex = _SCHedges.size();
  
  std::cout <<  "Dissapear vertex " << a 
            << " under edge " << edgeIndex+4 
            << std::endl; 

  // make new triangles
  std::cout << "\nMake new triangles:\n";
  _SCHtriangles.push_back(SCHtriangle(b,d,c,edgeIndex,edgeIndex+1,edgeIndex+4));
  _SCHtriangles.push_back(SCHtriangle(b,c,e,edgeIndex+4,edgeIndex+3,edgeIndex+2));
  std::cout << triangleIndex << ' ' << b << ' ' << d << ' ' << c << ' ' << edgeIndex << ' ' << edgeIndex+1 << edgeIndex+4 << std::endl;
  std::cout << triangleIndex+1 << ' ' << b << ' ' << c << ' ' << e << ' ' << edgeIndex+4 << ' ' << edgeIndex+3 << edgeIndex+2 << std::endl;

  std::cout << "\nMake new edges:\n";

  // update old faces
  updateNeighbours(ef3,triangleIndex);
  updateNeighbours(ef4,triangleIndex+1);

  // make new edges
  makeNewEdge(_SCHedges[ef3.first],ef3.first,edgeIndex,triangleIndex,heap.radius);
  makeNewEdge(_SCHedges[ef3.second],ef3.second,edgeIndex+1,triangleIndex,heap.radius);
  makeNewEdge(_SCHedges[ef4.first],ef4.first,edgeIndex+2,triangleIndex+1,heap.radius);
  makeNewEdge(_SCHedges[ef4.second],ef4.second,edgeIndex+3,triangleIndex+1,heap.radius);

  // remove edges
  _SCHedges[e1].removeFromHull();
  _SCHedges[e2].removeFromHull();
  _SCHedges[e3].removeFromHull();
  _SCHedges[e4].removeFromHull();

  _SCHedges[ef3.first].removeFromHull();
  _SCHedges[ef3.second].removeFromHull();
  _SCHedges[ef4.first].removeFromHull();
  _SCHedges[ef4.second].removeFromHull();

  // make the edge under which the vertex dissapears
  SCHedge newEdge = SCHedge(b,c,triangleIndex+1,triangleIndex);
  _SCHedges.push_back(newEdge);
  std::cout << edgeIndex+4 << ' ';
  double radius = getEdgeHeap(newEdge);
  if(radius < heap.radius)
    _heap.push(SCHheap(radius,type(edge),edgeIndex+4));
  else 
  {
    std::cout << "This radius is larger than current max heap" << std::endl;
  }
  addToVertexNeighbours(edgeIndex+4);


} // dissapearUnderEdge


SchCreator3D::SCHneighbours SchCreator3D::findEdges(size_t f, size_t a, size_t b, size_t e)
{
  SCHneighbours edges;
  size_t key = getEdgeKey(a,b);
  size_t e1 = _SCHtriangles[f].e1 == e ? _SCHtriangles[f].e3 : _SCHtriangles[f].e1,
         e2 = _SCHtriangles[f].e2 == e ? _SCHtriangles[f].e3 : _SCHtriangles[f].e2;
  if(getEdgeKey(e1) == key)
  {
    edges.first = e1;
    edges.second = e2;
  }
  else
  {
    edges.first = e2;
    edges.second = e1;
  }

  return edges;
} // findEdges


void SchCreator3D::removeTriangle(size_t t)
{
  // remove triangle from hull
  _SCHtriangles[t].removeFromHull();
  // remove triangle edges from hull
  _SCHedges[_SCHtriangles[t].e1].removeFromHull();
  _SCHedges[_SCHtriangles[t].e2].removeFromHull();
  _SCHedges[_SCHtriangles[t].e3].removeFromHull();
}

size_t SchCreator3D::findEdge(size_t f, size_t e1, size_t e2)
{
  bool inSet;
  std::set<size_t> indexes;
  
  // insert the known edges
  indexes.insert(e1);
  indexes.insert(e2);
  
  // insert the first edge
  inSet = indexes.insert(_SCHtriangles[f].e1).second;
  // if the edge is inserted to the set, return its index
  if(inSet)
    return _SCHtriangles[f].e1;
  // else insert the next edge
  else
    inSet = indexes.insert(_SCHtriangles[f].e2).second;

  // if the edge is inserted to the set, return its index
  if(inSet)
    return _SCHtriangles[f].e2;
  // else return the last edge
  else
    return _SCHtriangles[f].e3;
} // findEdge


void SchCreator3D::removeNeighboursFromHull(size_t v)
{
  for(auto i = _SCHvertexes[v].neighbours.begin();
      i !=  _SCHvertexes[v].neighbours.end(); i++)
    _SCHedges[*i].removeFromHull();

} // removeNeighboursFromHull

void SchCreator3D::dissapearVertex(SCHheap heap, size_t v1, size_t v2, size_t v3 , size_t v4,
                                        SCHneighbours e12,SCHneighbours e34, size_t e)
{
  // remove elements from hull
  _SCHvertexes[v1].removeFromHull();
  removeNeighboursFromHull(v1);
  _SCHtriangles[_SCHedges[heap.index].face1].removeFromHull();
  _SCHtriangles[_SCHedges[heap.index].face2].removeFromHull();

  // remove common face
  if(!_SCHtriangles[_SCHedges[e12.first].face1].inHull)
    _SCHtriangles[_SCHedges[e12.first].face2].removeFromHull();
  else _SCHtriangles[_SCHedges[e12.first].face1].removeFromHull();

  if(!_SCHtriangles[_SCHedges[e12.second].face1].inHull)
    _SCHtriangles[_SCHedges[e12.second].face2].removeFromHull();
  else _SCHtriangles[_SCHedges[e12.second].face1].removeFromHull();   

  // make new triangle
  std::cout << "New triangle: " << std::endl;
  size_t newT = _SCHtriangles.size(), index = _SCHedges.size();

  _SCHtriangles.push_back(SCHtriangle(v4,v3,v2,index+2,index+1,index));

  // update triangles
  updateNeighbours(e34,newT);
  updateNeighbours(e,newT);

  // insert triangle to heap
  std::cout << v2 << ' ' << v3 << ' ' << v4 << ' ' << findCircumSphere3(v2,v3,v4).radius << std::endl;

  double ccradius = findCircumSphere3(v2,v3,v4).radius;
  if(ccradius > heap.radius)
    std::cout << "New heap is larger than current max heap" << std::endl;
  _heap.push(SCHheap(ccradius,type(triangle),newT));
  // temp.push_back(SCHheap(findCircumSphere3(v2,v3,v4).radius,type(triangle),newT));

  // insert edge to heap, update edge
  std::cout << "New edges: " << std::endl;
  makeNewEdge(_SCHedges[e34.first],e34.first,index,newT,heap.radius);
  makeNewEdge(_SCHedges[e],e,index+1,newT,heap.radius);
  makeNewEdge(_SCHedges[e34.second],e34.second,index+2,newT,heap.radius);

  // remove old edges from hull
  _SCHedges[e34.first].removeFromHull();
  _SCHedges[e].removeFromHull();
  _SCHedges[e34.second].removeFromHull();
  
  _activeVertexes--;
} // dissapearVertex

void SchCreator3D::addToVertexNeighbours(size_t e)
{
  _SCHvertexes[_SCHedges[e].vertex1].neighbours.push_back(e);
  _SCHvertexes[_SCHedges[e].vertex2].neighbours.push_back(e);
} // addToVertexNeighbours

size_t SchCreator3D::findEdge(size_t v1, size_t v2)
{
  std::vector<size_t>::iterator i;
  for(i = _SCHvertexes[v1].neighbours.begin();
      i != _SCHvertexes[v1].neighbours.end(); i++)
  {
    // std::cout << _SCHedges[*i].vertex1 << ' ' << _SCHedges[*i].vertex2 << std::endl;
    if(_SCHedges[*i].vertex1 == v2 || _SCHedges[*i].vertex2 == v2)
      // std::cout << '\n' << *i << '\n' << std::endl;
      if(_SCHedges[*i].inHull)
        break;
  }

  return *i;
} // findEdge

void SchCreator3D::invertEdge(SCHheap heap, size_t v3, size_t v4, 
                              SCHneighbours e12, SCHneighbours e34)
{
  // remove edges and traingles from heap
  _SCHedges[heap.index].removeFromHull();
  _SCHtriangles[_SCHedges[heap.index].face1].removeFromHull();
  _SCHtriangles[_SCHedges[heap.index].face2].removeFromHull();

  // add the new edge
  size_t index = _SCHtriangles.size();
  _SCHedges.push_back(SCHedge(v3,v4,index,index+1));
  addToVertexNeighbours(_SCHedges.size()-1);
  
  
  // make new edges
  updateNeighbours(e12,index+1);
  updateNeighbours(e34,index);

  // get the new triangles
  index = _SCHedges.size()-1;

  std::cout << "New triangles: " << std::endl;
  std::cout << _SCHtriangles.size() << ' ' << _SCHedges[heap.index].vertex2
            << ' ' << v3 << ' ' << v4 << ' '
            << findCircumSphere3(_SCHedges[heap.index].vertex2,v3, v4).radius
            << std::endl;
  _SCHtriangles.push_back(SCHtriangle(_SCHedges[heap.index].vertex2,
                                        v3, v4,index+3,index,index+4));

  std::cout << _SCHtriangles.size() << ' ' << _SCHedges[heap.index].vertex1
            << ' ' << v4 << ' ' << v3 << ' '
            << findCircumSphere3(_SCHedges[heap.index].vertex1,v3, v4).radius
            << std::endl;
  _SCHtriangles.push_back(SCHtriangle(_SCHedges[heap.index].vertex1,
                                        v4, v3,index+2,index,index+1));
  // orderTriangle(_SCHedges[heap.index].vertex2,v3,v4);

  // insert new triangles to the heap
  _heap.push(SCHheap(findCircumSphere3(_SCHedges[heap.index].vertex1,
                                      v3, v4).radius,type(triangle),_SCHtriangles.size()-1));
  _heap.push(SCHheap(findCircumSphere3(_SCHedges[heap.index].vertex2,
                                      v3, v4).radius,type(triangle),_SCHtriangles.size()-2));
                                      
  // add edges to the hull and heap again
  std::cout << "New edges: " << std::endl;
  std::cout << _SCHedges.size()-1 << ' ' << v3 << ' ' << v4 << std::endl; 
  makeNewEdge(_SCHedges[e12.first], e12.first,_SCHedges.size(),_SCHtriangles.size()-1,heap.radius);
  makeNewEdge(_SCHedges[e12.second], e12.second,_SCHedges.size(),_SCHtriangles.size()-1,heap.radius);
  makeNewEdge(_SCHedges[e34.first], e34.first,_SCHedges.size(),_SCHtriangles.size()-2,heap.radius);
  makeNewEdge(_SCHedges[e34.second], e34.second,_SCHedges.size(),_SCHtriangles.size()-2,heap.radius);

  // remove edges from hull
  _SCHedges[e12.first].removeFromHull();
  _SCHedges[e12.second].removeFromHull();
  _SCHedges[e34.first].removeFromHull();
  _SCHedges[e34.second].removeFromHull();

} // invertEdge

void SchCreator3D::makeNewEdge(const SchCreator3D::SCHedge &e, size_t prevE, size_t newE, size_t t, double maxHeap)
{
  double radius;
  _SCHedges.push_back(e);
  updateNeighbours(prevE,t,newE);
  std::cout << newE << ' ';

  radius = getEdgeHeap(_SCHedges[newE]);
  if(radius < maxHeap)
    _heap.push(SCHheap(radius,type(edge),newE));
  else 
  {
    std::cout << "New heap is larger than current max heap" << std::endl;
  }
  addToVertexNeighbours(newE);
} // makeNewHeap

void SchCreator3D::updateNeighbours(SchCreator3D::SCHneighbours e, size_t index)
{
  updateNeighbours(e.first, index);
  updateNeighbours(e.second, index);
} // updateNeighbours

void SchCreator3D::updateNeighbours(size_t e, size_t index)
{
  if(!_SCHtriangles[_SCHedges[e].face1].inHull)
    _SCHedges[e].face1 = index;
  else
    _SCHedges[e].face2 = index;
} // updateNeighbours


void SchCreator3D::updateNeighbours(size_t oldE, size_t f, size_t e)
{
  size_t face;
  // find the face to update
  if(_SCHedges[e].face1 == f) 
    face = _SCHedges[e].face2;
  else 
    face = _SCHedges[e].face1;  
  std::cout << "Face to update: " << face << " edge: " << e << " old edge: " << oldE << std::endl;
  
  // find the old edge and update it
  if(_SCHtriangles[face].e1 == oldE) 
    _SCHtriangles[face].e1 = e;
  else if(_SCHtriangles[face].e2 == oldE) 
    _SCHtriangles[face].e2 = e;
  else if(_SCHtriangles[face].e3 == oldE) 
    _SCHtriangles[face].e3 = e;
} // updateNeighbours

bool SchCreator3D::checkSameNeighbour(size_t e1, size_t e2, size_t f1)
{
  bool check = (_SCHedges[e1].face1 == _SCHedges[e2].face1) && _SCHedges[e1].face1 != f1;
  check += (_SCHedges[e1].face1 == _SCHedges[e2].face2) && _SCHedges[e1].face1 != f1;
  check += (_SCHedges[e1].face2 == _SCHedges[e2].face1) && _SCHedges[e1].face2 != f1;
  check += (_SCHedges[e1].face2 == _SCHedges[e2].face2) && _SCHedges[e1].face2 != f1;

  return check;
}

bool SchCreator3D::checkSameNeighbour(size_t v1, size_t v2, type t)
{
  if(t==0)
  {
    for(auto i = _SCHvertexes[v1].neighbours.begin();
        i != _SCHvertexes[v1].neighbours.end(); i++)
    {
      if(_SCHedges[*i].vertex1 == v2 || _SCHedges[*i].vertex2 == v2)
      {
        if(!_SCHedges[*i].inHull && i != (_SCHvertexes[v1].neighbours.end()-1))
          continue;
        std::cout << "i: " << *i << std::endl;
        return _SCHedges[*i].inHull;
      }
    }
  } else
  {
    if(_SCHedges[v1].face1 == _SCHedges[v2].face1)
    {
      std::cout << "1. Face: ";
      std::cout << _SCHedges[v1].face1 << ' ' << _SCHedges[v2].face1 << std::endl;
      return true;
    }
    else if(_SCHedges[v1].face1 == _SCHedges[v2].face2)
    {
      std::cout << "1. Face: ";
      std::cout << _SCHedges[v1].face1 << ' ' << _SCHedges[v2].face2 << std::endl;
      return true;
    }
    else if(_SCHedges[v1].face2 == _SCHedges[v2].face1)
    {
      std::cout << "1. Face: ";
      std::cout << _SCHedges[v1].face2 << ' ' << _SCHedges[v2].face1 << std::endl;
      return true;
    } else if(_SCHedges[v1].face2 == _SCHedges[v2].face2)
    {
      std::cout << "1. Face: ";
      std::cout << _SCHedges[v1].face2 << ' ' << _SCHedges[v2].face2 << std::endl;
      return true;
    }
  }

  return false;
} // checkSameNeighbour


SchCreator3D::SCHneighbours SchCreator3D::findEdge(size_t f1, size_t f2, size_t v1, size_t v2, size_t v3)
{
  SCHneighbours edges;
  std::map<size_t,size_t> tempEdges = {{getEdgeKey(_SCHtriangles[f1].e1),_SCHtriangles[f1].e1},
                                   {getEdgeKey(_SCHtriangles[f1].e2),_SCHtriangles[f1].e2},
                                   {getEdgeKey(_SCHtriangles[f1].e3),_SCHtriangles[f1].e3},
                                   {getEdgeKey(_SCHtriangles[f2].e1),_SCHtriangles[f2].e1},
                                   {getEdgeKey(_SCHtriangles[f2].e2),_SCHtriangles[f2].e2},
                                   {getEdgeKey(_SCHtriangles[f2].e3),_SCHtriangles[f2].e3}};
  
  edges.first = tempEdges[getEdgeKey(v1,v2)];
  edges.second = tempEdges[getEdgeKey(v1,v3)];
  
  return edges;
} // findEdge

size_t SchCreator3D::findVertex(size_t e1, size_t e2)
{
  bool inSet;
  std::set<size_t> indexes;
  
  // add vertexes of the first edge
  indexes.insert(_SCHedges[e1].vertex1);
  indexes.insert(_SCHedges[e1].vertex2);

  // add first vertex of second edge
  inSet = indexes.insert(_SCHedges[e2].vertex1).second;
  // if this vertex is not in the set, return the last vertex
  if(inSet) 
    return _SCHedges[e2].vertex2;
  // otherwise, return this vertex
  else 
    return _SCHedges[e2].vertex1;
} // findVertex

size_t SchCreator3D::findVertex(size_t f,size_t a,size_t b)
{
  bool inSet;
  std::set<size_t> indexes;

  // add known vertexes to the set
  indexes.insert(a);
  indexes.insert(b);

  // add first sphere vertex
  inSet = indexes.insert(_SCHtriangles[f].p1).second;
  // if p1 is not in the set, return p1
  if(inSet) return _SCHtriangles[f].p1;
  // else, try adding p2 to the set
  else inSet = indexes.insert(_SCHtriangles[f].p2).second;

  // if p2 isn't in the set, return p2
  if(inSet) return _SCHtriangles[f].p2;
  // else, return p3
  else return _SCHtriangles[f].p3;
} // insertToSet

/*
 *   Creates the output file given a filename.
 */
void SchCreator3D::writeToFile(const std::string & filename)
{
  size_t index = 0;
  // std::vector<std::pair<size_t,Eigen::Vector3d>> SCHvertexes;
  std::vector<std::pair<size_t,BigSphere>> SCHspheres;
  std::vector<std::pair<size_t,Torus>> SCHtorii;

  // for(auto i = _SCHvertexes.begin(); i != _SCHvertexes.end(); i++)
  // {
  //   if((*i).inHull)
  //     SCHvertexes.push_back(std::make_pair(index,(*i).vertex));
  //   index++;
  // }

  index = _smallSpheres.size();
  for(auto i = _bigSpheres.begin(); i != _bigSpheres.end(); i++)
  {
    SCHspheres.push_back(std::make_pair(index,(*i)));
    index++;
  }

  index = _smallSpheres.size() + _bigSpheres.size();
  for(auto i = _torii.begin(); i != _torii.end(); i++)
  {
    SCHtorii.push_back(std::make_pair(index,(*i)));
    index++;
  }

  // write data into a file
  std::ofstream os;
  os.open(filename.c_str());
  os.precision(16);

  // radii used for computation on first line
  os << _r << ' ' << _R << std::endl;

  // number of small spheres
  os << ' ' << _smallSpheres.size() << std::endl;
  // writes small spheres
  index = 0;
  for(auto i = _smallSpheres.begin(); i != _smallSpheres.end(); i++)
  {
    os << (*i).radius << ' ';
    os << (*i).center[0] << ' ';
    os << (*i).center[1] << ' ';
    os << (*i).center[2] << std::endl;

    // number of neighbours
    os << _vertexNeighbours[index].size() << std::endl;
    for(auto j = _vertexNeighbours[index].begin();
        j != _vertexNeighbours[index].end(); j++)
    {
      // neighbours
      os << (*j).first << ' ' << (*j).second.cosangle << ' ';
      os << (*j).second.axis[0] << ' ';
      os << (*j).second.axis[1] << ' ';
      os << (*j).second.axis[2] << std::endl;
    }

    index++;
  }

  // number of big spheres
  index = 0;
  os << SCHspheres.size() << std::endl;
  for(auto i = SCHspheres.begin(); i != SCHspheres.end(); i++)
  {
    // radius and center of big sphere
    os << _R << ' ' << (*i).second.s.center[0] << ' ';
    os << (*i).second.s.center[1] << ' '
       << (*i).second.s.center[2] << std::endl;

    // coordinates of the three points touching the sphere
    os << _smallSpheres[(*i).second.p1].center[0] << ' '
       << _smallSpheres[(*i).second.p1].center[1] << ' '
       << _smallSpheres[(*i).second.p1].center[2] << ' ';
    os << _smallSpheres[(*i).second.p2].center[0] << ' '
       << _smallSpheres[(*i).second.p2].center[1] << ' '
       << _smallSpheres[(*i).second.p2].center[2] << ' ';
    os << _smallSpheres[(*i).second.p3].center[0] << ' '
       << _smallSpheres[(*i).second.p3].center[1] << ' '
       << _smallSpheres[(*i).second.p3].center[2] << std::endl;

    // normals to the sphere planes
    os << _bigSphereNormals[(*i).first].plane1.first << ' ';
    os << _bigSphereNormals[(*i).first].plane1.second[0] << ' '
       << _bigSphereNormals[(*i).first].plane1.second[1] << ' '
       << _bigSphereNormals[(*i).first].plane1.second[2] << std::endl;

    os << _bigSphereNormals[(*i).first].plane2.first << ' ';
    os << _bigSphereNormals[(*i).first].plane2.second[0] << ' '
       << _bigSphereNormals[(*i).first].plane2.second[1] << ' '
       << _bigSphereNormals[(*i).first].plane2.second[2] << std::endl;

    os << _bigSphereNormals[(*i).first].plane3.first << ' ';
    os << _bigSphereNormals[(*i).first].plane3.second[0] << ' '
       << _bigSphereNormals[(*i).first].plane3.second[1] << ' '
       << _bigSphereNormals[(*i).first].plane3.second[2] << std::endl;
  }

  // number of torii
  os << SCHtorii.size() << std::endl;
  for(auto i = SCHtorii.begin(); i != SCHtorii.end(); i++)
  {
    // remove torus boolean
    os << 1 << std::endl;

    // write torus' external radius, center and normal coordinates
    os << (*i).second.extRadius << ' ' << _R << ' ';

    os << (*i).second.center[0] << ' '
       << (*i).second.center[1] << ' '
       << (*i).second.center[2] << ' ';

    os << (*i).second.normal[0] << ' '
       << (*i).second.normal[1] << ' '
       << (*i).second.normal[2] << std::endl;

    // write torus' cones
    // cone [a,b]: a index, cosangle, axis coordinates
    os << _toriiCones[(*i).first].first.first << ' ';
    os << _toriiCones[(*i).first].first.second.cosangle << ' ';
    os << _toriiCones[(*i).first].first.second.axis[0] << ' '
       << _toriiCones[(*i).first].first.second.axis[1] << ' '
       << _toriiCones[(*i).first].first.second.axis[2] << std::endl;

    // cone [b,a]: b index, cosangle, axis coordinates
    os << _toriiCones[(*i).first].second.first << ' ';
    os << _toriiCones[(*i).first].second.second.cosangle << ' ';
    os << _toriiCones[(*i).first].second.second.axis[0] << ' '
       << _toriiCones[(*i).first].second.second.axis[1] << ' '
       << _toriiCones[(*i).first].second.second.axis[2] << std::endl;

    // write torus' planes
    // first triangle: triangle index, normal coordinates
    os << _toriiPlanes[(*i).first].first.first << ' ';
    os << _toriiPlanes[(*i).first].first.second[0] << ' '
       << _toriiPlanes[(*i).first].first.second[1] << ' '
       << _toriiPlanes[(*i).first].first.second[2] << std::endl;

    // second triangle: triangle index, normal coordinates
    os << _toriiPlanes[(*i).first].second.first << ' ';
    os << _toriiPlanes[(*i).first].second.second[0] << ' '
       << _toriiPlanes[(*i).first].second.second[1] << ' '
       << _toriiPlanes[(*i).first].second.second[2] << std::endl;
  }

  // close the file
  os.close();

  std::cout << "\nSCH Created, output file " << filename.c_str() << std::endl;
}

void SchCreator3D::printEdges()
{
  size_t index = 0;
  std::cout << "\nedges " << _SCHedges.size() << std::endl;
  std::cout << "n\tVertex\tFaces\tInHull\n";

  for(auto i = _SCHedges.begin(); i != _SCHedges.end(); i++)
  {
    std::cout << index << '\t' << (*i).vertex1 << ' '
              << (*i).vertex2 << '\t'
              << (*i).face1 << ' '
              << (*i).face2 << '\t'
              << (*i).inHull << '\n';
    index++;
  }
}

void SchCreator3D::printVertexes()
{
  std::cout << "\nvertexes " << std::endl;
  size_t index = 0;
  std::cout << "\nIndex\tInHull " << std::endl;
  for(auto i = _SCHvertexes.begin(); i != _SCHvertexes.end(); i++)
  {
    std::cout << index << '\t' << (*i).inHull << std::endl;
    std::cout << "neighbours: ";
    for(auto j = (*i).neighbours.begin(); j != (*i).neighbours.end(); j++)
      std::cout << *j << ' ';

    std::cout << std::endl;
    index++;
  }
}

void SchCreator3D::printTriangles()
{
  size_t index = 0;
  std::cout << "\nfaces " << _SCHtriangles.size() << std::endl;
  std::cout << "n\tVertexes\tEdges   \tInHull" << std::endl;

  for(auto i = _SCHtriangles.begin(); i != _SCHtriangles.end(); i++)
  {
    std::cout << index << '\t' << (*i).p1 << ' '  << (*i).p2 << ' '  << (*i).p3 << "\t\t";
    std::cout << (*i).e1 << ' '  << (*i).e2 << ' '  << (*i).e3 << "   \t";
    std::cout << (*i).inHull << std::endl;
    index++;
  }

}

void SchCreator3D::printHeap()
{
  std::priority_queue<SCHheap> heap(_heap);
  size_t x =  _heap.size();
  for(size_t i = 0; i < x; i++)
  {
    std::cout << i << ' ' << heap.top().radius
              << ' ' << heap.top().t
              << ' ' << heap.top().index << std::endl;
    heap.pop();
  }
}

bool SchCreator3D::checkHeap()
{
  bool check = true;
  switch (_heap.top().t)
  {
  case 0:
    check *= _SCHedges[_heap.top().index].inHull;
    check *= _SCHvertexes[_SCHedges[_heap.top().index].vertex1].inHull;
    check *= _SCHvertexes[_SCHedges[_heap.top().index].vertex2].inHull;
    check *= _SCHtriangles[_SCHedges[_heap.top().index].face1].inHull;
    check *= _SCHtriangles[_SCHedges[_heap.top().index].face2].inHull;
    break;
  default:
    check *= _SCHtriangles[_heap.top().index].inHull;
    break;
  }

  return check;
} // checkHeap

bool SchCreator3D::checkTorii(size_t e, size_t e1, size_t e2)
{
  std::cout << "\nChecking thickness of edge " << e << ": " << std::endl; 
  bool check = torusThicknessCheck(_SCHedges[e].vertex1,_SCHedges[e].vertex2,
                                  _SCHedges[e].face1,_SCHedges[e].face2);
  
  std::cout << check << "\nChecking thickness of edge " << e1 << ": " << std::endl; 
  check *= torusThicknessCheck(_SCHedges[e1].vertex1,_SCHedges[e1].vertex2,
                                  _SCHedges[e1].face1,_SCHedges[e1].face2);
  
  std::cout << check << "\nChecking thickness of edge " << e2 << ": " << std::endl; 
  check *= torusThicknessCheck(_SCHedges[e2].vertex1,_SCHedges[e2].vertex2,
                                  _SCHedges[e2].face1,_SCHedges[e2].face2);
  std::cout << check << std::endl;
  return check;
}

/*  Given the indexes of two vertexex and two faces that contain them, this function computes 
*   the dot product of their plane normals.
*   
*   If the dot product is equal or close to -1, the function returns true.
*/
bool SchCreator3D::torusThicknessCheck(size_t v1, size_t v2, size_t f1, size_t f2)
{
  Eigen::Vector3d n1 = getPlaneNormal(v1,v2,f1), n2 = getPlaneNormal(v2,v1,f2);
  // std::cout << "normal1 a b c1: " << n1.transpose() 
  //           << "\nnormal2 b a c2: " << n2.transpose() << std::endl;

  double dotProduct = n1.dot(n2), d  = dotProduct + 1; 
  std::cout << "n1.n2: " << dotProduct << "  " << d*d << ' ';
  // std::cout << "\td*d < epsilon: " << d*d << " < " << _epsilon << std::endl;

  return (d*d) <= _epsilon;
} // torusThicknessCheck

void SchCreator3D::computeSCH(const std::string & filename)
{
  // Read points from file with poly_algorithms
  poly.openFromFile(filename);
  // get no. of vertex
  _numberOfVertexes = poly.vertexes_.size();
  _vertexes.reserve(_numberOfVertexes);
  // std::cout << "No. Vertex: " << _numberOfVertexes << std::endl;
  // std::cout << "No. Triangles: " << poly.triangles_.size() << std::endl;
  // std::cout << "No. Edges: " << poly.edges_.size() << std::endl;

  // Ensure alpha is smaller than the max. body distance,
  if(findMaxDistance())
  {
    std::cout << "ERROR, impossible to compute SCH, choose larger R." << std::endl;
    std::cout << "Finished." << std::endl;
    return;
  }

  initialize();
  _activeVertexes = _vertexes.size();
  getHeap();


  printVertexes();
  printEdges();
  printTriangles();

  std::cout << _heap.size() << std::endl;
  SCHheap heap = _heap.top();

  _alpha = heap.radius;
  
  std::cout << "\n-------------------------------" << std::endl;

  std::cout << "Max Heap: " << _alpha << ' '
              << _heap.top().t << ' ' << _heap.top().index << std::endl;

    std::cout << "Triangles: " << _SCHtriangles.size();
    std::cout << "\tEdges: " << _SCHedges.size() << std::endl;

  while(_alpha > _desiredAlpha)
  {
    size_t tindex = 0;
    for(auto i = _SCHtriangles.begin(); i != _SCHtriangles.end(); i++)
    {
      if(i->inHull)
      {
        if(!_SCHedges[i->e1].inHull || !_SCHedges[i->e2].inHull || !_SCHedges[i->e3].inHull)
        {
          std::cout << "The triangle index " << tindex << ' ' << i->e1 << ' ' << i->e2 << ' ' << i->e3 << std::endl;
          std::cout << "The triangle index " << i->inHull << ' ' << _SCHedges[i->e1].inHull << ' ' << _SCHedges[i->e2].inHull << ' ' << _SCHedges[i->e3].inHull << std::endl;
          exit(1);
        }
      }
      tindex++;
    }

    if(_heap.size() == 0 || (_activeVertexes <= 4 && heap.t == 1))
      break;
  
    if(heap.t == 0)
    {
      _heap.pop();
      changeTopology(heap);
    } else
      dissapearUnderEdge(heap);

    bool check = checkHeap();
    std::cout << "\nCheck if current heap exists... \n";
    while(!check)
    {
      std::cout << "Max Heap: " << _heap.top().radius << ' '
                << _heap.top().t << ' ' << _heap.top().index << " Target alpha: " << _desiredAlpha;
      std::cout << " Check: " << check << std::endl;
      _heap.pop();
      check = checkHeap();
    }
    std::cout << " Done." << std::endl;

    heap = _heap.top();
    _alpha = heap.radius;

    printVertexes();
    printEdges();
    printTriangles();

    std::cout << "\n-------------------------------" << std::endl;

    std::cout << "Max Heap: " << _alpha << ' '
              << heap.t << ' ' << heap.index << " Target alpha: " << _desiredAlpha << std::endl;
  }



  updateVertexesIndex();
  // get spheres
  getSmallSpheres();
  getBigSpheres();
  getTorii();
  getVertexNeighbours();
  
  // std::priority_queue<SCHheap> vHeap = std::priority_queue<SCHheap>(temp.begin(),temp.end());
  // std::cout << "HEAP" << std::endl;
  // std::cout << std::setprecision(10);
  // for(size_t i = 0; i < vHeap.size(); i++)
  // {
  //   std::cout << vHeap.top().radius << ' ' << vHeap.top().index << ' ' << vHeap.top().t;
  //   bool check = (vHeap.top().t == 0) ? _SCHedges[vHeap.top().index].inHull:_SCHtriangles[vHeap.top().index].inHull;
  //   std::cout << ' ' << check << std::endl;
  //   vHeap.pop();
  // }
} // computeSCH
} // namespace sch

// ./build/src/sch-creator-3d /home/amrf/balloon-inflating/sch-visualization/tests/shared-tests/data/sample_polyhedron.otp
int main(int argc, char ** argv)
{
  namespace po = boost::program_options;

  double r, R;

  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "r,r", po::value<double>(&r)->default_value(.02),
      "small sphere radius")("R,R", po::value<double>(&R)->default_value(300.),
                             "big sphere radius")("input-file", po::value<std::string>(),
                                                  "input file")("output-file", po::value<std::string>(), "output file");

  po::positional_options_description pos;
  pos.add("input-file", 1);
  pos.add("output-file", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);
  po::notify(vm);

  if(vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 1;
  }

  std::cout << "\n SCH parameters: r = " << r << ", R = " << R << std::endl << std::endl;

  if(vm.count("input-file") && vm.count("output-file"))
  {
    std::fstream testfile;
    std::string input = vm["input-file"].as<std::string>();
    std::string output = vm["output-file"].as<std::string>();

    std::cout << "Opening " << input << std::endl;
    sch::SchCreator3D sch(r, R);
    sch.computeSCH(input);

    sch.writeToFile(output);

    // /home/amrf/balloon-inflating/sch-visualization/tests/shared-tests/data/sample_polyhedron.otp
  }

  return 0;
}