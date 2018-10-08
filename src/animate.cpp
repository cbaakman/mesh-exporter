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

#include <exception>
#include <map>
#include <algorithm>
#include <climits>

#include "mesh.h"
#include "build.h"


namespace XMLMesh
{
    MeshState *DeriveMeshState(const MeshData *pMeshData)
    {
        MeshStateBuilder builder;

        // First copy the vertices.
        for (const MeshVertex *pVertex : pMeshData->IterVertices())
        {
            builder.AddVertex(pVertex->GetID(), pVertex->GetPosition());
        }

        MeshTexCoords txs[4];
        std::string vertexIDs[4];
        size_t i;

        // Next copy the faces, that connect the vertices.
        for (const MeshFace *pFace : pMeshData->IterFaces())
        {
            if (pFace->CountCorners() > 4 || pFace->CountCorners() < 3)
                throw MeshKeyError("face %s has %u corners", pFace->GetID(), pFace->CountCorners());

            for (i = 0; i < pFace->CountCorners(); i++)
            {
                txs[i] = pFace->GetCorners()[i].GetTexCoords();
                vertexIDs[i] = pFace->GetCorners()[i].GetVertex()->GetID();
            }

            if (pFace->CountCorners() == 4)
                builder.AddQuad(pFace->GetID(), pFace->IsSmooth(), txs, vertexIDs);
            else if (pFace->CountCorners() == 3)
                builder.AddTriangle(pFace->GetID(), pFace->IsSmooth(), txs, vertexIDs);
        }

        // Finally copy the subset, holding the faces.
        for (const MeshSubset *pSubset : pMeshData->IterSubsets())
        {
            builder.AddSubset(pSubset->GetID());
            for (const MeshFace *pFace : pSubset->IterFaces())
            {
                if (pFace->CountCorners() == 4)
                    builder.AddQuadToSubset(pSubset->GetID(), pFace->GetID());
                else if (pFace->CountCorners() == 3)
                    builder.AddTriangleToSubset(pSubset->GetID(), pFace->GetID());
            }
        }

        return builder.GetMeshState();
    }

    MeshBoneTransformation Interpolate(const MeshBoneTransformation &t0,
                                       const MeshBoneTransformation &t1,
                                       const float s)
    {
        MeshBoneTransformation r;

        r.rotation = slerp(t0.rotation, t1.rotation, s);
        r.translation = (1.0f - s) * t0.translation + s * t1.translation;

        return r;
    }

    float ModulateFrame(const milliseconds ms, const float framesPerSecond, const size_t loopFrames)
    {
        float msPeriod = float(1000 * loopFrames) / framesPerSecond,
              msPerFrame = 1000.0f / framesPerSecond;
        milliseconds msRemain = ms % milliseconds(msPeriod);

        return ((float)msRemain) / msPerFrame;
    }

    float ClampFrame(const milliseconds ms, const float framesPerSecond, const size_t totalFrames)
    {
        float msMax = float(1000 * totalFrames) / framesPerSecond,
              msPerFrame = 1000.0f / framesPerSecond;

        return std::min((float)ms, msMax) / msPerFrame;
    }


    /**
     *  Precondition is that 'frame' is between 0 and 'animationLength'.
     */
    void PickKeyFrames(const MeshBoneLayer *pLayer, const float frame, const size_t animationLength,
                       const bool loop,
                       int &framePrev, int &frameNext,
                       float &distanceToPrev, float &distanceToNext)
    {
        int keyFrame, frameFirst, frameLast;

        // Determine first and last key frame.
        frameFirst = INT_MAX;
        frameLast = INT_MIN;
        for (const auto &frameKeyPair : pLayer->mKeys)
        {
            keyFrame = std::get<0>(frameKeyPair);

            if (keyFrame < frameFirst)
                frameFirst = keyFrame;

            if (keyFrame > frameLast)
                frameLast = keyFrame;
        }

        if (frameFirst < 0 || frameLast > animationLength)
            throw MeshKeyError("Layer has no keys");

        // Determine previous and next key frame.
        framePrev = INT_MIN;
        frameNext = INT_MAX;
        for (const auto &frameKeyPair : pLayer->mKeys)
        {
            keyFrame = std::get<0>(frameKeyPair);

            if (keyFrame <= frame && keyFrame > framePrev)
                framePrev = keyFrame;

            if (keyFrame >= frame && keyFrame < frameNext)
                frameNext = keyFrame;
        }

        if (framePrev < 0)
        {
            // What if there's no previous frame between 'frame' and the start of the animation?

            if (loop)
            {
                framePrev = frameLast;
                distanceToPrev = frame + float(animationLength - frameLast);
            }
            else
            {
                framePrev = frameFirst;
                if (frame < frameFirst)
                    distanceToPrev = 0.0f;
                else
                    distanceToPrev = frame - float(frameFirst);
            }
        }
        else
            distanceToPrev = frame - float(framePrev);

        if (frameNext > animationLength)
        {
            // What if there's no next frame between 'frame' and the end of the animation?

            if (loop)
            {
                frameNext = frameFirst;
                distanceToNext = float(animationLength) - frame + float(frameFirst);
            }
            else
            {
                frameNext = frameLast;
                if (frame > frameLast)
                    distanceToNext = 0.0f;
                else
                    distanceToNext = float(frameLast) - frame;
            }
        }
        else
            distanceToNext = float(frameNext) - frame;
    }

    void GetBoneTransformationsAt(const MeshData *pMeshData, const std::string &animationID,
                                  const milliseconds ms, const float framesPerSecond, const bool loop,
                                  std::unordered_map<std::string, MeshBoneTransformation> &transformationsOut)
    {
        const MeshSkeletalAnimation *pAnimation = pMeshData->GetAnimation(animationID);

        float frame;
        if (loop)
            frame = ModulateFrame(ms, framesPerSecond, pAnimation->length);
        else
            frame = ClampFrame(ms, framesPerSecond, pAnimation->length);

        std::string boneID;
        int framePrev, frameNext;
        float distanceToPrev, distanceToNext;
        const MeshBoneLayer *pLayer;
        const MeshBoneKey *pKey;
        for (const auto &idLayerPair : pAnimation->mLayers)
        {
            boneID = std::get<0>(idLayerPair);
            pLayer = &(std::get<1>(idLayerPair));

            PickKeyFrames(pLayer, frame, pAnimation->length, loop,
                          framePrev, frameNext,
                          distanceToPrev, distanceToNext);

            if (framePrev == frameNext)  // We hit an exact key frame.
            {
                const MeshBoneKey *pKey = &(pLayer->mKeys.at(framePrev));
                transformationsOut[boneID] = pKey->transformation;
            }
            else  // Need to interpolate between two key frames.
            {
                const MeshBoneKey *pKeyPrev = &(pLayer->mKeys.at(framePrev)),
                                  *pKeyNext = &(pLayer->mKeys.at(frameNext));

                transformationsOut[boneID] = Interpolate(pKeyPrev->transformation,
                                                         pKeyNext->transformation,
                                                         distanceToPrev / (distanceToPrev + distanceToNext));
            }
        }
    }

    const MeshBoneTransformation MESHBONETRANSFORM_ID = {quat(1.0f, 0.0f, 0.0f, 0.0f), vec3(0.0f)};


    void ApplyBoneTransformations(const MeshData *pMeshData,
                                  const std::unordered_map<std::string, MeshBoneTransformation> &boneTransformations,
                                  MeshState *pMeshState)
    {
        float pullWeight;
        vec3 transformedPosition, pivot;
        MeshBoneTransformation t;
        for (const MeshVertex *pVertex : pMeshData->IterVertices())
        {
            float sumWeight = 0.0f;
            vec3 sumPosition(0.0f);

            for (const MeshBone *pBone : pVertex->IterBones())
            {
                pullWeight = pBone->GetWeight();
                transformedPosition = pVertex->GetPosition();

                while (pBone != NULL)
                {
                    if (HAS_ID(boneTransformations, pBone->GetID()))
                    {
                        t = boneTransformations.at(pBone->GetID());

                        pivot = pBone->GetHeadPosition();

                        transformedPosition = t.rotation * (transformedPosition - pivot) + pivot + t.translation;
                    }
                    // else assume in rest position

                    pBone = pBone->GetParent();
                }

                sumPosition += transformedPosition * pullWeight;
                sumWeight += pullWeight;
            }

            // Average over all bones pulling directly at this vertex.
            pMeshState->GetVertex(pVertex->GetID())->SetPosition(sumPosition / sumWeight);
        }
    }
}
