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

#include "mesh.h"
#include "iter.h"


namespace XMLMesh
{
    MeshTexCoords MeshCorner::GetTexCoords(void) const
    {
        return texCoords;
    }

    const MeshVertex *MeshCorner::GetVertex(void) const
    {
        return pVertex;
    }

    const MeshFace *MeshCorner::GetFace(void) const
    {
        return pFace;
    }

    const MeshCorner *MeshCorner::GetPrev() const
    {
        return pPrev;
    }

    const MeshCorner *MeshCorner::GetNext() const
    {
        return pNext;
    }

    const char *MeshVertex::GetID(void) const
    {
        return id.c_str();
    }

    vec3 MeshVertex::GetPosition(void) const
    {
        return position;
    }
    void MeshVertex::SetPosition(const vec3 &pos)
    {
        position = pos;
    }

    ConstSetIterable<MeshCorner> MeshVertex::IterCorners(void) const
    {
        return ConstSetIterable<MeshCorner>(cornersInvolvedPs);
    }

    ConstSetIterable<MeshBone> MeshVertex::IterBones(void) const
    {
        return ConstSetIterable<MeshBone>(bonesPullingPs);
    }

    ConstArrayIterable<MeshCorner> MeshFace::IterCorners(void) const
    {
        return ConstArrayIterable<MeshCorner>(mCorners, countCorners);
    }

    const char *MeshFace::GetID(void) const
    {
        return id.c_str();
    }

    bool MeshFace::IsSmooth(void) const
    {
        return smooth;
    }

    const MeshCorner *MeshFace::GetCorners(void) const
    {
        return mCorners;
    }

    size_t MeshFace::CountCorners(void) const
    {
        return countCorners;
    }

    const char *MeshBone::GetID(void) const
    {
        return id.c_str();
    }

    bool MeshBone::HasParent(void) const
    {
        return pParent != NULL;
    }

    const MeshBone *MeshBone::GetParent(void) const
    {
        return pParent;
    }

    vec3 MeshBone::GetHeadPosition(void) const
    {
        return headPosition;
    }

    float MeshBone::GetWeight(void) const
    {
        return weight;
    }

    ConstSetIterable<MeshVertex> MeshBone::IterVertices(void) const
    {
        return ConstSetIterable<MeshVertex>(vertexPs);
    }

    bool MeshData::HasVertex(const std::string &id) const
    {
        return HAS_ID(mVertices, id);
    }
    const MeshVertex *MeshData::GetVertex(const std::string &id) const
    {
        if (!HAS_ID(mVertices, id))
            throw MeshKeyError("No such vertex: %s", id.c_str());

        return mVertices.at(id);
    }
    ConstMapValueIterable<MeshVertex> MeshData::IterVertices(void) const
    {
        return ConstMapValueIterable<MeshVertex>(mVertices);
    }

    bool MeshState::HasVertex(const std::string &id) const
    {
        return HAS_ID(mVertices, id);
    }
    MeshVertex *MeshState::GetVertex(const std::string &id)
    {
        if (!HAS_ID(mVertices, id))
            throw MeshKeyError("No such vertex: %s", id.c_str());

        return mVertices.at(id);
    }
    const MeshVertex *MeshState::GetVertex(const std::string &id) const
    {
        if (!HAS_ID(mVertices, id))
            throw MeshKeyError("No such vertex: %s", id.c_str());

        return mVertices.at(id);
    }
    ConstMapValueIterable<MeshVertex> MeshState::IterVertices(void) const
    {
        return ConstMapValueIterable<MeshVertex>(mVertices);
    }
    MapValueIterable<MeshVertex> MeshState::IterVertices(void)
    {
        return MapValueIterable<MeshVertex>(mVertices);
    }

    bool MeshData::HasFace(const std::string &id) const
    {
        return HAS_ID(mFaces, id);
    }
    const MeshFace *MeshData::GetFace(const std::string &id) const
    {
        if (!HAS_ID(mFaces, id))
            throw MeshKeyError("No such face: %s", id.c_str());

        return mFaces.at(id);
    }
    ConstMapValueIterable<MeshFace> MeshData::IterFaces(void) const
    {
        return ConstMapValueIterable<MeshFace>(mFaces);
    }

    bool MeshState::HasFace(const std::string &id) const
    {
        return HAS_ID(mFaces, id);
    }
    const MeshFace *MeshState::GetFace(const std::string &id) const
    {
        if (!HAS_ID(mFaces, id))
            throw MeshKeyError("No such face: %s", id.c_str());

        return mFaces.at(id);
    }
    ConstMapValueIterable<MeshFace> MeshState::IterFaces(void) const
    {
        return ConstMapValueIterable<MeshFace>(mFaces);
    }

    bool MeshData::HasSubset(const std::string &id) const
    {
        return HAS_ID(mSubsets, id);
    }
    const MeshSubset *MeshData::GetSubset(const std::string &id) const
    {
        if (!HAS_ID(mSubsets, id))
            throw MeshKeyError("No such subset: %s", id.c_str());

        return mSubsets.at(id);
    }
    ConstMapValueIterable<MeshSubset> MeshData::IterSubsets(void) const
    {
        return ConstMapValueIterable<MeshSubset>(mSubsets);
    }

    bool MeshState::HasSubset(const std::string &id) const
    {
        return HAS_ID(mSubsets, id);
    }
    const MeshSubset *MeshState::GetSubset(const std::string &id) const
    {
        if (!HAS_ID(mSubsets, id))
            throw MeshKeyError("No such subset: %s", id.c_str());

        return mSubsets.at(id);
    }
    ConstMapValueIterable<MeshSubset> MeshState::IterSubsets(void) const
    {
        return ConstMapValueIterable<MeshSubset>(mSubsets);
    }

    bool MeshData::HasBone(const std::string &id) const
    {
        return HAS_ID(mBones, id);
    }
    const MeshBone *MeshData::GetBone(const std::string &id) const
    {
        if (!HAS_ID(mBones, id))
            throw MeshKeyError("No such bone: %s", id.c_str());

        return mBones.at(id);
    }
    ConstMapValueIterable<MeshBone> MeshData::IterBones(void) const
    {
        return ConstMapValueIterable<MeshBone>(mBones);
    }

    bool MeshData::HasAnimation(const std::string &id) const
    {
        return HAS_ID(mAnimations, id);
    }
    const MeshSkeletalAnimation *MeshData::GetAnimation(const std::string &id) const
    {
        if (!HAS_ID(mAnimations, id))
            throw MeshKeyError("No such animation: %s", id.c_str());

        return mAnimations.at(id);
    }

    std::tuple<size_t, size_t> MeshData::CountQuadsTriangles(void) const
    {
        size_t nQuads = 0, nTriangles = 0;
        for (const MeshFace *pFace : IterFaces())
            if (pFace->CountCorners() == 4)
                nQuads++;
            else if(pFace->CountCorners() == 3)
                nTriangles++;
        return std::make_tuple(nQuads, nTriangles);
    }
    std::tuple<size_t, size_t> MeshState::CountQuadsTriangles(void) const
    {
        size_t nQuads = 0, nTriangles = 0;
        for (const MeshFace *pFace : IterFaces())
            if (pFace->CountCorners() == 4)
                nQuads++;
            else if(pFace->CountCorners() == 3)
                nTriangles++;
        return std::make_tuple(nQuads, nTriangles);
    }
    std::tuple<size_t, size_t> MeshSubset::CountQuadsTriangles(void) const
    {
        size_t nQuads = 0, nTriangles = 0;
        for (const MeshFace *pFace : facePs)
            if (pFace->CountCorners() == 4)
                nQuads++;
            else if(pFace->CountCorners() == 3)
                nTriangles++;
        return std::make_tuple(nQuads, nTriangles);
    }
    const char *MeshSubset::GetID(void) const
    {
        return id.c_str();
    }
    ConstSetIterable<MeshFace> MeshSubset::IterFaces(void) const
    {
        return ConstSetIterable<MeshFace>(facePs);
    }

}
