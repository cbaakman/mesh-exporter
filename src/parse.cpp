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

#include <list>
#include <cstring>
#include <exception>
#include <math.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "mesh.h"
#include "build.h"


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


    bool iequals(const char *s1, const char *s2)
    {
        size_t i;
        for (i = 0; s1[i] != NULL && s2[i] != NULL; i++)
            if (toupper(s1[i]) != toupper(s2[i]))
                return false;

        return s1[i] == s2[i];  // must both be NULL
    }

    bool HasChild(xmlNodePtr pTag, const char *tagName)
    {
        for (xmlNodePtr pChild : XMLChildIterable(pTag))
        {
            if (iequals((const char *)pChild->name, tagName))
                return true;
        }
        return false;
    }

    xmlNodePtr FindChild(xmlNodePtr pParent, const char *tagName)
    {
        for (xmlNodePtr pChild : XMLChildIterable(pParent))
        {
            if (iequals((const char *)pChild->name, tagName))
                return pChild;
        }

        throw MeshParseError("No %s tag found in %s tag", tagName, (const char *)pParent->name);
    }

    std::list<xmlNodePtr> IterFindChildren(xmlNodePtr pParent, const char *tagName)
    {
        std::list<xmlNodePtr> l;

        for (xmlNodePtr pChild : XMLChildIterable(pParent))
        {
            if (iequals((const char *)pChild->name, tagName))
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
        if (strcmp("0", s) == 0)
            b = false;
        else if (iequals("true", s))
            b = true;
        else if (iequals("false", s))
            b = false;
        else
            b = true;
    }

    void ParseStringAttrib(const xmlNodePtr pTag, const xmlChar *key, std::string &s)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError("Missing %s attribute: %s", (const char *)pTag->name, (const char *)key);

        s.assign((const char *)pS);
        xmlFree(pS);
    }

    void ParseBoolAttrib(const xmlNodePtr pTag, const xmlChar *key, bool &b)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError("Missing %s attribute: %s", (const char *)pTag->name, (const char *)key);

        ParseBool((const char *)pS, b);

        xmlFree(pS);
    }

    void ParseFloatAttrib(const xmlNodePtr pTag, const xmlChar *key, float &f)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError("Missing %s attribute: %s", (const char *)pTag->name, (const char *)key);

        if (!ParseFloat((const char *)pS, f))
        {
            xmlFree(pS);
            throw MeshParseError("Malformed floating point: %s", (const char *)pS);
        }
        xmlFree(pS);
    }

    void ParseLengthAttrib(const xmlNodePtr pTag, const xmlChar *key, size_t &length)
    {
        xmlChar *pS = xmlGetProp(pTag, key);
        if (pS == nullptr)
            throw MeshParseError("Missing %s attribute: %s",(const char *)pTag->name, (const char *)key);

        int i = atoi((const char *)pS);
        if (i < 0)
        {
            MeshParseError err("%s cannot be %s", (const char *)key, (const char *)pS);
            xmlFree(pS);
            throw err;
        }

        length = i;
        xmlFree(pS);
    }

    void ParseVertex(const xmlNodePtr pVertexTag, MeshDataBuilder &builder)
    {
        std::string id;
        ParseStringAttrib(pVertexTag, (const xmlChar *)"id", id);

        xmlNodePtr pPositionTag = FindChild(pVertexTag, "pos");

        vec3 position;
        ParseFloatAttrib(pPositionTag, (const xmlChar *)"x", position.x);
        ParseFloatAttrib(pPositionTag, (const xmlChar *)"y", position.y);
        ParseFloatAttrib(pPositionTag, (const xmlChar *)"z", position.z);

        builder.AddVertex(id, position);
    }

    void ParseCorner(const xmlNodePtr pCornerTag, MeshTexCoords &texCoords, std::string &vertexID)
    {
        ParseStringAttrib(pCornerTag, (const xmlChar *)"vertex_id", vertexID);

        ParseFloatAttrib(pCornerTag, (const xmlChar *)"tex_u", texCoords.x);
        ParseFloatAttrib(pCornerTag, (const xmlChar *)"tex_v", texCoords.y);
    }

    void ParseQuadFace(const xmlNodePtr pQuadTag, MeshDataBuilder &builder)
    {
        std::string id;
        ParseStringAttrib(pQuadTag, (const xmlChar *)"id", id);

        bool smooth;
        ParseBoolAttrib(pQuadTag, (const xmlChar *)"smooth", smooth);

        size_t count = 0;
        MeshTexCoords tx[4];
        std::string vIDs[4];
        for (xmlNodePtr pCornerTag : IterFindChildren(pQuadTag, "corner"))
        {
            if (count < 4)
            {
                ParseCorner(pCornerTag, tx[count], vIDs[count]);
            }
            count++;
        }

        if (count != 4)
            throw MeshParseError("encountered a quad with %u corners", count);

        builder.AddQuad(id, smooth, tx, vIDs);
    }

    void ParseTriangleFace(const xmlNodePtr pTriangleTag, MeshDataBuilder &builder)
    {
        std::string id;
        ParseStringAttrib(pTriangleTag, (const xmlChar *)"id", id);

        bool smooth;
        ParseBoolAttrib(pTriangleTag, (const xmlChar *)"smooth", smooth);

        size_t count = 0;
        MeshTexCoords tx[3];
        std::string vIDs[3];
        for (xmlNodePtr pCornerTag : IterFindChildren(pTriangleTag, "corner"))
        {
            if (count < 3)
            {
                ParseCorner(pCornerTag, tx[count], vIDs[count]);
            }
            count++;
        }

        if (count != 3)
            throw MeshParseError("encountered a triangle with %u corners", count);

        builder.AddTriangle(id, smooth, tx, vIDs);
    }

    void ParseSubset(const xmlNodePtr pSubsetTag, MeshDataBuilder &builder)
    {
        const MeshFace *pFace;

        std::string id;
        ParseStringAttrib(pSubsetTag, (const xmlChar *)"id", id);

        builder.AddSubset(id);

        xmlNodePtr pFacesTag = FindChild(pSubsetTag, "faces");
        for (xmlNodePtr pQuadTag : IterFindChildren(pFacesTag, "quad"))
        {
            std::string quadID;
            ParseStringAttrib(pQuadTag, (const xmlChar *)"id", quadID);

            builder.AddQuadToSubset(id, quadID);
        }
        for (xmlNodePtr pTriangleTag : IterFindChildren(pFacesTag, "triangle"))
        {
            std::string triangleID;
            ParseStringAttrib(pTriangleTag, (const xmlChar *)"id", triangleID);

            builder.AddTriangleToSubset(id, triangleID);
        }
    }

    void ParseBone(const xmlNodePtr pBoneTag, MeshDataBuilder &builder)
    {
        std::string id;
        ParseStringAttrib(pBoneTag, (const xmlChar *)"id", id);

        vec3 headPosition;
        ParseFloatAttrib(pBoneTag, (const xmlChar *)"x", headPosition.x);
        ParseFloatAttrib(pBoneTag, (const xmlChar *)"y", headPosition.y);
        ParseFloatAttrib(pBoneTag, (const xmlChar *)"z", headPosition.z);

        float weight;
        ParseFloatAttrib(pBoneTag, (const xmlChar *)"weight", weight);

        builder.AddBone(id, headPosition, weight);

        if (HasChild(pBoneTag, "vertices"))
        {
            xmlNodePtr pVerticesTag = FindChild(pBoneTag, "vertices");
            for (xmlNodePtr pVertexTag : IterFindChildren(pVerticesTag, "vertex"))
            {
                std::string vertexID;
                ParseStringAttrib(pVertexTag, (const xmlChar *)"id", vertexID);

                builder.ConnectBoneToVertex(id, vertexID);
            }
        }

        std::string parentID;
        if (xmlHasProp(pBoneTag, (const xmlChar *)"parent_id"))
        {
            ParseStringAttrib(pBoneTag, (const xmlChar *)"parent_id", parentID);

            builder.ConnectBones(parentID, id);
        }
    }

    void ParseKey(const xmlNodePtr pKeyTag,
                  const std::string animationID, const std::string &boneID,
                  MeshDataBuilder &builder)
    {
        size_t frame;
        ParseLengthAttrib(pKeyTag, (const xmlChar *)"frame", frame);

        MeshBoneTransformation transformation = MESHBONETRANSFORM_ID;

        if (xmlHasProp(pKeyTag, (const xmlChar *)"x"))
        {
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"x", transformation.translation.x);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"y", transformation.translation.y);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"z", transformation.translation.z);
        }
        if (xmlHasProp(pKeyTag, (const xmlChar *)"rot_x"))
        {
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_x", transformation.rotation.x);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_y", transformation.rotation.y);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_z", transformation.rotation.z);
            ParseFloatAttrib(pKeyTag, (const xmlChar *)"rot_w", transformation.rotation.w);
        }

        builder.AddKey(animationID, boneID, frame, transformation);
    }

    void ParseLayer(const xmlNodePtr pLayerTag, const std::string &animationID, MeshDataBuilder &builder)
    {
        std::string boneID;
        ParseStringAttrib(pLayerTag, (const xmlChar *)"bone_id", boneID);

        builder.AddLayer(animationID, boneID);

        size_t countKeys = 0;
        for (xmlNodePtr pKeyTag : IterFindChildren(pLayerTag, "key"))
        {
            ParseKey(pKeyTag, animationID, boneID, builder);
            countKeys++;
        }

        if (countKeys <= 0)
            throw MeshParseError("Layer %s in animation %s has no keys", boneID.c_str(), animationID.c_str());
    }

    void ParseAnimation(const xmlNodePtr pAnimationTag, MeshDataBuilder &builder)
    {
        std::string id;
        ParseStringAttrib(pAnimationTag, (const xmlChar *)"id", id);

        size_t length;
        ParseLengthAttrib(pAnimationTag, (const xmlChar *)"length", length);

        builder.AddAnimation(id, length);

        for (xmlNodePtr pLayerTag : IterFindChildren(pAnimationTag, "layer"))
        {
            ParseLayer(pLayerTag, id, builder);
        }
    }

    MeshData *ParseMeshData(std::istream &is)
    {
        const xmlDocPtr pDoc = ParseXML(is);

        MeshDataBuilder builder;

        try
        {
            xmlNodePtr pRoot = xmlDocGetRootElement(pDoc);
            if(pRoot == nullptr)
                throw MeshParseError("no root element found in xml tree");

            if (!iequals((const char *)pRoot->name, "mesh"))
                throw MeshParseError("root element is not \"mesh\"");

            // First, parse all the vertices.
            xmlNodePtr pVerticesTag = FindChild(pRoot, "vertices");
            for (xmlNodePtr pVertexTag : IterFindChildren(pVerticesTag, "vertex"))
            {
                ParseVertex(pVertexTag, builder);
            }

            // Next, the faces that connect the vertices.
            xmlNodePtr pFacesTag = FindChild(pRoot, "faces");
            for (xmlNodePtr pQuadTag : IterFindChildren(pFacesTag, "quad"))
            {
                ParseQuadFace(pQuadTag, builder);
            }
            for (xmlNodePtr pTriangleTag : IterFindChildren(pFacesTag, "triangle"))
            {
                ParseTriangleFace(pTriangleTag, builder);
            }

            // Then, the subsets that contain faces.
            xmlNodePtr pSubsetsTag = FindChild(pRoot, "subsets");
            for (xmlNodePtr pSubsetTag : IterFindChildren(pSubsetsTag, "subset"))
            {
                ParseSubset(pSubsetTag, builder);
            }

            if (HasChild(pRoot, "armature"))
            {
                // Then the bones, (optional) which are attached to vertices.
                xmlNodePtr pArmatureTag = FindChild(pRoot, "armature");
                xmlNodePtr pBonesTag = FindChild(pArmatureTag, "bones");
                for (xmlNodePtr pBoneTag : IterFindChildren(pBonesTag, "bone"))
                {
                    ParseBone(pBoneTag, builder);
                }

                if (HasChild(pArmatureTag, "animations"))
                {
                    // Parse the animations, involving the bones. (optional)
                    xmlNodePtr pAnimationsTag = FindChild(pArmatureTag, "animations");
                    for (xmlNodePtr pAnimationTag : IterFindChildren(pAnimationsTag, "animation"))
                    {
                        ParseAnimation(pAnimationTag, builder);
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
        return builder.GetMeshData();
    }
}
