/* Copyright (C) 2018 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <set>
#include <unordered_map>
#include <exception>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "iter.h"


using namespace glm;

#define HAS_ID(map, id) (map.find(id) != map.end())

#define ERRORBUF_SIZE 1024

namespace XMLMesh
{
    class MeshError: public std::exception
    {
        protected:
            char buffer[ERRORBUF_SIZE];
        public:
            const char *what(void) const noexcept;
    };

    /**
     * Thrown when a key is missing in the MeshData or MeshState object.
     */
    class MeshKeyError: public MeshError
    {
        public:
            MeshKeyError(const char *format, ...);
    };


    /**
     * Thrown when parsing from xml fails.
     */
    class MeshParseError: public MeshError
    {
        public:
            MeshParseError(const char *format, ...);
    };


    class MeshData;
    class MeshState;
    class MeshVertex;
    class MeshFace;
    class MeshBone;

    typedef vec2 MeshTexCoords;

    class MeshCorner
    {
        private:
            MeshTexCoords texCoords;
            MeshVertex *pVertex;
            MeshFace *pFace;  // either quad or triangle

            // Used for calculating normal, tangent, bitangent.
            MeshCorner *pPrev, *pNext;

            MeshCorner(void);
            ~MeshCorner(void);

            MeshCorner(const MeshCorner &) = delete;
            void operator=(const MeshCorner &) = delete;
        public:
            MeshTexCoords GetTexCoords(void) const;
            const MeshVertex *GetVertex(void) const;
            const MeshFace *GetFace(void) const;
            const MeshCorner *GetPrev() const;
            const MeshCorner *GetNext() const;

        friend class MeshFace;
        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
        friend void DestroyMeshData(MeshData *);
        friend void DestroyMeshState(MeshState *);
    };


    class MeshVertex
    {
        private:
            std::string id;

            vec3 position;  // in mesh space

            std::set<MeshCorner *> cornersInvolvedPs;
            std::set<MeshBone *> bonesPullingPs;

            MeshVertex(void);
            ~MeshVertex(void);

            MeshVertex(const MeshVertex &) = delete;
            void operator=(const MeshVertex &) = delete;
        public:
            const char *GetID(void) const;
            vec3 GetPosition(void) const;
            void SetPosition(const vec3 &);
            ConstSetIterable<MeshCorner> IterCorners(void) const;
            ConstSetIterable<MeshBone> IterBones(void) const;

        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
        friend void DestroyMeshData(MeshData *);
        friend void DestroyMeshState(MeshState *);
    };


    class MeshFace
    {
        private:
            std::string id;
            bool smooth;
            MeshCorner *mCorners;
            size_t countCorners;
        protected:
            MeshFace(const size_t numberOfCorners);
            ~MeshFace(void);

            MeshFace(const MeshFace &) = delete;
            void operator=(const MeshFace &) = delete;
        public:
            /*
             * This function iterates in the same order as blender.
             */
            ConstArrayIterable<MeshCorner> IterCorners(void) const;

            const char *GetID(void) const;
            bool IsSmooth(void) const;
            const MeshCorner *GetCorners(void) const;
            size_t CountCorners(void) const;

        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
        friend void DestroyMeshData(MeshData *);
        friend void DestroyMeshState(MeshState *);
    };

    class MeshTriangleFace: public MeshFace
    {
        private:
            MeshTriangleFace(void);

            MeshTriangleFace(const MeshTriangleFace &) = delete;
            void operator=(const MeshTriangleFace &) = delete;

        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
        friend void DestroyMeshData(MeshData *);
        friend void DestroyMeshState(MeshState *);
    };

    class MeshQuadFace: public MeshFace
    {
        private:
            MeshQuadFace(void);

            MeshQuadFace(const MeshQuadFace &) = delete;
            void operator=(const MeshQuadFace &) = delete;

        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
        friend void DestroyMeshData(MeshData *);
        friend void DestroyMeshState(MeshState *);
    };


    class MeshSubset
    {
        private:
            std::string id;
            std::set<MeshFace *> facePs;
        public:
            const char *GetID(void) const;
            ConstSetIterable<MeshFace> IterFaces(void) const;
            std::tuple<size_t, size_t> CountQuadsTriangles(void) const;

        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
    };


    class MeshBone
    {
        private:
            std::string id;

            MeshBone *pParent;  // can be null

            vec3 headPosition;  // in mesh space
            float weight;

            std::set<MeshVertex *> vertexPs;

            MeshBone(void);
            ~MeshBone(void);

            MeshBone(const MeshBone &) = delete;
            void operator=(const MeshBone &) = delete;
        public:
            const char *GetID(void) const;
            bool HasParent(void) const;
            const MeshBone *GetParent(void) const;
            vec3 GetHeadPosition(void) const;
            float GetWeight(void) const;
            ConstSetIterable<MeshVertex> IterVertices(void) const;

        friend class MeshDataBuilder;
        friend class MeshStateBuilder;
        friend void DestroyMeshData(MeshData *);
        friend void DestroyMeshState(MeshState *);
    };


    struct MeshBoneTransformation
    {
        quat rotation;
        vec3 translation;
    };

    extern const MeshBoneTransformation MESHBONETRANSFORM_ID;  // puts bone in rest position

    MeshBoneTransformation Interpolate(const MeshBoneTransformation &t0,
                                       const MeshBoneTransformation &t1,
                                       const float s);
                                       // precondition: s ranges from 0.0f to 1.0f

    struct MeshBoneKey
    {
        size_t frame;

        MeshBoneTransformation transformation;
    };


    struct MeshBoneLayer
    {
        MeshBone *pBone;

        std::unordered_map<size_t, MeshBoneKey> mKeys;
    };


    /**
     *  It is safe to modify an animation, because no other
     *  objects refer to it.
     */
    struct MeshSkeletalAnimation
    {
        std::string id;

        size_t length;

        std::unordered_map<std::string, MeshBoneLayer> mLayers;
    };



    /**
     *  In principle, one can render meshes using a MeshData object only.
     *  For using the animations however, you'll need to derive a MeshState object.
     */
    class MeshData
    {
        private:
            std::unordered_map<std::string, MeshVertex *> mVertices;
            std::unordered_map<std::string, MeshFace *> mFaces;
            std::unordered_map<std::string, MeshSubset *> mSubsets;
            std::unordered_map<std::string, MeshBone *> mBones;
            std::unordered_map<std::string, MeshSkeletalAnimation *> mAnimations;

            MeshData(void);
            ~MeshData(void);

            MeshData(const MeshData &) = delete;
            void operator=(const MeshData &) = delete;
        public:
            bool HasVertex(const std::string &id) const;
            const MeshVertex *GetVertex(const std::string &id) const;
            ConstMapValueIterable<MeshVertex> IterVertices(void) const;

            bool HasFace(const std::string &id) const;
            const MeshFace *GetFace(const std::string &id) const;
            ConstMapValueIterable<MeshFace> IterFaces(void) const;

            bool HasSubset(const std::string &id) const;
            const MeshSubset *GetSubset(const std::string &id) const;
            ConstMapValueIterable<MeshSubset> IterSubsets(void) const;

            bool HasBone(const std::string &id) const;
            const MeshBone *GetBone(const std::string &id) const;
            ConstMapValueIterable<MeshBone> IterBones(void) const;

            bool HasAnimation(const std::string &id) const;
            const MeshSkeletalAnimation *GetAnimation(const std::string &id) const;

            std::tuple<size_t, size_t> CountQuadsTriangles(void) const;

        friend class MeshDataBuilder;
        friend void DestroyMeshData(MeshData *);
    };

    MeshData *ParseMeshData(std::istream &);
    void DestroyMeshData(MeshData *);


    /**
     * It's possible to apply transformations to this,
     * using the MeshData object as rest position.
     */
    class MeshState
    {
        private:
            std::unordered_map<std::string, MeshVertex *> mVertices;
            std::unordered_map<std::string, MeshFace *> mFaces;
            std::unordered_map<std::string, MeshSubset *> mSubsets;

            MeshState(void);
            MeshState(const MeshState &);
            ~MeshState(void);
        public:
            bool HasVertex(const std::string &id) const;
            MeshVertex *GetVertex(const std::string &id);
            const MeshVertex *GetVertex(const std::string &id) const;
            ConstMapValueIterable<MeshVertex> IterVertices(void) const;
            MapValueIterable<MeshVertex> IterVertices(void);

            bool HasFace(const std::string &id) const;
            const MeshFace *GetFace(const std::string &id) const;
            ConstMapValueIterable<MeshFace> IterFaces(void) const;

            bool HasSubset(const std::string &id) const;
            const MeshSubset *GetSubset(const std::string &id) const;
            ConstMapValueIterable<MeshSubset> IterSubsets(void) const;

            std::tuple<size_t, size_t> CountQuadsTriangles(void) const;

        friend class MeshStateBuilder;
        friend void DestroyMeshState(MeshState *);
        friend void ApplyBoneTransformations(const MeshData *,
                                             const std::unordered_map<std::string, MeshBoneTransformation> &,
                                             MeshState *);
    };

    MeshState *DeriveMeshState(const MeshData *);
    void DestroyMeshState(MeshState *);


    typedef unsigned long long milliseconds;

    void GetBoneTransformationsAt(const MeshData *, const std::string &animationID,
                                  const milliseconds msSinceStart, const float framesPerSecond, const bool loop,
                                  std::unordered_map<std::string, MeshBoneTransformation> &);

    // The user might sometimes want to transform individual bones.

    /**
     * Must use a MeshState object, derived from the given MeshData object.
     */
    void ApplyBoneTransformations(const MeshData *,
                                  const std::unordered_map<std::string, MeshBoneTransformation> &,
                                  MeshState *);


    /**
     *  for solid shading
     */
    vec3 CalculateFaceNormal(const MeshFace *);
    std::tuple<vec3, vec3> CalculateFaceTangentBitangent(const MeshFace *);

    /**
     *  for smooth shading
     */
    vec3 CalculateVertexNormal(const MeshVertex *);
    std::tuple<vec3, vec3> CalculateVertexTangentBiTangent(const MeshVertex *);

    /*
     * One can flip the normals by taking their negatives. Otherwise,
     * they are just like in blender.
     */
}

#endif  // MESH_H
