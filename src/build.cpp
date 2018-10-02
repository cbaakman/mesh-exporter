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


#include "build.h"


namespace XMLMesh
{
    MeshCorner::MeshCorner(void)
    {
    }
    MeshCorner::~MeshCorner(void)
    {
    }
    MeshVertex::MeshVertex(void)
    {
    }
    MeshVertex::~MeshVertex(void)
    {
    }
    MeshFace::MeshFace(const size_t nCorners)
    {
        countCorners = nCorners;
        mCorners = new MeshCorner[nCorners];

        size_t i, iPrev, iNext;
        for (i = 0; i < nCorners; i++)
        {
            iPrev = (i + nCorners - 1) % nCorners;
            iNext = (i + 1) % nCorners;

            mCorners[i].pPrev = &(mCorners[iPrev]);
            mCorners[i].pNext = &(mCorners[iNext]);
            mCorners[i].pFace = this;
        }
    }
    MeshFace::~MeshFace(void)
    {
        delete[] mCorners;
    }
    MeshTriangleFace::MeshTriangleFace(void): MeshFace(3)
    {
    }
    MeshQuadFace::MeshQuadFace(void): MeshFace(4)
    {
    }
    MeshBone::MeshBone(void): pParent(NULL)
    {
    }
    MeshBone::~MeshBone(void)
    {
    }
    MeshData::MeshData(void)
    {
    }
    MeshData::~MeshData(void)
    {
    }
    MeshState::MeshState(void)
    {
    }
    MeshState::~MeshState(void)
    {
    }

    MeshDataBuilder::MeshDataBuilder(void)
    {
        pMeshData = new MeshData;
    }

    void MeshDataBuilder::AddVertex(const std::string &id, const vec3 &position)
    {
        if (pMeshData->HasVertex(id))
            throw MeshKeyError("duplicate vertex %s", id.c_str());

        MeshVertex *pVertex = new MeshVertex;
        pVertex->id = id;
        pVertex->position = position;

        pMeshData->mVertices.emplace(id, pVertex);
    }
    void MeshDataBuilder::AddQuad(const std::string id, const bool smooth,
                                  const MeshTexCoords *txs, const std::string *vertexIDs)
    {
        if (pMeshData->HasFace(id))
            throw MeshKeyError("duplicate face %s", id.c_str());

        MeshQuadFace *pQuad = new MeshQuadFace;
        pQuad->smooth = smooth;
        pQuad->id = id;

        // The corners have been created and linked together in the constructor.

        size_t i;
        for (i = 0; i < 4; i++)
        {
            if (!HAS_ID(pMeshData->mVertices, vertexIDs[i]))
                throw MeshKeyError("No such vertex %s", vertexIDs[i].c_str());

            pQuad->mCorners[i].pVertex = pMeshData->mVertices.at(vertexIDs[i]);
            pQuad->mCorners[i].pVertex->cornersInvolvedPs.insert(&(pQuad->mCorners[i]));
            pQuad->mCorners[i].texCoords = txs[i];
        }

        pMeshData->mFaces.emplace(id, pQuad);
    }
    void MeshDataBuilder::AddTriangle(const std::string id, const bool smooth,
                                      const MeshTexCoords *txs, const std::string *vertexIDs)
    {
        if (pMeshData->HasFace(id))
            throw MeshKeyError("duplicate face %s", id.c_str());

        MeshTriangleFace *pTriangle = new MeshTriangleFace;
        pTriangle->smooth = smooth;
        pTriangle->id = id;

        // The corners have been created and linked together in the constructor.

        size_t i;
        for (i = 0; i < 3; i++)
        {
            if (!HAS_ID(pMeshData->mVertices, vertexIDs[i]))
                throw MeshKeyError("No such vertex %s", vertexIDs[i].c_str());

            pTriangle->mCorners[i].pVertex = pMeshData->mVertices.at(vertexIDs[i]);
            pTriangle->mCorners[i].pVertex->cornersInvolvedPs.insert(&(pTriangle->mCorners[i]));
            pTriangle->mCorners[i].texCoords = txs[i];
        }

        pMeshData->mFaces.emplace(id, pTriangle);
    }

    void MeshDataBuilder::AddSubset(const std::string &id)
    {
        if (pMeshData->HasSubset(id))
            throw MeshKeyError("duplicate subset %s", id.c_str());

        MeshSubset *pSubset = new MeshSubset;
        pSubset->id = id;

        pMeshData->mSubsets.emplace(id, pSubset);
    }
    void MeshDataBuilder::AddQuadToSubset(const std::string &subsetID, const std::string &quadID)
    {
        if (!HAS_ID(pMeshData->mSubsets, subsetID))
            throw MeshKeyError("No such subset %s", subsetID.c_str());

        if (!HAS_ID(pMeshData->mFaces, quadID))
            throw MeshKeyError("No such quad %s", quadID.c_str());

        MeshSubset *pSubset = pMeshData->mSubsets.at(subsetID);
        MeshFace *pFace = pMeshData->mFaces.at(quadID);
        if (pFace->CountCorners() != 4)
            throw MeshKeyError("%s is not a quad", quadID.c_str());
        pSubset->facePs.insert(pFace);
    }
    void MeshDataBuilder::AddTriangleToSubset(const std::string &subsetID, const std::string &triangleID)
    {
        if (!HAS_ID(pMeshData->mSubsets, subsetID))
            throw MeshKeyError("No such subset %s", subsetID.c_str());

        if (!HAS_ID(pMeshData->mFaces, triangleID))
            throw MeshKeyError("No such triangle %s", triangleID.c_str());

        MeshSubset *pSubset = pMeshData->mSubsets.at(subsetID);
        MeshFace *pFace = pMeshData->mFaces.at(triangleID);
        if (pFace->CountCorners() != 3)
            throw MeshKeyError("%s is not a triangle", triangleID.c_str());
        pSubset->facePs.insert(pFace);
    }
    void MeshDataBuilder::AddBone(const std::string &id, const vec3 &headPosition, const float weight)
    {
        if (pMeshData->HasBone(id))
            throw MeshKeyError("Duplicate bone %1%", id.c_str());

        MeshBone *pBone = new MeshBone;
        pBone->id = id;
        pBone->headPosition = headPosition;
        pBone->weight = weight;

        pMeshData->mBones.emplace(id, pBone);
    }
    void MeshDataBuilder::ConnectBoneToVertex(const std::string &boneID, const std::string &vertexID)
    {
        if (!HAS_ID(pMeshData->mBones, boneID))
            throw MeshKeyError("No such bone %s", boneID.c_str());

        if (!HAS_ID(pMeshData->mVertices, vertexID))
            throw MeshKeyError("No such vertex %s", vertexID.c_str());

        MeshBone *pBone = pMeshData->mBones.at(boneID);
        MeshVertex *pVertex = pMeshData->mVertices.at(vertexID);

        pVertex->bonesPullingPs.insert(pBone);
        pBone->vertexPs.insert(pVertex);
    }
    void MeshDataBuilder::ConnectBones(const std::string &parentID, const std::string &childID)
    {
        if (!HAS_ID(pMeshData->mBones, parentID))
            throw MeshKeyError("No such bone %s", parentID.c_str());

        if (!HAS_ID(pMeshData->mBones, childID))
            throw MeshKeyError("No such bone %s", childID.c_str());

        MeshBone *pParent = pMeshData->mBones.at(parentID),
                 *pChild = pMeshData->mBones.at(childID);

        pChild->pParent = pParent;
    }
    void MeshDataBuilder::AddKey(const std::string &animationID, const std::string &boneID,
                                 const size_t frame, const MeshBoneTransformation &t)
    {
        if(!HAS_ID(pMeshData->mAnimations, animationID))
            throw MeshKeyError("No such animation %s", animationID.c_str());

        MeshSkeletalAnimation *pAnimation = pMeshData->mAnimations.at(animationID);

        if (!HAS_ID(pAnimation->mLayers, boneID))
            AddLayer(animationID, boneID);

        if (HAS_ID(pAnimation->mLayers.at(boneID).mKeys, frame))
            throw MeshKeyError("Duplicate key for animation %s layer %s frame %u",
                               animationID.c_str(), boneID.c_str(), frame);

        pAnimation->mLayers.at(boneID).mKeys[frame].frame = frame;
        pAnimation->mLayers.at(boneID).mKeys[frame].transformation = t;
    }

    void MeshDataBuilder::AddLayer(const std::string &animationID, const std::string &boneID)
    {
        if(!HAS_ID(pMeshData->mAnimations, animationID))
            throw MeshKeyError("No such animation %s", animationID.c_str());

        if(!HAS_ID(pMeshData->mBones, boneID))
            throw MeshKeyError("No such bone %s", boneID.c_str());

        MeshSkeletalAnimation *pAnimation = pMeshData->mAnimations.at(animationID);
        MeshBone *pBone = pMeshData->mBones.at(boneID);

        pAnimation->mLayers[boneID].pBone = pBone;
    }
    void MeshDataBuilder::AddAnimation(const std::string &id, const size_t length)
    {
        if (pMeshData->HasAnimation(id))
            throw MeshKeyError("Duplicate animation %s", id.c_str());

        MeshSkeletalAnimation *pAnimation = new MeshSkeletalAnimation;
        pAnimation->length = length;
        pAnimation->id = id;

        pMeshData->mAnimations.emplace(id, pAnimation);
    }

    MeshData *MeshDataBuilder::GetMeshData(void)
    {
        return pMeshData;
    }

    void DestroyMeshData(MeshData *pMeshData)
    {
        if (pMeshData == NULL)
            return;

        for (const auto &pair : pMeshData->mAnimations)
            delete std::get<1>(pair);

        for (const auto &pair : pMeshData->mBones)
            delete std::get<1>(pair);

        for (const auto &pair : pMeshData->mSubsets)
            delete std::get<1>(pair);

        for (const auto &pair : pMeshData->mFaces)
            delete std::get<1>(pair);

        for (const auto &pair : pMeshData->mVertices)
            delete std::get<1>(pair);

        delete pMeshData;
    }

    MeshStateBuilder::MeshStateBuilder(void)
    {
        pMeshState = new MeshState;
    }

    void MeshStateBuilder::AddVertex(const std::string &id, const vec3 &position)
    {
        if (pMeshState->HasVertex(id))
            throw MeshKeyError("duplicate vertex %s", id.c_str());

        MeshVertex *pVertex = new MeshVertex;
        pVertex->id = id;
        pVertex->position = position;

        pMeshState->mVertices.emplace(id, pVertex);
    }
    void MeshStateBuilder::AddQuad(const std::string id, const bool smooth,
                                   const MeshTexCoords *txs, const std::string *vertexIDs)
    {
        if (pMeshState->HasFace(id))
            throw MeshKeyError("duplicate face %s", id.c_str());

        MeshQuadFace *pQuad = new MeshQuadFace;
        pQuad->smooth = smooth;
        pQuad->id = id;

        // The corners have been created and linked together in the constructor.

        size_t i;
        for (i = 0; i < 4; i++)
        {
            if (!HAS_ID(pMeshState->mVertices, vertexIDs[i]))
                throw MeshKeyError("No such vertex %s", vertexIDs[i].c_str());

            pQuad->mCorners[i].pVertex = pMeshState->mVertices.at(vertexIDs[i]);
            pQuad->mCorners[i].pVertex->cornersInvolvedPs.insert(&(pQuad->mCorners[i]));
            pQuad->mCorners[i].texCoords = txs[i];
        }

        pMeshState->mFaces.emplace(id, pQuad);
    }
    void MeshStateBuilder::AddTriangle(const std::string id, const bool smooth,
                                       const MeshTexCoords *txs, const std::string *vertexIDs)
    {
        if (pMeshState->HasFace(id))
            throw MeshKeyError("duplicate face %s", id.c_str());

        MeshTriangleFace *pTriangle = new MeshTriangleFace;
        pTriangle->smooth = smooth;
        pTriangle->id = id;

        // The corners have been created and linked together in the constructor.

        size_t i;
        for (i = 0; i < 3; i++)
        {
            if (!HAS_ID(pMeshState->mVertices, vertexIDs[i]))
                throw MeshKeyError("No such vertex %s", vertexIDs[i].c_str());

            pTriangle->mCorners[i].pVertex = pMeshState->mVertices.at(vertexIDs[i]);
            pTriangle->mCorners[i].pVertex->cornersInvolvedPs.insert(&(pTriangle->mCorners[i]));
            pTriangle->mCorners[i].texCoords = txs[i];
        }

        pMeshState->mFaces.emplace(id, pTriangle);
    }

    void MeshStateBuilder::AddSubset(const std::string &id)
    {
        if (pMeshState->HasSubset(id))
            throw MeshKeyError("duplicate subset %s", id.c_str());

        MeshSubset *pSubset = new MeshSubset;
        pSubset->id = id;

        pMeshState->mSubsets.emplace(id, pSubset);
    }
    void MeshStateBuilder::AddQuadToSubset(const std::string &subsetID, const std::string &quadID)
    {
        if (!HAS_ID(pMeshState->mSubsets, subsetID))
            throw MeshKeyError("No such subset %s", subsetID.c_str());

        if (!HAS_ID(pMeshState->mFaces, quadID))
            throw MeshKeyError("No such quad %s", quadID.c_str());

        MeshSubset *pSubset = pMeshState->mSubsets.at(subsetID);
        MeshFace *pFace = pMeshState->mFaces.at(quadID);
        if (pFace->CountCorners() != 4)
            throw MeshKeyError("%s is not a quad", quadID.c_str());
        pSubset->facePs.insert(pFace);
    }
    void MeshStateBuilder::AddTriangleToSubset(const std::string &subsetID, const std::string &triangleID)
    {
        if (!HAS_ID(pMeshState->mSubsets, subsetID))
            throw MeshKeyError("No such subset %s", subsetID.c_str());

        if (!HAS_ID(pMeshState->mFaces, triangleID))
            throw MeshKeyError("No such triangle %s", triangleID.c_str());

        MeshSubset *pSubset = pMeshState->mSubsets.at(subsetID);
        MeshFace *pFace = pMeshState->mFaces.at(triangleID);
        if (pFace->CountCorners() != 3)
            throw MeshKeyError("%s is not a triangle", triangleID.c_str());
        pSubset->facePs.insert(pFace);
    }

    MeshState *MeshStateBuilder::GetMeshState(void)
    {
        return pMeshState;
    }

    void DestroyMeshState(MeshState *pMeshState)
    {
        if (pMeshState == NULL)
            return;

        for (const auto &pair : pMeshState->mSubsets)
            delete std::get<1>(pair);

        for (const auto &pair : pMeshState->mFaces)
            delete std::get<1>(pair);

        for (const auto &pair : pMeshState->mVertices)
            delete std::get<1>(pair);

        delete pMeshState;
    }
}
