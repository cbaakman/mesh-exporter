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

#include <boost/format.hpp>
#include <linear-gl/matrix.h>

#include "mesh.h"


namespace XMLMesh
{
    class MeshKeyError: public std::exception
    {
        private:
            std::string message;
        public:
            MeshKeyError(const std::string &msg)
            {
                message = msg;
            }
            MeshKeyError(const boost::format &fmt)
            {
                message = fmt.str();
            }

            const char *what(void) const noexcept
            {
                return message.c_str();
            }
    };

    void CopyVertex(const std::string &vertexID, const MeshVertex &vertex, MeshState &meshState)
    {
        meshState.mVertices[vertexID].id = vertexID;
        meshState.mVertices[vertexID].position = vertex.position;
    }

    void CopyCorners(const MeshFace &srcFace, const size_t cornerCount, MeshFace &dstFace, MeshState &meshState)
    {
        size_t i, iprev, inext;
        std::string vertexID;
        for (i = 0; i < cornerCount; i++)
        {
            iprev = (cornerCount + i - 1) % cornerCount;
            inext = (i + 1) % cornerCount;

            dstFace.mCorners[i].texCoords = srcFace.mCorners[i].texCoords;
            vertexID = srcFace.mCorners[i].pVertex->id;
            if (!HAS_ID(meshState.mVertices, vertexID))
                throw MeshKeyError(boost::format("No such vertex in mesh state: %1%") % vertexID);

            // Connect everything.
            dstFace.mCorners[i].pVertex = &(meshState.mVertices[vertexID]);
            meshState.mVertices[vertexID].cornersInvolvedPs.push_back(&(dstFace.mCorners[i]));
            dstFace.mCorners[i].pFace = &dstFace;
            dstFace.mCorners[i].pPrev = &(dstFace.mCorners[iprev]);
            dstFace.mCorners[i].pNext = &(dstFace.mCorners[inext]);
        }
    }

    void CopyQuad(const std::string &quadID, const MeshQuadFace &quad, MeshState &meshState)
    {
        meshState.mQuads[quadID].id = quadID;
        meshState.mQuads[quadID].smooth = quad.smooth;

        CopyCorners(quad, 4, meshState.mQuads[quadID], meshState);
    }

    void CopyTriangle(const std::string &triangleID, const MeshTriangleFace &triangle, MeshState &meshState)
    {
        meshState.mTriangles[triangleID].id = triangleID;
        meshState.mTriangles[triangleID].smooth = triangle.smooth;

        CopyCorners(triangle, 3, meshState.mTriangles[triangleID], meshState);
    }

    void CopySubset(const std::string &subsetID, const MeshSubset &subset, MeshState &meshState)
    {
        std::string faceID;

        meshState.mSubsets[subsetID].id = subsetID;

        for (const MeshFace *pFace : subset.facePs)
        {
            faceID = pFace->id;

            if (HAS_ID(meshState.mQuads, faceID))
                meshState.mSubsets[subsetID].facePs.push_back(&meshState.mQuads[faceID]);
            else if (HAS_ID(meshState.mTriangles, faceID))
                meshState.mSubsets[subsetID].facePs.push_back(&meshState.mTriangles[faceID]);
            else
                throw MeshKeyError(boost::format("No such face: %1%") % faceID);
        }
    }

    void DeriveMeshState(const MeshData &meshData, MeshState &meshState)
    {
        // First copy the vertices.
        for (const auto &pair : meshData.mVertices)
            CopyVertex(std::get<0>(pair), std::get<1>(pair), meshState);

        // Next copy the faces, that connect the vertices.
        for (const auto &pair : meshData.mQuads)
            CopyQuad(std::get<0>(pair), std::get<1>(pair), meshState);
        for (const auto &pair : meshData.mTriangles)
            CopyTriangle(std::get<0>(pair), std::get<1>(pair), meshState);

        // Finally copy the subset, holding the faces.
        for (const auto &pair : meshData.mSubsets)
            CopySubset(std::get<0>(pair), std::get<1>(pair), meshState);
    }

    MeshBoneTransformation Interpolate(const MeshBoneTransformation &t0,
                                       const MeshBoneTransformation &t1,
                                       const float s)
    {
        MeshBoneTransformation r;

        r.rotation = Slerp(t0.rotation, t1.rotation, s);
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


    void BoneTransformationToMatrix(const MeshBoneTransformation &t, matrix4 &m)
    {
        m = MatTranslate(t.translation) * MatQuat(t.rotation);
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

    void GetBoneTransformationsAt(const MeshData &meshData, const std::string &animationID,
                                  const milliseconds ms, const float framesPerSecond, const bool loop,
                                  std::map<std::string, MeshBoneTransformation> &transformationsOut)
    {
        if (!HAS_ID(meshData.mAnimations, animationID))
            throw MeshKeyError(boost::format("No such animation: %1%") % animationID);

        const MeshSkeletalAnimation *pAnimation = &(meshData.mAnimations.at(animationID));

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

    const MeshBoneTransformation MESHBONETRANSFORM_ID = {QUATERNION_ID, VEC3_ZERO};

    void ApplyBoneTransformations(const MeshData &meshData,
                                  const std::map<std::string, MeshBoneTransformation> &boneTransformations,
                                  MeshState &meshStateOut)
    {
        const MeshVertex *pVertex;
        float pullWeight;
        vec3 transformedPosition, pivot;
        MeshBoneTransformation t;
        for (const auto &pair : meshData.mVertices)
        {
            pVertex = &(std::get<1>(pair));

            float sumWeight = 0.0f;
            vec3 sumPosition = VEC3_ZERO;

            for (const MeshBone *pBone : pVertex->bonesPullingPs)
            {
                pullWeight = pBone->weight;
                transformedPosition = pVertex->position;

                while (pBone != NULL)
                {
                    if (HAS_ID(boneTransformations, pBone->id))
                    {
                        t = boneTransformations.at(pBone->id);

                        pivot = pBone->headPosition;

                        transformedPosition = Rotate(t.rotation, transformedPosition - pivot) + pivot + t.translation;
                    }
                    // else assume in rest position

                    pBone = pBone->pParent;
                }

                sumPosition += transformedPosition * pullWeight;
                sumWeight += pullWeight;
            }

            // Average over all bones pulling directly at this vertex.
            meshStateOut.mVertices[pVertex->id].position = sumPosition / sumWeight;
        }
    }
}
