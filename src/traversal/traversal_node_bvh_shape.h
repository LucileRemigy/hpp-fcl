/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011-2014, Willow Garage, Inc.
 *  Copyright (c) 2014-2015, Open Source Robotics Foundation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan */


#ifndef HPP_FCL_TRAVERSAL_NODE_MESH_SHAPE_H
#define HPP_FCL_TRAVERSAL_NODE_MESH_SHAPE_H

/// @cond INTERNAL

#include <hpp/fcl/collision_data.h>
#include <hpp/fcl/shape/geometric_shapes.h>
#include "../src/shape/geometric_shapes_utility.h"
#include "traversal_node_base.h"
#include "details/traversal.h"
#include <hpp/fcl/BVH/BVH_model.h>


namespace hpp
{
namespace fcl
{

/// @addtogroup Traversal_For_Collision
/// @{

/// @brief Traversal node for collision between BVH and shape
template<typename BV, typename S>
class BVHShapeCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  BVHShapeCollisionTraversalNode(const CollisionRequest& request) :
  CollisionTraversalNodeBase (request)
  {
    model1 = NULL;
    model2 = NULL;

    num_bv_tests = 0;
    num_leaf_tests = 0;
    query_time_seconds = 0.0;
  }

  /// @brief Whether the BV node in the first BVH tree is leaf
  bool isFirstNodeLeaf(int b) const
  {
    return model1->getBV(b).isLeaf();
  }

  /// @brief Obtain the left child of BV node in the first BVH
  int getFirstLeftChild(int b) const
  {
    return model1->getBV(b).leftChild();
  }

  /// @brief Obtain the right child of BV node in the first BVH
  int getFirstRightChild(int b) const
  {
    return model1->getBV(b).rightChild();
  }

  const BVHModel<BV>* model1;
  const S* model2;
  BV model2_bv;

  mutable int num_bv_tests;
  mutable int num_leaf_tests;
  mutable FCL_REAL query_time_seconds;
};

/// @brief Traversal node for collision between shape and BVH
template<typename S, typename BV>
class ShapeBVHCollisionTraversalNode : public CollisionTraversalNodeBase
{
public:
  ShapeBVHCollisionTraversalNode(const CollisionRequest& request) :
  CollisionTraversalNodeBase(request)
  {
    model1 = NULL;
    model2 = NULL;

    num_bv_tests = 0;
    num_leaf_tests = 0;
    query_time_seconds = 0.0;
  }

  /// @brief Alway extend the second model, which is a BVH model
  bool firstOverSecond(int, int) const
  {
    return false;
  }

  /// @brief Whether the BV node in the second BVH tree is leaf
  bool isSecondNodeLeaf(int b) const
  {
    return model2->getBV(b).isLeaf();
  }

  /// @brief Obtain the left child of BV node in the second BVH
  int getSecondLeftChild(int b) const
  {
    return model2->getBV(b).leftChild();
  }

  /// @brief Obtain the right child of BV node in the second BVH
  int getSecondRightChild(int b) const
  {
    return model2->getBV(b).rightChild();
  }

  const S* model1;
  const BVHModel<BV>* model2;
  BV model1_bv;

  mutable int num_bv_tests;
  mutable int num_leaf_tests;
  mutable FCL_REAL query_time_seconds;
};


/// @brief Traversal node for collision between mesh and shape
template<typename BV, typename S,
  int _Options = RelativeTransformationIsIdentity>
class MeshShapeCollisionTraversalNode : public BVHShapeCollisionTraversalNode<BV, S>
{
public:
  enum {
    Options = _Options,
    RTIsIdentity = _Options & RelativeTransformationIsIdentity
  };

  MeshShapeCollisionTraversalNode(const CollisionRequest& request) :
  BVHShapeCollisionTraversalNode<BV, S> (request)
  {
    vertices = NULL;
    tri_indices = NULL;

    nsolver = NULL;
  }

  /// @brief BV culling test in one BVTT node
  bool BVDisjoints(int b1, int /*b2*/) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    if (RTIsIdentity)
      return !this->model1->getBV(b1).bv.overlap(this->model2_bv);
    else
      return !overlap(this->tf1.getRotation(), this->tf1.getTranslation(), this->model2_bv, this->model1->getBV(b1).bv);
  }

  /// test between BV b1 and shape
  /// @param b1 BV to test,
  /// @retval sqrDistLowerBound square of a lower bound of the minimal
  ///         distance between bounding volumes.
  /// @brief BV culling test in one BVTT node
  bool BVDisjoints(int b1, int /*b2*/, FCL_REAL& sqrDistLowerBound) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    bool res;
    if (RTIsIdentity)
      res = !this->model1->getBV(b1).bv.overlap(this->model2_bv, this->request, sqrDistLowerBound);
    else
      res = !overlap(this->tf1.getRotation(), this->tf1.getTranslation(),
                      this->model2_bv, this->model1->getBV(b1).bv,
                      this->request, sqrDistLowerBound);
    assert (!res || sqrDistLowerBound > 0);
    return res;
  }

  /// @brief Intersection testing between leaves (one triangle and one shape)
  void leafCollides(int b1, int /*b2*/, FCL_REAL& sqrDistLowerBound) const
  {
    if(this->enable_statistics) this->num_leaf_tests++;
    const BVNode<BV>& node = this->model1->getBV(b1);

    int primitive_id = node.primitiveId();

    const Triangle& tri_id = tri_indices[primitive_id];

    const Vec3f& p1 = vertices[tri_id[0]];
    const Vec3f& p2 = vertices[tri_id[1]];
    const Vec3f& p3 = vertices[tri_id[2]];

    FCL_REAL distance;
    Vec3f normal;
    Vec3f c1, c2; // closest point

    bool collision;
    if (RTIsIdentity) {
      static const Transform3f Id;
      collision =
        nsolver->shapeTriangleInteraction(*(this->model2), this->tf2, p1, p2, p3,
                                          Id       , distance, c2, c1, normal);
    } else {
      collision =
        nsolver->shapeTriangleInteraction(*(this->model2), this->tf2, p1, p2, p3,
                                          this->tf1, distance, c2, c1, normal);
    }

    if(collision) {
      if(this->request.num_max_contacts > this->result->numContacts())
      {
        this->result->addContact(Contact(this->model1, this->model2,
                                         primitive_id, Contact::NONE,
                                         c1, -normal, -distance));
        assert (this->result->isCollision ());
        return;
      }
    }
    sqrDistLowerBound = distance * distance;
    assert (distance > 0);
    if (   this->request.security_margin > 0
        && distance <= this->request.security_margin)
    {
      this->result->addContact(Contact(this->model1, this->model2,
                                       primitive_id, Contact::NONE,
                                       .5 * (c1+c2), (c2-c1).normalized (),
                                       -distance));
    }
    assert (!this->result->isCollision () || sqrDistLowerBound > 0);
  }

  /// @brief Whether the traversal process can stop early
  bool canStop() const
  {
    return this->request.isSatisfied(*(this->result));
  }

  Vec3f* vertices;
  Triangle* tri_indices;

  const GJKSolver* nsolver;
};

/// @brief Traversal node for mesh and shape, when mesh BVH is one of the oriented node (OBB, RSS, OBBRSS, kIOS)
template<typename S>
class MeshShapeCollisionTraversalNodeOBB : public MeshShapeCollisionTraversalNode<OBB, S, 0>
{
public:
  MeshShapeCollisionTraversalNodeOBB(const CollisionRequest& request) :
  MeshShapeCollisionTraversalNode<OBB, S, 0>
    (request)
  {
  }

};

template<typename S>
class MeshShapeCollisionTraversalNodeRSS : public MeshShapeCollisionTraversalNode<RSS, S, 0>
{
public:
  MeshShapeCollisionTraversalNodeRSS (const CollisionRequest& request):
  MeshShapeCollisionTraversalNode<RSS, S, 0>
    (request)
  {
  }
};

template<typename S>
class MeshShapeCollisionTraversalNodekIOS : public MeshShapeCollisionTraversalNode<kIOS, S, 0>
{
public:
  MeshShapeCollisionTraversalNodekIOS(const CollisionRequest& request):
  MeshShapeCollisionTraversalNode<kIOS, S, 0>
    (request)
  {
  }
};

template<typename S>
class MeshShapeCollisionTraversalNodeOBBRSS : public MeshShapeCollisionTraversalNode<OBBRSS, S, 0>
{
public:
  MeshShapeCollisionTraversalNodeOBBRSS (const CollisionRequest& request) :
  MeshShapeCollisionTraversalNode <OBBRSS, S, 0>
    (request)
  {
  }
};


/// @brief Traversal node for collision between shape and mesh
template<typename S, typename BV,
  int _Options = RelativeTransformationIsIdentity>
class ShapeMeshCollisionTraversalNode : public ShapeBVHCollisionTraversalNode<S, BV>
{
public:
  enum {
    Options = _Options,
    RTIsIdentity = _Options & RelativeTransformationIsIdentity
  };

  ShapeMeshCollisionTraversalNode() : ShapeBVHCollisionTraversalNode<S, BV>()
  {
    vertices = NULL;
    tri_indices = NULL;

    nsolver = NULL;
  }

  /// BV test between b1 and b2
  /// @param b2 Bounding volumes to test,
  bool BVDisjoints(int /*b1*/, int b2) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    if (RTIsIdentity)
      return !this->model2->getBV(b2).bv.overlap(this->model1_bv);
    else
      return !overlap(this->tf2.getRotation(), this->tf2.getTranslation(), this->model1_bv, this->model2->getBV(b2).bv);
  }

  /// BV test between b1 and b2
  /// @param b2 Bounding volumes to test,
  /// @retval sqrDistLowerBound square of a lower bound of the minimal
  ///         distance between bounding volumes.
  bool BVDisjoints(int /*b1*/, int b2, FCL_REAL& sqrDistLowerBound) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    bool res;
    if (RTIsIdentity)
      res = !this->model2->getBV(b2).bv.overlap(this->model1_bv, sqrDistLowerBound);
    else
      res = !overlap(this->tf2.getRotation(), this->tf2.getTranslation(),
                     this->model1_bv, this->model2->getBV(b2).bv,
                     sqrDistLowerBound);
    assert (!res || sqrDistLowerBound > 0);
    return res;
  }

  /// @brief Intersection testing between leaves (one shape and one triangle)
  void leafCollides(int /*b1*/, int b2, FCL_REAL& sqrDistLowerBound) const
  {
    if(this->enable_statistics) this->num_leaf_tests++;
    const BVNode<BV>& node = this->model2->getBV(b2);

    int primitive_id = node.primitiveId();

    const Triangle& tri_id = tri_indices[primitive_id];

    const Vec3f& p1 = vertices[tri_id[0]];
    const Vec3f& p2 = vertices[tri_id[1]];
    const Vec3f& p3 = vertices[tri_id[2]];

    FCL_REAL distance;
    Vec3f normal;
    Vec3f c1, c2; // closest points

    bool collision;
    if (RTIsIdentity) {
      static const Transform3f Id;
      collision =
        nsolver->shapeTriangleInteraction(*(this->model1), this->tf1, p1, p2, p3,
                                          Id       , c1, c2, distance, normal);
    } else {
      collision =
        nsolver->shapeTriangleInteraction(*(this->model1), this->tf1, p1, p2, p3,
                                          this->tf2, c1, c2, distance, normal);
    }

    if (collision) {
      if(this->request.num_max_contacts > this->result->numContacts())
      {  
        this->result->addContact (Contact(this->model1 , this->model2,
                                          Contact::NONE, primitive_id,
                                          c1, normal, -distance));
        assert (this->result->isCollision ());
        return;
      }
    }
    sqrDistLowerBound = distance * distance;
    assert (distance > 0);
    if (   this->request.security_margin == 0
        && distance <= this->request.security_margin)
    {
      this->result.addContact (Contact(this->model1 , this->model2,
                                       Contact::NONE, primitive_id,
                                       .5 * (c1+c2), (c2-c1).normalized (),
                                       -distance));
    }
    assert (!this->result->isCollision () || sqrDistLowerBound > 0);
  }

  /// @brief Whether the traversal process can stop early
  bool canStop() const
  {
    return this->request.isSatisfied(*(this->result));
  }

  Vec3f* vertices;
  Triangle* tri_indices;

  const GJKSolver* nsolver;
};

/// @brief Traversal node for shape and mesh, when mesh BVH is one of the oriented node (OBB, RSS, OBBRSS, kIOS)
template<typename S>
class ShapeMeshCollisionTraversalNodeOBB : public ShapeMeshCollisionTraversalNode<S, OBB, 0>
{
public:
  ShapeMeshCollisionTraversalNodeOBB() : ShapeMeshCollisionTraversalNode<S, OBB>()
  {
  }
};


template<typename S>
class ShapeMeshCollisionTraversalNodeRSS : public ShapeMeshCollisionTraversalNode<S, RSS, 0>
{
public:
  ShapeMeshCollisionTraversalNodeRSS() : ShapeMeshCollisionTraversalNode<S, RSS>()
  {
  }
};


template<typename S>
class ShapeMeshCollisionTraversalNodekIOS : public ShapeMeshCollisionTraversalNode<S, kIOS, 0>
{
public:
  ShapeMeshCollisionTraversalNodekIOS() : ShapeMeshCollisionTraversalNode<S, kIOS>()
  {
  }
};


template<typename S>
class ShapeMeshCollisionTraversalNodeOBBRSS : public ShapeMeshCollisionTraversalNode<S, OBBRSS, 0>
{
public:
  ShapeMeshCollisionTraversalNodeOBBRSS() : ShapeMeshCollisionTraversalNode<S, OBBRSS>()
  {
  }
};

/// @}

/// @addtogroup Traversal_For_Distance
/// @{

/// @brief Traversal node for distance computation between BVH and shape
template<typename BV, typename S>
class BVHShapeDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  BVHShapeDistanceTraversalNode() : DistanceTraversalNodeBase()
  {
    model1 = NULL;
    model2 = NULL;
    
    num_bv_tests = 0;
    num_leaf_tests = 0;
    query_time_seconds = 0.0;
  }

  /// @brief Whether the BV node in the first BVH tree is leaf
  bool isFirstNodeLeaf(int b) const 
  {
    return model1->getBV(b).isLeaf();
  }

  /// @brief Obtain the left child of BV node in the first BVH
  int getFirstLeftChild(int b) const
  {
    return model1->getBV(b).leftChild();
  }

  /// @brief Obtain the right child of BV node in the first BVH
  int getFirstRightChild(int b) const
  {
    return model1->getBV(b).rightChild();
  }

  /// @brief BV culling test in one BVTT node
  FCL_REAL BVDistanceLowerBound(int b1, int /*b2*/) const
  {
    return model1->getBV(b1).bv.distance(model2_bv);
  }

  const BVHModel<BV>* model1;
  const S* model2;
  BV model2_bv;

  mutable int num_bv_tests;
  mutable int num_leaf_tests;
  mutable FCL_REAL query_time_seconds;
};

/// @brief Traversal node for distance computation between shape and BVH
template<typename S, typename BV>
class ShapeBVHDistanceTraversalNode : public DistanceTraversalNodeBase
{
public:
  ShapeBVHDistanceTraversalNode() : DistanceTraversalNodeBase()
  {
    model1 = NULL;
    model2 = NULL;
    
    num_bv_tests = 0;
    num_leaf_tests = 0;
    query_time_seconds = 0.0;
  }

  /// @brief Whether the BV node in the second BVH tree is leaf
  bool isSecondNodeLeaf(int b) const
  {
    return model2->getBV(b).isLeaf();
  }

  /// @brief Obtain the left child of BV node in the second BVH
  int getSecondLeftChild(int b) const
  {
    return model2->getBV(b).leftChild();
  }

  /// @brief Obtain the right child of BV node in the second BVH
  int getSecondRightChild(int b) const
  {
    return model2->getBV(b).rightChild();
  }

  /// @brief BV culling test in one BVTT node
  FCL_REAL BVDistanceLowerBound(int b1, int b2) const
  {
    return model1_bv.distance(model2->getBV(b2).bv);
  }

  const S* model1;
  const BVHModel<BV>* model2;
  BV model1_bv;
  
  mutable int num_bv_tests;
  mutable int num_leaf_tests;
  mutable FCL_REAL query_time_seconds;
};
                                  

/// @brief Traversal node for distance between mesh and shape
template<typename BV, typename S>
class MeshShapeDistanceTraversalNode : public BVHShapeDistanceTraversalNode<BV, S>
{ 
public:
  MeshShapeDistanceTraversalNode() : BVHShapeDistanceTraversalNode<BV, S>()
  {
    vertices = NULL;
    tri_indices = NULL;

    rel_err = 0;
    abs_err = 0;

    nsolver = NULL;
  }

  /// @brief Distance testing between leaves (one triangle and one shape)
  void leafComputeDistance(int b1, int /*b2*/) const
  {
    if(this->enable_statistics) this->num_leaf_tests++;
    
    const BVNode<BV>& node = this->model1->getBV(b1);
    
    int primitive_id = node.primitiveId();
    
    const Triangle& tri_id = tri_indices[primitive_id];

    const Vec3f& p1 = vertices[tri_id[0]];
    const Vec3f& p2 = vertices[tri_id[1]];
    const Vec3f& p3 = vertices[tri_id[2]];
    
    FCL_REAL d;
    Vec3f closest_p1, closest_p2, normal;
    nsolver->shapeTriangleInteraction(*(this->model2), this->tf2, p1, p2, p3,
                                      Transform3f (), d, closest_p2, closest_p1,
                                      normal);

    this->result->update(d, this->model1, this->model2, primitive_id,
                         DistanceResult::NONE, closest_p1, closest_p2,
                         normal);
  }

  /// @brief Whether the traversal process can stop early
  bool canStop(FCL_REAL c) const
  {
    if((c >= this->result->min_distance - abs_err) && (c * (1 + rel_err) >= this->result->min_distance))
      return true;
    return false;
  }

  Vec3f* vertices;
  Triangle* tri_indices;

  FCL_REAL rel_err;
  FCL_REAL abs_err;
    
  const GJKSolver* nsolver;
};

/// @cond IGNORE
namespace details
{

template<typename BV, typename S>
void meshShapeDistanceOrientedNodeleafComputeDistance(int b1, int /* b2 */,
                                              const BVHModel<BV>* model1, const S& model2,
                                              Vec3f* vertices, Triangle* tri_indices,
                                              const Transform3f& tf1,
                                              const Transform3f& tf2,
                                              const GJKSolver* nsolver,
                                              bool enable_statistics,
                                              int & num_leaf_tests,
                                              const DistanceRequest& /* request */,
                                              DistanceResult& result)
{
  if(enable_statistics) num_leaf_tests++;
    
  const BVNode<BV>& node = model1->getBV(b1);
  int primitive_id = node.primitiveId();

  const Triangle& tri_id = tri_indices[primitive_id];
  const Vec3f& p1 = vertices[tri_id[0]];
  const Vec3f& p2 = vertices[tri_id[1]];
  const Vec3f& p3 = vertices[tri_id[2]];
    
  FCL_REAL distance;
  Vec3f closest_p1, closest_p2, normal;
  nsolver->shapeTriangleInteraction(model2, tf2, p1, p2, p3, tf1, distance,
                                    closest_p2, closest_p1, normal);

  result.update(distance, model1, &model2, primitive_id, DistanceResult::NONE,
                closest_p1, closest_p2, normal);
}


template<typename BV, typename S>
static inline void distancePreprocessOrientedNode(const BVHModel<BV>* model1,
                                                  Vec3f* vertices, Triangle* tri_indices, int init_tri_id,
                                                  const S& model2, const Transform3f& tf1, const Transform3f& tf2,
                                                  const GJKSolver* nsolver,
                                                  const DistanceRequest& /* request */,
                                                  DistanceResult& result)
{
  const Triangle& init_tri = tri_indices[init_tri_id];
  
  const Vec3f& p1 = vertices[init_tri[0]];
  const Vec3f& p2 = vertices[init_tri[1]];
  const Vec3f& p3 = vertices[init_tri[2]];
  
  FCL_REAL distance;
  Vec3f closest_p1, closest_p2, normal;
  nsolver->shapeTriangleInteraction(model2, tf2, p1, p2, p3, tf1, distance,
                                    closest_p2, closest_p1, normal);

  result.update(distance, model1, &model2, init_tri_id, DistanceResult::NONE,
                closest_p1, closest_p2, normal);
}


}

/// @endcond



/// @brief Traversal node for distance between mesh and shape, when mesh BVH is one of the oriented node (RSS, kIOS, OBBRSS)
template<typename S>
class MeshShapeDistanceTraversalNodeRSS : public MeshShapeDistanceTraversalNode<RSS, S>
{
public:
  MeshShapeDistanceTraversalNodeRSS() : MeshShapeDistanceTraversalNode<RSS, S>()
  {
  }

  void preprocess()
  {
    details::distancePreprocessOrientedNode(this->model1, this->vertices, this->tri_indices, 0, 
                                            *(this->model2), this->tf1, this->tf2, this->nsolver, this->request, *(this->result));
  }

  void postprocess()
  {
  }

  FCL_REAL BVDistanceLowerBound(int b1, int /*b2*/) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    return distance(this->tf1.getRotation(), this->tf1.getTranslation(), this->model2_bv, this->model1->getBV(b1).bv);
  }

  void leafComputeDistance(int b1, int b2) const
  {
    details::meshShapeDistanceOrientedNodeleafComputeDistance(b1, b2, this->model1, *(this->model2), this->vertices, this->tri_indices,
                                                      this->tf1, this->tf2, this->nsolver, this->enable_statistics, this->num_leaf_tests, this->request, *(this->result));
  }
};


template<typename S>
class MeshShapeDistanceTraversalNodekIOS : public MeshShapeDistanceTraversalNode<kIOS, S>
{
public:
  MeshShapeDistanceTraversalNodekIOS() : MeshShapeDistanceTraversalNode<kIOS, S>()
  {
  }

  void preprocess()
  {
    details::distancePreprocessOrientedNode(this->model1, this->vertices, this->tri_indices, 0, 
                                            *(this->model2), this->tf1, this->tf2, this->nsolver, this->request, *(this->result));
  }

  void postprocess()
  {    
  }

  FCL_REAL BVDistanceLowerBound(int b1, int /*b2*/) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    return distance(this->tf1.getRotation(), this->tf1.getTranslation(), this->model2_bv, this->model1->getBV(b1).bv);
  }

  void leafComputeDistance(int b1, int b2) const
  {
    details::meshShapeDistanceOrientedNodeleafComputeDistance(b1, b2, this->model1, *(this->model2), this->vertices, this->tri_indices,
                                                      this->tf1, this->tf2, this->nsolver, this->enable_statistics, this->num_leaf_tests, this->request, *(this->result));
  }

};

template<typename S>
class MeshShapeDistanceTraversalNodeOBBRSS : public MeshShapeDistanceTraversalNode<OBBRSS, S>
{
public:
  MeshShapeDistanceTraversalNodeOBBRSS() : MeshShapeDistanceTraversalNode<OBBRSS, S>()
  {
  }

  void preprocess()
  {
    details::distancePreprocessOrientedNode(this->model1, this->vertices, this->tri_indices, 0, 
                                            *(this->model2), this->tf1, this->tf2, this->nsolver, this->request, *(this->result));
  }

  void postprocess()
  {
    
  }

  FCL_REAL BVDistanceLowerBound(int b1, int /*b2*/) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    return distance(this->tf1.getRotation(), this->tf1.getTranslation(), this->model2_bv, this->model1->getBV(b1).bv);
  }

  void leafComputeDistance(int b1, int b2) const
  {
    details::meshShapeDistanceOrientedNodeleafComputeDistance(b1, b2, this->model1, *(this->model2), this->vertices, this->tri_indices,
                                                      this->tf1, this->tf2, this->nsolver, this->enable_statistics, this->num_leaf_tests, this->request, *(this->result));
  }
  
};

/// @brief Traversal node for distance between shape and mesh
template<typename S, typename BV>
class ShapeMeshDistanceTraversalNode : public ShapeBVHDistanceTraversalNode<S, BV>
{ 
public:
  ShapeMeshDistanceTraversalNode() : ShapeBVHDistanceTraversalNode<S, BV>()
  {
    vertices = NULL;
    tri_indices = NULL;

    rel_err = 0;
    abs_err = 0;

    nsolver = NULL;
  }

  /// @brief Distance testing between leaves (one shape and one triangle)
  void leafComputeDistance(int b1, int b2) const
  {
    if(this->enable_statistics) this->num_leaf_tests++;
    
    const BVNode<BV>& node = this->model2->getBV(b2);
    
    int primitive_id = node.primitiveId();
    
    const Triangle& tri_id = tri_indices[primitive_id];

    const Vec3f& p1 = vertices[tri_id[0]];
    const Vec3f& p2 = vertices[tri_id[1]];
    const Vec3f& p3 = vertices[tri_id[2]];
    
    FCL_REAL distance;
    Vec3f closest_p1, closest_p2, normal;
    nsolver->shapeTriangleInteraction(*(this->model1), this->tf1, p1, p2, p3,
                                      Transform3f (), distance, closest_p1,
                                      closest_p2, normal);

    this->result->update(distance, this->model1, this->model2,
                         DistanceResult::NONE, primitive_id, closest_p1,
                         closest_p2, normal);
  }

  /// @brief Whether the traversal process can stop early
  bool canStop(FCL_REAL c) const
  {
    if((c >= this->result->min_distance - abs_err) && (c * (1 + rel_err) >= this->result->min_distance))
      return true;
    return false;
  }

  Vec3f* vertices;
  Triangle* tri_indices;

  FCL_REAL rel_err;
  FCL_REAL abs_err;
    
  const GJKSolver* nsolver;
};

/// @brief Traversal node for distance between shape and mesh, when mesh BVH is one of the oriented node (RSS, kIOS, OBBRSS)
template<typename S>
class ShapeMeshDistanceTraversalNodeRSS : public ShapeMeshDistanceTraversalNode<S, RSS>
{
public:
  ShapeMeshDistanceTraversalNodeRSS() : ShapeMeshDistanceTraversalNode<S, RSS>()
  {
  }

  void preprocess()
  {
    details::distancePreprocessOrientedNode(this->model2, this->vertices, this->tri_indices, 0,
                                            *(this->model1), this->tf2, this->tf1, this->nsolver, this->request, *(this->result));
  }

  void postprocess()
  {
  }

  FCL_REAL BVDistanceLowerBound(int b1, int b2) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    return distance(this->tf2.getRotation(), this->tf2.getTranslation(), this->model1_bv, this->model2->getBV(b2).bv);
  }

  void leafComputeDistance(int b1, int b2) const
  {
    details::meshShapeDistanceOrientedNodeleafComputeDistance(b2, b1, this->model2, *(this->model1), this->vertices, this->tri_indices,
                                                      this->tf2, this->tf1, this->nsolver, this->enable_statistics, this->num_leaf_tests, this->request, *(this->result));
  }

};

template<typename S>
class ShapeMeshDistanceTraversalNodekIOS : public ShapeMeshDistanceTraversalNode<S, kIOS>
{
public:
  ShapeMeshDistanceTraversalNodekIOS() : ShapeMeshDistanceTraversalNode<S, kIOS>()
  {
  }

  void preprocess()
  {
    details::distancePreprocessOrientedNode(this->model2, this->vertices, this->tri_indices, 0,
                                            *(this->model1), this->tf2, this->tf1, this->nsolver, *(this->result));
  }

  void postprocess()
  {
  }

  FCL_REAL BVDistanceLowerBound(int b1, int b2) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    return distance(this->tf2.getRotation(), this->tf2.getTranslation(), this->model1_bv, this->model2->getBV(b2).bv);
  }

  void leafComputeDistance(int b1, int b2) const
  {
    details::meshShapeDistanceOrientedNodeleafComputeDistance(b2, b1, this->model2, *(this->model1), this->vertices, this->tri_indices,
                                                      this->tf2, this->tf1, this->nsolver, this->enable_statistics, this->num_leaf_tests, this->request, *(this->result));
  }
  
};

template<typename S>
class ShapeMeshDistanceTraversalNodeOBBRSS : public ShapeMeshDistanceTraversalNode<S, OBBRSS>
{
public:
  ShapeMeshDistanceTraversalNodeOBBRSS() : ShapeMeshDistanceTraversalNode<S, OBBRSS>()
  {
  }

  void preprocess()
  {
    details::distancePreprocessOrientedNode(this->model2, this->vertices, this->tri_indices, 0,
                                            *(this->model1), this->tf2, this->tf1, this->nsolver, *(this->result));
  }

  void postprocess()
  {    
  }

  FCL_REAL BVDistanceLowerBound(int b1, int b2) const
  {
    if(this->enable_statistics) this->num_bv_tests++;
    return distance(this->tf2.getRotation(), this->tf2.getTranslation(), this->model1_bv, this->model2->getBV(b2).bv);
  }

  void leafComputeDistance(int b1, int b2) const
  {
    details::meshShapeDistanceOrientedNodeleafComputeDistance(b2, b1, this->model2, *(this->model1), this->vertices, this->tri_indices,
                                                      this->tf2, this->tf1, this->nsolver, this->enable_statistics, this->num_leaf_tests, this->request, *(this->result));
  }
  
};

/// @}

}

} // namespace hpp

/// @endcond

#endif
