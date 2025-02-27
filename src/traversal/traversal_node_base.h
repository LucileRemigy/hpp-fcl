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

#ifndef HPP_FCL_TRAVERSAL_NODE_BASE_H
#define HPP_FCL_TRAVERSAL_NODE_BASE_H

/// @cond INTERNAL

#include <hpp/fcl/data_types.h>
#include <hpp/fcl/math/transform.h>
#include <hpp/fcl/collision_data.h>

namespace hpp
{
namespace fcl
{

/// @brief Node structure encoding the information required for traversal.

class TraversalNodeBase
{
public:
  virtual ~TraversalNodeBase();

  virtual void preprocess() {}
  
  virtual void postprocess() {}

  /// @brief Whether b is a leaf node in the first BVH tree 
  virtual bool isFirstNodeLeaf(int b) const;

  /// @brief Whether b is a leaf node in the second BVH tree
  virtual bool isSecondNodeLeaf(int b) const;

  /// @brief Traverse the subtree of the node in the first tree first
  virtual bool firstOverSecond(int b1, int b2) const;

  /// @brief Get the left child of the node b in the first tree
  virtual int getFirstLeftChild(int b) const;

  /// @brief Get the right child of the node b in the first tree
  virtual int getFirstRightChild(int b) const;

  /// @brief Get the left child of the node b in the second tree
  virtual int getSecondLeftChild(int b) const;

  /// @brief Get the right child of the node b in the second tree
  virtual int getSecondRightChild(int b) const;

  /// @brief Enable statistics (verbose mode)
  virtual void enableStatistics(bool enable) = 0;

  /// @brief configuation of first object
  Transform3f tf1;

  /// @brief configuration of second object
  Transform3f tf2;
};

/// @defgroup Traversal_For_Collision
/// regroup class about traversal for distance.
/// @{

/// @brief Node structure encoding the information required for collision traversal.
class CollisionTraversalNodeBase : public TraversalNodeBase
{
public:
  CollisionTraversalNodeBase (const CollisionRequest& request_) :
  request (request_), result(NULL), enable_statistics(false) {}

  virtual ~CollisionTraversalNodeBase();

  /// @brief BV test between b1 and b2
  virtual bool BVDisjoints(int b1, int b2) const = 0;

  /// BV test between b1 and b2
  /// @param b1, b2 Bounding volumes to test,
  /// @retval sqrDistLowerBound square of a lower bound of the minimal
  ///         distance between bounding volumes.
  virtual bool BVDisjoints(int b1, int b2, FCL_REAL& sqrDistLowerBound) const = 0;

  /// @brief Leaf test between node b1 and b2, if they are both leafs
  virtual void leafCollides(int /*b1*/, int /*b2*/, FCL_REAL& /*sqrDistLowerBound*/) const
  {
    throw std::runtime_error ("Not implemented");
  }

  /// @brief Check whether the traversal can stop
  virtual bool canStop() const;

  /// @brief Whether store some statistics information during traversal
  void enableStatistics(bool enable) { enable_statistics = enable; }

  /// @brief request setting for collision
  const CollisionRequest& request;

  /// @brief collision result kept during the traversal iteration
  CollisionResult* result;

  /// @brief Whether stores statistics 
  bool enable_statistics;
};

/// @}

/// @defgroup Traversal_For_Distance
/// regroup class about traversal for distance.
/// @{

/// @brief Node structure encoding the information required for distance traversal.
class DistanceTraversalNodeBase : public TraversalNodeBase
{
public:
  DistanceTraversalNodeBase() : result(NULL), enable_statistics(false) {}

  virtual ~DistanceTraversalNodeBase();

  /// @brief BV test between b1 and b2
  /// @return a lower bound of the distance between the two BV.
  /// @note except for OBB, this method returns the distance.
  virtual FCL_REAL BVDistanceLowerBound(int b1, int b2) const;

  /// @brief Leaf test between node b1 and b2, if they are both leafs
  virtual void leafComputeDistance(int b1, int b2) const = 0;

  /// @brief Check whether the traversal can stop
  virtual bool canStop(FCL_REAL c) const;

  /// @brief Whether store some statistics information during traversal
  void enableStatistics(bool enable) { enable_statistics = enable; }

  /// @brief request setting for distance
  DistanceRequest request;

  /// @brief distance result kept during the traversal iteration
  DistanceResult* result;

  /// @brief Whether stores statistics 
  bool enable_statistics;
};

///@}

}

} // namespace hpp

/// @endcond

#endif