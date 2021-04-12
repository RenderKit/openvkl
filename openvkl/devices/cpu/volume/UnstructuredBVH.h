// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <vector>
#include "../common/math.h"
#include "embree3/rtcore.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace cpu_device {

    // BVH definitions ////////////////////////////////////////////////////////

    struct Node
    {
      vec3f nominalLength;  // x set to negative for LeafNode;
      range1f valueRange;
      int level;
      Node *parent;

      Node()
      {
        parent = nullptr;
      }
    };

    struct LeafNode : public Node
    {
      box3fa bounds;
      uint64_t cellID;

      LeafNode(unsigned id, const box3fa &bounds, const range1f &range)
          : cellID(id), bounds(bounds)
      {
        nominalLength   = bounds.upper - bounds.lower;
        nominalLength.x = -nominalLength.x;  // leaf
        valueRange    = range;
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          const RTCBuildPrimitive *prims,
                          size_t numPrims,
                          void *userPtr)
      {
        assert(numPrims == 1);

        auto id    = (uint64_t(prims->geomID) << 32) | prims->primID;
        auto range = ((range1f *)userPtr)[id];

        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(LeafNode), 16);
        return (void *)new (ptr) LeafNode(id, *(const box3fa *)prims, range);
      }
    };

    struct InnerNode : public Node
    {
      box3fa bounds[2];
      Node *children[2];

      InnerNode()
      {
        children[0] = children[1] = nullptr;
        bounds[0] = bounds[1] = empty;
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          unsigned int numChildren,
                          void *userPtr)
      {
        assert(numChildren == 2);
        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(InnerNode), 16);
        return (void *)new (ptr) InnerNode;
      }

      static void setChildren(void *nodePtr,
                              void **childPtr,
                              unsigned int numChildren,
                              void *userPtr)
      {
        assert(numChildren == 2);
        auto innerNode = (InnerNode *)nodePtr;
        for (size_t i = 0; i < 2; i++) {
          innerNode->children[i]         = (Node *)childPtr[i];
          innerNode->children[i]->parent = innerNode;
        }
        innerNode->nominalLength.x =
            std::min(fabsf(innerNode->children[0]->nominalLength.x),
                     fabsf(innerNode->children[1]->nominalLength.x));
        innerNode->nominalLength.y =
            std::min(innerNode->children[0]->nominalLength.y,
                     innerNode->children[1]->nominalLength.y);
        innerNode->nominalLength.z =
            std::min(innerNode->children[0]->nominalLength.z,
                     innerNode->children[1]->nominalLength.z);
        innerNode->valueRange = innerNode->children[0]->valueRange;
        innerNode->valueRange.extend(innerNode->children[1]->valueRange);
      }

      static void setBounds(void *nodePtr,
                            const RTCBounds **bounds,
                            unsigned int numChildren,
                            void *userPtr)
      {
        assert(numChildren == 2);
        for (size_t i = 0; i < 2; i++)
          ((InnerNode *)nodePtr)->bounds[i] = *(const box3fa *)bounds[i];
      }
    };

    // Helper functions ///////////////////////////////////////////////////////

    inline bool isLeafNode(const Node *node)
    {
      return (node->nominalLength.x < 0);
    }

    inline void addLevelToNodes(Node *root, int level)
    {
      root->level = level;

      if (root->nominalLength.x > 0) {
        auto inner = (InnerNode *)root;

        addLevelToNodes(inner->children[0], level + 1);
        addLevelToNodes(inner->children[1], level + 1);
      }
    }

    inline void getNodesAtLevel(Node *root,
                                int level,
                                std::vector<Node *> &nodes)
    {
      if (root->level == level) {
        nodes.push_back(root);
      } else if (root->level < level && !isLeafNode(root)) {
        auto inner = (InnerNode *)root;
        getNodesAtLevel(inner->children[0], level, nodes);
        getNodesAtLevel(inner->children[1], level, nodes);
      }
    }

    inline void getLeafNodes(Node *root, std::vector<LeafNode *> &leafNodes)
    {
      if (isLeafNode(root)) {
        leafNodes.push_back((LeafNode *)root);
      } else {
        auto inner = (InnerNode *)root;
        getLeafNodes(inner->children[0], leafNodes);
        getLeafNodes(inner->children[1], leafNodes);
      }
    }

    inline box3f getNodeBounds(const Node *node)
    {
      if (isLeafNode(node)) {
        return box3f(((LeafNode *)node)->bounds);
      } else {
        InnerNode *inner = (InnerNode *)node;
        box3f bounds     = box3f(inner->bounds[0]);
        bounds.extend(box3f(inner->bounds[1]));
        return bounds;
      }
    }

    inline bool nodesOverlap(const Node *node1, const Node *node2)
    {
      const box3f b1 = getNodeBounds(node1);
      const box3f b2 = getNodeBounds(node2);

      return !(b1.lower.x >= b2.upper.x || b2.lower.x >= b1.upper.x ||
               b1.lower.y >= b2.upper.y || b2.lower.y >= b1.upper.y ||
               b1.lower.z >= b2.upper.z || b2.lower.z >= b1.upper.z);
    }

    inline std::vector<Node *> getOverlappingNodesAtSameLevel(Node *root,
                                                              Node *checkNode)
    {
      Node *node = root;
      Node *nodeStack[32];
      int stackPtr = 0;

      std::vector<Node *> overlappingNodes;

      while (true) {
        if (node->level == checkNode->level) {
          if (node != checkNode && nodesOverlap(checkNode, node)) {
            overlappingNodes.push_back(node);
          }
        } else if (node->level < checkNode->level && !isLeafNode(node)) {
          InnerNode *inner    = (InnerNode *)node;
          const bool overlap0 = nodesOverlap(checkNode, inner->children[0]);
          const bool overlap1 = nodesOverlap(checkNode, inner->children[1]);

          if (overlap0) {
            if (overlap1) {
              nodeStack[stackPtr++] = inner->children[1];
              node                  = inner->children[0];
              continue;
            } else {
              node = inner->children[0];
              continue;
            }
          } else {
            if (overlap1) {
              node = inner->children[1];
              continue;
            } else {
              // do nothing, just pop
            }
          }
        }

        if (stackPtr == 0)
          break;

        node = nodeStack[--stackPtr];
      }

      return overlappingNodes;
    }

    // accumulates node metadata (value range, etc) for overlapping nodes at the
    // same level of the tree, across all levels of the tree. this allows BVH
    // node intersections to be used individually in interval / hit iteration.
    inline void computeOverlappingNodeMetadata(Node *root)
    {
      int level = 0;

      while (true) {
        std::vector<Node *> nodesAtLevel;
        getNodesAtLevel(root, level, nodesAtLevel);

        if (!nodesAtLevel.size()) {
          break;
        }

        struct AccumulatedMetadata
        {
          vec3f nominalLength;
          range1f valueRange;
        };

        // must accumulate overlapping metadata and then apply separately, to
        // avoid effects of transitive overlaps
        std::vector<AccumulatedMetadata> accum(nodesAtLevel.size());

        rkcommon::tasking::parallel_for(
            nodesAtLevel.size(), [&](size_t nodeIndex) {
              Node *node = nodesAtLevel[nodeIndex];

              std::vector<Node *> overlappingNodes =
                  getOverlappingNodesAtSameLevel(root, node);

              accum[nodeIndex] =
                  AccumulatedMetadata{node->nominalLength, node->valueRange};

              for (const Node *on : overlappingNodes) {
                if (isLeafNode(node)) {
                  // for leaves, nominalLength is stored as negative (but used
                  // as an absolute value)
                  accum[nodeIndex].nominalLength.x =
                      std::max(accum[nodeIndex].nominalLength.x,
                               -fabsf(on->nominalLength.x));
                } else {
                  accum[nodeIndex].nominalLength.x =
                      std::min(accum[nodeIndex].nominalLength.x,
                               fabsf(on->nominalLength.x));
                }
                accum[nodeIndex].nominalLength.y = std::min(
                    accum[nodeIndex].nominalLength.y, on->nominalLength.y);
                accum[nodeIndex].nominalLength.z = std::min(
                    accum[nodeIndex].nominalLength.z, on->nominalLength.z);

                accum[nodeIndex].valueRange.extend(on->valueRange);
              }
            });

        // apply new metadata to nodes
        for (size_t i = 0; i < nodesAtLevel.size(); i++) {
          Node *node = nodesAtLevel[i];

          node->nominalLength = accum[i].nominalLength;
          node->valueRange    = accum[i].valueRange;
        }

        level++;
      }
    }

  }  // namespace cpu_device
}  // namespace openvkl
