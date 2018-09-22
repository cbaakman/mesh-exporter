#include <exception>
#include <math.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "mesh.h"


namespace XMLMesh
{
    class XMLChildIterator : public std::iterator<std::input_iterator_tag, xmlNodePtr>
    {
        private:
            xmlNodePtr pChildren;
        public:
            XMLChildIterator(xmlNodePtr p): pChildren(p) {}
            XMLChildIterator(const XMLChildIterator &it): pChildren(it.pChildren) {}

            XMLChildIterator &operator++(void)
            {
                pChildren = pChildren->next;
                return *this;
            }
            XMLChildIterator operator++(int)
            {
                XMLChildIterator tmp(*this);
                operator++();
                return tmp;
            }
            bool operator==(const XMLChildIterator &other) const
            {
                return pChildren == other.pChildren;
            }
            bool operator!=(const XMLChildIterator &other) const
            {
                return pChildren != other.pChildren;
            }
            xmlNodePtr operator*(void)
            {
                return pChildren;
            }
    };

    class XMLChildIterable
    {
        private:
            xmlNodePtr pParent;
        public:
            XMLChildIterable(xmlNodePtr p): pParent(p) {}

            XMLChildIterator begin(void)
            {
                return XMLChildIterator(pParent->children);
            }

            XMLChildIterator end(void)
            {
                return XMLChildIterator(nullptr);
            }
    };


    class MeshParseError : public std::exception
    {
        private:
            std::string message;
        public:
            MeshParseError(const std::string &msg)
            {
                message = msg;
            }

            MeshParseError(const boost::format &fmt)
            {
                message = fmt.str();
            }

            const char *what(void) const noexcept
            {
                return message.c_str();
            }
    };

    bool HasChild(xmlNodePtr pTag, const char *tagName)
    {
        for (xmlNodePtr pChild : XMLChildIterable(pTag))
        {
            if (boost::iequals((const char *)pChild->name, tagName))
                return true;
        }
        return false;
    }

    xmlNodePtr FindChild(xmlNodePtr pParent, const char *tagName)
    {
        for (xmlNodePtr pChild : XMLChildIterable(pParent))
        {
            if (boost::iequals((const char *)pChild->name, tagName))
                return pChild;
        }

        throw MeshParseError(boost::format("No %1% tag found in %2% tag")
                         % tagName % ((const char *)pParent->name));
    }

    std::list<xmlNodePtr> IterFindChildren(xmlNodePtr pParent, const char *tagName)
    {
        std::list<xmlNodePtr> l;

        for (xmlNodePtr pChild : XMLChildIterable(pParent))
        {
            if (boost::iequals((const char *)pChild->name, tagName))
                l.push_back(pChild);
        }

        return l;
    }

    xmlDocPtr ParseXML(std::istream &is)
    {
        const size_t bufSize = 1024;
        std::streamsize res;
        char buf[bufSize];
        int wellFormed;

        xmlParserCtxtPtr pCtxt;
        xmlDocPtr pDoc;

        // Read the first 4 bytes.
        is.read(buf, 4);
        if (!is.good())
            throw MeshParseError("Error reading the first xml bytes!");
        res = is.gcount();

        // Create a progressive parsing context.
        pCtxt = xmlCreatePushParserCtxt(NULL, NULL, buf, res, NULL);
        if (!pCtxt)
            throw MeshParseError("Failed to create parser context!");

        // Loop on the input, getting the document data.
        while (is.good())
        {
            is.read(buf, bufSize);
            res = is.gcount();

            xmlParseChunk(pCtxt, buf, res, 0);
        }

        // There is no more input, indicate the parsing is finished.
        xmlParseChunk(pCtxt, buf, 0, 1);

        // Check if it was well formed.
        pDoc = pCtxt->myDoc;
        wellFormed = pCtxt->wellFormed;
        xmlFreeParserCtxt(pCtxt);

        if (!wellFormed)
        {
            xmlFreeDoc(pDoc);
            throw MeshParseError("xml document is not well formed");
        }

        return pDoc;
    }

    /**
     * Case independent, always uses a dot as decimal separator.
     *
     * :return the pointer to the string after the floating point number.
     */
    const char *ParseFloat(const char *in, float &out)
    {
        float f = 10.0f;
        int digit, ndigit = 0;

        // start from zero
        out = 0.0f;

        const char *p = in;
        while (*p)
        {
            if (isdigit(*p))
            {
                digit = (*p - '0');

                if (f > 1.0f) // left from period
                {
                    out *= f;
                    out += digit;
                }
                else // right from period, decimal
                {
                    out += f * digit;
                    f *= 0.1f;
                }
                ndigit++;
            }
            else if (tolower(*p) == 'e')
            {
                // exponent

                // if no digits precede the exponent, assume 1
                if (ndigit <= 0)
                    out = 1.0f;

                p++;
                if (*p == '+') // '+' just means positive power, default
                    p++; // skip it, don't give it to atoi

                int e = atoi(p);

                out = out * pow(10, e);

                // read past exponent number
                if (*p == '-')
                    p++;

                while (isdigit(*p))
                    p++;

                return p;
            }
            else if (*p == '.')
            {
                // expect decimal digits after this

                f = 0.1f;
            }
            else if (*p == '-')
            {
                // negative number
                float v;
                p = ParseFloat(p + 1, v);

                out = -v;

                return p;
            }
            else
            {
                // To assume success, must have read at least one digit
                if (ndigit > 0)
                    return p;
                else
                    return nullptr;
            }
            p++;
        }

        return p;
    }

    void ParseBool(const char *s, bool &b)
    {
        if (boost::equals("0", s))
            b = false;
        else if (boost::iequals("true", s))
            b = true;
        else if (boost::iequals("false", s))
            b = false;
        else
            b = true;
    }

    void ParseStringAttrib(const xmlNodePtr pTag, const xmlChar *key, std::string &s)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError(boost::format("Missing %1% attribute: %2%")
                             % ((const char *)pTag->name) % ((const char *)key));

        s.assign((const char *)pS);
        xmlFree(pS);
    }

    void ParseBoolAttrib(const xmlNodePtr pTag, const xmlChar *key, bool &b)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError(boost::format("Missing %1% attribute: %2%")
                             % ((const char *)pTag->name) % ((const char *)key));

        ParseBool((const char *)pS, b);

        xmlFree(pS);
    }

    void ParseFloatAttrib(const xmlNodePtr pTag, const xmlChar *key, float &f)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError(boost::format("Missing %1% attribute: %2%")
                             % ((const char *)pTag->name) % ((const char *)key));

        if (!ParseFloat((const char *)pS, f))
        {
            xmlFree(pS);
            throw MeshParseError(boost::format("Malformed floating point: %1%")
                             % ((const char *)pS));
        }
        xmlFree(pS);
    }

    void ParseLengthAttrib(const xmlNodePtr pTag, const xmlChar *key, size_t &i)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError(boost::format("Missing %1% attribute: %2%")
                             % ((const char *)pTag->name) % ((const char *)key));
        try
        {
            i = boost::lexical_cast<size_t>((const char *)pS);
        }
        catch(...)
        {
            xmlFree(pS);
            std::rethrow_exception(std::current_exception());
        }
        xmlFree(pS);
    }

    void ParseVertex(const xmlNodePtr pVertexTag, MeshData &meshData)
    {
        std::string id;
        ParseStringAttrib(pVertexTag, (const xmlChar *)"id", id);

        if (HAS_ID(meshData.mVertices, id))
            throw MeshParseError(boost::format("duplicate vertex id: %1%") % id);
        meshData.mVertices[id].id = id;

        xmlNodePtr pPositionTag = FindChild(pVertexTag, "pos");

        ParseFloatAttrib(pPositionTag, (const xmlChar *)"x", meshData.mVertices[id].position.x);
        ParseFloatAttrib(pPositionTag, (const xmlChar *)"y", meshData.mVertices[id].position.y);
        ParseFloatAttrib(pPositionTag, (const xmlChar *)"z", meshData.mVertices[id].position.z);
    }

    void ParseCorner(const xmlNodePtr pCornerTag,
                     MeshData &meshData, MeshCorner &corner)
    {
        std::string vertexID;
        ParseStringAttrib(pCornerTag, (const xmlChar *)"vertex_id", vertexID);

        if (!HAS_ID(meshData.mVertices, vertexID))
            throw MeshParseError(boost::format("No such vertex: %1%") % vertexID);

        corner.pVertex = &(meshData.mVertices[vertexID]);

        ParseFloatAttrib(pCornerTag, (const xmlChar *)"tex_u", corner.texCoords.u);
        ParseFloatAttrib(pCornerTag, (const xmlChar *)"tex_v", corner.texCoords.v);
    }

    void Connect(MeshFace *pFace, MeshCorner *pCorners, const size_t cornerCount)
    {
        size_t i, iprev, inext;
        for (i = 0; i < cornerCount; i++)
        {
            iprev = (cornerCount + i - 1) % cornerCount;
            inext = (i + 1) % cornerCount;

            pCorners[i].pFace = pFace;
            pCorners[i].pVertex->cornersInvolvedPs.push_back(&(pCorners[i]));
            pCorners[i].pPrev = &(pCorners[iprev]);
            pCorners[i].pNext = &(pCorners[inext]);
        }
    }

    void ParseQuadFace(const xmlNodePtr pQuadTag, MeshData &meshData)
    {
        std::string id;
        ParseStringAttrib(pQuadTag, (const xmlChar *)"id", id);

        if (HAS_ID(meshData.mQuads, id) || HAS_ID(meshData.mTriangles, id))
            throw MeshParseError(boost::format("duplicate face id: %1%") % id);
        meshData.mQuads[id].id = id;

        ParseBoolAttrib(pQuadTag, (const xmlChar *)"smooth", meshData.mQuads[id].smooth);

        size_t count = 0;
        for (xmlNodePtr pCornerTag : IterFindChildren(pQuadTag, "corner"))
        {
            ParseCorner(pCornerTag, meshData,
                        meshData.mQuads[id].mCorners[count]);
            count++;
        }

        if (count != 4)
            throw MeshParseError(boost::format("encountered a quad with %1% corners") % count);

        Connect(&(meshData.mQuads[id]), meshData.mQuads[id].mCorners, 4);
    }

    void ParseTriangleFace(const xmlNodePtr pTriangleTag, MeshData &meshData)
    {
        std::string id;
        ParseStringAttrib(pTriangleTag, (const xmlChar *)"id", id);

        if (HAS_ID(meshData.mQuads, id) || HAS_ID(meshData.mTriangles, id))
            throw MeshParseError(boost::format("duplicate face id: %1%") % id);
        meshData.mTriangles[id].id = id;

        ParseBoolAttrib(pTriangleTag, (const xmlChar *)"smooth", meshData.mTriangles[id].smooth);

        size_t count = 0;
        for (xmlNodePtr pCornerTag : IterFindChildren(pTriangleTag, "corner"))
        {
            ParseCorner(pCornerTag, meshData,
                        meshData.mTriangles[id].mCorners[count]);
            count++;
        }

        if (count != 3)
            throw MeshParseError(boost::format("encountered a triangle with %1% corners") % count);

        Connect(&(meshData.mTriangles[id]), meshData.mTriangles[id].mCorners, 3);
    }

    void ParseSubset(const xmlNodePtr pSubsetTag, MeshData &meshData)
    {
        std::string id;
        ParseStringAttrib(pSubsetTag, (const xmlChar *)"id", id);

        if (HAS_ID(meshData.mSubsets, id))
            throw MeshParseError(boost::format("duplicate subset id: %1%") % id);
        meshData.mSubsets[id].id = id;

        xmlNodePtr pFacesTag = FindChild(pSubsetTag, "faces");
        for (xmlNodePtr pQuadTag : IterFindChildren(pFacesTag, "quad"))
        {
            std::string quadID;
            ParseStringAttrib(pQuadTag, (const xmlChar *)"id", quadID);

            if (!HAS_ID(meshData.mQuads, quadID))
                throw MeshParseError(boost::format("No such quad: %1%") % quadID);

            meshData.mSubsets[id].facePs.push_back(&(meshData.mQuads[quadID]));
        }
        for (xmlNodePtr pTriangleTag : IterFindChildren(pFacesTag, "triangle"))
        {
            std::string triangleID;
            ParseStringAttrib(pTriangleTag, (const xmlChar *)"id", triangleID);

            if (!HAS_ID(meshData.mTriangles, triangleID))
                throw MeshParseError(boost::format("No such triangle: %1%") % triangleID);

            meshData.mSubsets[id].facePs.push_back(&(meshData.mTriangles[triangleID]));
        }
    }

    void ParseBone(const xmlNodePtr pBoneTag, MeshData &meshData)
    {
        std::string id;
        ParseStringAttrib(pBoneTag, (const xmlChar *)"id", id);

        if (HAS_ID(meshData.mBones, id))
            throw MeshParseError(boost::format("duplicate bone id: %1%") % id);
        meshData.mBones[id].id = id;
        meshData.mBones[id].pParent = nullptr;

        ParseFloatAttrib(pBoneTag, (const xmlChar *)"x",
                         meshData.mBones[id].headPosition.x);
        ParseFloatAttrib(pBoneTag, (const xmlChar *)"y",
                         meshData.mBones[id].headPosition.y);
        ParseFloatAttrib(pBoneTag, (const xmlChar *)"z",
                         meshData.mBones[id].headPosition.z);

        ParseFloatAttrib(pBoneTag, (const xmlChar *)"weight",
                         meshData.mBones[id].weight);

        if (HasChild(pBoneTag, "vertices"))
        {
            xmlNodePtr pVerticesTag = FindChild(pBoneTag, "vertices");
            for (xmlNodePtr pVertexTag : IterFindChildren(pVerticesTag, "vertex"))
            {
                std::string vertexID;
                ParseStringAttrib(pVertexTag, (const xmlChar *)"id", vertexID);

                if (!HAS_ID(meshData.mVertices, vertexID))
                    throw MeshParseError(boost::format("No such vertex: %1%") % vertexID);

                meshData.mBones[id].vertexPs.push_back(&(meshData.mVertices[vertexID]));
                meshData.mVertices[vertexID].bonesPullingPs.push_back(&(meshData.mBones[id]));
            }
        }
    }

    void ConnectBone(const xmlNodePtr pBoneTag, std::map<std::string, MeshBone> &bones)
    {
        std::string id;
        ParseStringAttrib(pBoneTag, (const xmlChar *)"id", id);

        std::string parentID;
        if (xmlHasProp(pBoneTag, (const xmlChar *)"parent_id"))
        {
            ParseStringAttrib(pBoneTag, (const xmlChar *)"parent_id", parentID);

            if (!HAS_ID(bones, parentID))
                throw MeshParseError(boost::format("No such bone: %1%") % parentID);

            bones[id].pParent = &(bones[parentID]);
        }
    }

    void ParseKey(const xmlNodePtr pKeyTag, MeshBoneLayer &layer)
    {
        size_t frame;
        ParseLengthAttrib(pKeyTag, (const xmlChar *)"frame", frame);

        layer.mKeys[frame].frame = frame;
        layer.mKeys[frame].transformation.rotation = QUATERNION_ID;
        layer.mKeys[frame].transformation.translation = VEC3_ZERO;

        if (xmlHasProp(pKeyTag, (const xmlChar *)"x"))
        {
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"x",
                             layer.mKeys[frame].transformation.translation.x);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"y",
                             layer.mKeys[frame].transformation.translation.y);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"z",
                             layer.mKeys[frame].transformation.translation.z);
        }
        if (xmlHasProp(pKeyTag, (const xmlChar *)"rot_x"))
        {
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_x",
                             layer.mKeys[frame].transformation.rotation.x);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_y",
                             layer.mKeys[frame].transformation.rotation.y);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_z",
                             layer.mKeys[frame].transformation.rotation.z);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_w",
                             layer.mKeys[frame].transformation.rotation.w);
        }
    }

    void ParseLayer(const xmlNodePtr pLayerTag,
                    MeshSkeletalAnimation &animation,
                    MeshData &meshData)
    {
        std::string boneID;
        ParseStringAttrib(pLayerTag, (const xmlChar *)"bone_id", boneID);

        if (!HAS_ID(meshData.mBones, boneID))
            throw MeshParseError(boost::format("No such bone: %1%") % boneID);

        animation.mLayers[boneID].pBone = &(meshData.mBones[boneID]);

        size_t countKeys = 0;
        for (xmlNodePtr pKeyTag : IterFindChildren(pLayerTag, "key"))
        {
            ParseKey(pKeyTag, animation.mLayers[boneID]);
            countKeys++;
        }

        if (countKeys <= 0)
            throw MeshParseError(boost::format("Layer %1% in animation %2% has no keys") % boneID % animation.id);
    }

    void ParseAnimation(const xmlNodePtr pAnimationTag, MeshData &meshData)
    {
        std::string id;
        ParseStringAttrib(pAnimationTag, (const xmlChar *)"id", id);

        if (HAS_ID(meshData.mAnimations, id))
            throw MeshParseError(boost::format("duplicate animation id: %1%") % id);
        meshData.mAnimations[id].id = id;

        size_t length;
        ParseLengthAttrib(pAnimationTag, (const xmlChar *)"length", length);
        meshData.mAnimations[id].length = length;

        for (xmlNodePtr pLayerTag : IterFindChildren(pAnimationTag, "layer"))
        {
            ParseLayer(pLayerTag, meshData.mAnimations[id], meshData);
        }
    }

    void ParseMesh(std::istream &is, MeshData &meshData)
    {
        const xmlDocPtr pDoc = ParseXML(is);

        try
        {
            xmlNodePtr pRoot = xmlDocGetRootElement(pDoc);
            if(pRoot == nullptr)
                throw MeshParseError("no root element found in xml tree");

            if (!boost::iequals((const char *)pRoot->name, "mesh"))
                throw MeshParseError("no root element is not \"mesh\"");

            // First, parse all the vertices.
            xmlNodePtr pVerticesTag = FindChild(pRoot, "vertices");
            for (xmlNodePtr pVertexTag : IterFindChildren(pVerticesTag, "vertex"))
            {
                ParseVertex(pVertexTag, meshData);
            }

            // Next, the faces that connect the vertices.
            xmlNodePtr pFacesTag = FindChild(pRoot, "faces");
            for (xmlNodePtr pQuadTag : IterFindChildren(pFacesTag, "quad"))
            {
                ParseQuadFace(pQuadTag, meshData);
            }
            for (xmlNodePtr pTriangleTag : IterFindChildren(pFacesTag, "triangle"))
            {
                ParseTriangleFace(pTriangleTag, meshData);
            }

            // Then, the subsets that contain faces.
            xmlNodePtr pSubsetsTag = FindChild(pRoot, "subsets");
            for (xmlNodePtr pSubsetTag : IterFindChildren(pSubsetsTag, "subset"))
            {
                ParseSubset(pSubsetTag, meshData);
            }

            if (HasChild(pRoot, "armature"))
            {
                // Then the bones, (optional) which are attached to vertices.
                xmlNodePtr pArmatureTag = FindChild(pRoot, "armature");
                xmlNodePtr pBonesTag = FindChild(pArmatureTag, "bones");
                for (xmlNodePtr pBoneTag : IterFindChildren(pBonesTag, "bone"))
                {
                    ParseBone(pBoneTag, meshData);
                }
                // Now that the bones are there, connect them to each other.
                for (xmlNodePtr pBoneTag : IterFindChildren(pBonesTag, "bone"))
                {
                    ConnectBone(pBoneTag, meshData.mBones);
                }

                if (HasChild(pArmatureTag, "animations"))
                {
                    // Parse the animations, involving the bones. (optional)
                    xmlNodePtr pAnimationsTag = FindChild(pArmatureTag, "animations");
                    for (xmlNodePtr pAnimationTag : IterFindChildren(pAnimationsTag, "animation"))
                    {
                        ParseAnimation(pAnimationTag, meshData);
                    }
                }
            }
        }
        catch(...)
        {
            xmlFreeDoc(pDoc);

            std::rethrow_exception(std::current_exception());
        }

        xmlFreeDoc(pDoc);
    }
}
