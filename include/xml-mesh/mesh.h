#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <list>
#include <map>

#include <linear-gl/vec.h>
#include <linear-gl/quat.h>


using namespace LinearGL;

#define HAS_ID(map, id) (map.find(id) != map.end())

namespace XMLMesh
{
    /**
     * Thrown when a key is missing in the MeshData or MeshState object.
     */
    class MeshKeyError;

    /**
     * Thrown when parsing from xml fails.
     */
    class MeshParseError;


    struct MeshVertex;
    struct MeshFace;
    struct MeshBone;

    typedef vec2 MeshTexCoords;

    struct MeshCorner
    {
        MeshTexCoords texCoords;
        MeshVertex *pVertex;
        MeshFace *pFace;  // either quad or triangle

        // Used for calculating normal, tangent, bitangent.
        MeshCorner *pPrev, *pNext;
    };

    struct MeshVertex
    {
        std::string id;

        vec3 position;  // in mesh space

        std::list<MeshCorner *> cornersInvolvedPs;
        std::list<MeshBone *> bonesPullingPs;
    };

    class MeshCornerIterable
    {
    private:
        MeshCorner *array;
        size_t length;
    public:
        MeshCornerIterable(MeshCorner *array, const size_t length);

        MeshCorner *begin(void) const;
        MeshCorner *end(void) const;
    };

    class ConstMeshCornerIterable
    {
    private:
        const MeshCorner *array;
        size_t length;
    public:
        ConstMeshCornerIterable(const MeshCorner *array, const size_t length);

        const MeshCorner *begin(void) const;
        const MeshCorner *end(void) const;
    };

    struct MeshFace
    {
        std::string id;
        bool smooth;
        MeshCorner *mCorners;
        size_t countCorners;

        MeshFace(const size_t numberOfCorners);
        MeshFace(const MeshFace &);
        ~MeshFace(void);

        /*
         * The following two functions iterate in the same order as blender:
         * counter-clockwise.
         */
        MeshCornerIterable IterCorners(void);
        ConstMeshCornerIterable IterCorners(void) const;
    };

    struct MeshTriangleFace: public MeshFace
    {
        MeshTriangleFace(void);
    };

    struct MeshQuadFace: public MeshFace
    {
        MeshQuadFace(void);
    };


    struct MeshSubset
    {
        std::string id;

        std::list<MeshFace *> facePs;
    };


    struct MeshBone
    {
        std::string id;

        MeshBone *pParent;  // can be null

        vec3 headPosition;  // in mesh space
        float weight;

        std::list<MeshVertex *> vertexPs;
    };


    struct MeshBoneTransformation
    {
        quaternion rotation;
        vec3 translation;
    };

    extern const MeshBoneTransformation MESHBONETRANSFORM_ID;

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

        std::map<size_t, MeshBoneKey> mKeys;
    };


    struct MeshSkeletalAnimation
    {
        std::string id;

        size_t length;

        std::map<std::string, MeshBoneLayer> mLayers;
    };




    /**
     *  The user of this library shouldn't modify mesh data
     *  in source files. Instead, parse an XML file.
     *
     *  In principle, one can render meshes using a MeshData object only.
     *  For using the animations however, you'll need to derive a MeshState object.
     */
    struct MeshData
    {
        std::map<std::string, MeshVertex> mVertices;
        std::map<std::string, MeshQuadFace> mQuads;
        std::map<std::string, MeshTriangleFace> mTriangles;
        std::map<std::string, MeshSubset> mSubsets;
        std::map<std::string, MeshBone> mBones;
        std::map<std::string, MeshSkeletalAnimation> mAnimations;
    };

    void ParseMesh(std::istream &, MeshData &);


    /**
     * It's possible to apply transformations to this,
     * using the MeshData object as rest position.
     */
    struct MeshState
    {
        std::map<std::string, MeshVertex> mVertices;
        std::map<std::string, MeshQuadFace> mQuads;
        std::map<std::string, MeshTriangleFace> mTriangles;
        std::map<std::string, MeshSubset> mSubsets;
    };

    void DeriveMeshState(const MeshData &, MeshState &);


    typedef unsigned long long milliseconds;

    void GetBoneTransformationsAt(const MeshData &, const std::string &animationID,
                                  const milliseconds msSinceStart, const float framesPerSecond, const bool loop,
                                  std::map<std::string, MeshBoneTransformation> &);

    // The user might sometimes want to transform individual bones.

    /**
     * Must use a MeshState object, derived from the given MeshData object.
     */
    void ApplyBoneTransformations(const MeshData &,
                                  const std::map<std::string, MeshBoneTransformation> &,
                                  MeshState &);


    /**
     *  for solid shading
     */
    vec3 CalculateFaceNormal(const MeshFace &);
    std::tuple<vec3, vec3> CalculateFaceTangentBitangent(const MeshFace &);

    /**
     *  for smooth shading
     */
    vec3 CalculateVertexNormal(const MeshVertex &);
    std::tuple<vec3, vec3> CalculateVertexTangentBiTangent(const MeshVertex &);

    /*
     * One can flip the normals by taking their negatives. Otherwise,
     * they are just like in blender.
     */
}

#endif  // MESH_H
