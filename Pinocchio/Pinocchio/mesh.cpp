/*  This file is part of the Pinocchio automatic rigging library.
    Copyright (C) 2007 Ilya Baran (ibaran@mit.edu)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "mesh.h"
#include "hashutils.h"
#include "utils.h"
#include "debugging.h"

#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <cstring>

namespace Pinocchio {
    Mesh::Mesh(const std::string &file, float weight)
    : scale(1.), withTexture(false), blendWeight(weight) {
        int i;
        #define OUT { vertices.clear(); edges.clear(); return; }
        std::ifstream obj(file.c_str());

        if(!obj.is_open()) {
            Debugging::out() << "Error opening file " << file << std::endl;
            return;
        }

        Debugging::out() << "Reading " << file << std::endl;

        if(file.length() < 4) {
            Debugging::out() << "I don't know what kind of file it is" << std::endl;
            return;
        }

        if(std::string(file.end() - 4, file.end()) == std::string(".obj")) {
            readObj(obj);
        } else {
            Debugging::out() << "I don't know what kind of file it is" << std::endl;
            return;
        }

        // reconstruct the rest of the information
        int verts = vertices.size();

        if(verts == 0)
            return;

        for(i = 0; i < (int)edges.size(); ++i) { // make sure all vertex indices are valid
            if(edges[i].vertex < 0 || edges[i].vertex >= verts) {
                Debugging::out() << "Error: invalid vertex index " << edges[i].vertex << std::endl;
                OUT;
            }
        }

        fixDupFaces();

        computeTopology();

        if (integrityCheck()) {
            Debugging::out() << "Successfully read " << file << ": " << vertices.size() << " vertices, " << edges.size() << " edges" << std::endl;
        }
        else {
            Debugging::out() << "Somehow read " << file << ": " << vertices.size() << " vertices, " << edges.size() << " edges" << std::endl;
        }

        computeVertexNormals();
    }

    void Mesh::computeTopology() {
        int i;
        for(i = 0; i < (int)edges.size(); ++i) {
            edges[i].prev = (i - i % 3) + (i + 2) % 3;
        }

        std::vector<std::map<int, int> > halfEdgeMap(vertices.size());
        for(i = 0; i < (int)edges.size(); ++i) {
            int v1 = edges[i].vertex;
            int v2 = edges[edges[i].prev].vertex;

            vertices[v1].edge = edges[edges[i].prev].prev; // assign the vertex' edge

            if(halfEdgeMap[v1].count(v2)) {
                Debugging::out() << "Error: duplicate edge detected: " << v1 << " to " << v2 << std::endl;
                OUT;
            }

            halfEdgeMap[v1][v2] = i;
            if(halfEdgeMap[v2].count(v1)) {
                int twin = halfEdgeMap[v2][v1];
                edges[twin].twin = i;
                edges[i].twin = twin;
            }
        }
    }

    void Mesh::computeVertexNormals() {
        int i;
        for(i = 0; i < (int)vertices.size(); ++i)
            vertices[i].normal = Vector3();
        for(i = 0; i < (int)edges.size(); i += 3) {
            int i1 = edges[i].vertex;
            int i2 = edges[i + 1].vertex;
            int i3 = edges[i + 2].vertex;
            Vector3 normal = ((vertices[i2].pos - vertices[i1].pos) % (vertices[i3].pos - vertices[i1].pos)).normalize();
            vertices[i1].normal += normal;
            vertices[i2].normal += normal;
            vertices[i3].normal += normal;
        }
        for(i = 0; i < (int)vertices.size(); ++i) {
            vertices[i].normal = vertices[i].normal.normalize();
        }
    }

    void Mesh::normalizeBoundingBox() {
        int i;
        std::vector<Vector3> positions;
        for(i = 0; i < (int)vertices.size(); ++i) {
            positions.push_back(vertices[i].pos);
        }
        Rect3 boundingBox = Rect3(positions.begin(), positions.end());
        double cscale = .9 / boundingBox.getSize().accumulate(ident<double>(), maximum<double>());
        Vector3 ctoAdd = Vector3(0.5, 0.5, 0.5) - boundingBox.getCenter() * cscale;
        for(i = 0; i < (int)vertices.size(); ++i) {
            vertices[i].pos = ctoAdd + vertices[i].pos * cscale;
        }
        toAdd = ctoAdd + cscale * toAdd;
        scale *= cscale;
    }

    struct MFace
    {
        MFace(int v1, int v2, int v3) {
            v[0] = v1; v[1] = v2; v[2] = v3;
            std::sort(v, v + 3);
        }

        bool operator<(const MFace &f) const { return std::lexicographical_compare(v, v + 3, f.v, f.v + 3); }
        int v[3];
    };

    void Mesh::fixDupFaces() {
        int i;
        std::map<MFace, int> faces;
        for(i = 0; i < (int)edges.size(); i += 3) {
            MFace current(edges[i].vertex, edges[i + 1].vertex, edges[i + 2].vertex);

            if(faces.count(current)) {
                int oth = faces[current];
                if(oth == -1) {
                    faces[current] = i;
                    continue;
                }
                faces[current] = -1;
                int newOth = edges.size() - 6;
                int newCur = edges.size() - 3;

                edges[oth] = edges[newOth];
                edges[oth + 1] = edges[newOth + 1];
                edges[oth + 2] = edges[newOth + 2];
                edges[i] = edges[newCur];
                edges[i + 1] = edges[newCur + 1];
                edges[i + 2] = edges[newCur + 2];

                MFace newOthF(edges[newOth].vertex, edges[newOth + 1].vertex, edges[newOth + 2].vertex);
                faces[newOthF] = newOth;

                edges.resize(edges.size() - 6);
                i -= 3;
            }
            else {
                faces[current] = i;
            }
        }

        //scan for unreferenced vertices and get rid of them
        std::set<int> referencedVerts;
        for(i = 0; i < (int)edges.size(); ++i) {
            if(edges[i].vertex < 0 || edges[i].vertex >= (int)vertices.size())
                continue;
            referencedVerts.insert(edges[i].vertex);
        }

        std::vector<int> newIdxs(vertices.size(), -1);
        int curIdx = 0;
        for(i = 0; i < (int)vertices.size(); ++i) {
            if(referencedVerts.count(i))
                newIdxs[i] = curIdx++;
        }

        for(i = 0; i < (int)edges.size(); ++i) {
            if(edges[i].vertex < 0 || edges[i].vertex >= (int)vertices.size())
                continue;
            edges[i].vertex = newIdxs[edges[i].vertex];
        }
        for(i = 0; i < (int)vertices.size(); ++i) {
            if(newIdxs[i] > 0)
                vertices[newIdxs[i]] = vertices[i];
        }
        vertices.resize(referencedVerts.size());
    }

    void Mesh::readObj(std::istream &strm) {
        int lineNum = 0;
        while (!strm.eof()) {
            ++lineNum;

            std::vector<std::string> words = readWords(strm);

            if(words.size() == 0) { // empty line
                continue;
            }

            if(words[0][0] == '#') { // comment
                continue;
            }

            // deal with the line based on the first word

            if (words[0] == "vt") { // texture coordinates
                double tu, tv;
                withTexture = true;
                sscanf(words[1].c_str(), "%lf", &tu);
                sscanf(words[2].c_str(), "%lf", &tv);

                MeshTextureCoords t(tu, tv);
                texcoords.push_back(t);

            } else if (words[0] == "vn") { // vertex normal
                if (words.size() != 4) {
                    Debugging::out() << "Error on line " << lineNum << std::endl;
                    OUT;
                }

                double nx, ny, nz;
                sscanf(words[1].c_str(), "%lf", &nx);
                sscanf(words[2].c_str(), "%lf", &ny);
                sscanf(words[3].c_str(), "%lf", &nz);

                MeshNormal n(nx, ny, nz);
                normals.push_back(n);

            } else if(words[0][0] == 'v' && words[0].size() == 1) { // geometric vertices
                if (words.size() != 4) {
                    Debugging::out() << "Error on line " << lineNum << std::endl;
                    OUT;
                }

                double x, y, z;
                sscanf(words[1].c_str(), "%lf", &x);
                sscanf(words[2].c_str(), "%lf", &y);
                sscanf(words[3].c_str(), "%lf", &z);

                MeshVertex v(x, y, z);
                vertices.push_back(v);

            } else if(words[0].size() != 1) { // unknown line
                continue;
            }

            if (words[0][0] == 'f') { // polygonal face element
                if (words.size() < 4 || words.size() > 15) {
                    Debugging::out() << "Error on line " << lineNum << std::endl;
                    OUT;
                }

                int a[16]; memset(a, 0, sizeof(a));
                int t[16]; memset(t, 0, sizeof(t));
                int n[16]; memset(n, 0, sizeof(n));

                for (int i = 0; i < (int)words.size() - 1; ++i) {
                    //~ sscanf(words[i + 1].c_str(), "%d/%d", a + i, t + i);
                    if (sscanf(words[i + 1].c_str(), "%d//%d", a + i, n + i) == 2) { /* std::cout << "Vertex + Normal" << std::endl; */ }
                    else if (sscanf(words[i + 1].c_str(), "%d/%d/%d", a + i, t + i, n + i) == 3) { /* std::cout << "Vertex + Texture + Normal" << std::endl; */ }
                    else if (sscanf(words[i + 1].c_str(), "%d/%d", a + i, t + i) == 2) { /* std::cout << "Vertex + Texture" << std::endl; */ }
                    else if (sscanf(words[i + 1].c_str(), "%d/%d", a + i, t + i) == 1) { /* std::cout << "Vertex" << std::endl; */ }
                    else { std::cout << "Unreadable vertex" << std::endl; }
                }

                //~ for (int i = 0; i < (int)words.size() - 1; ++i) {
                //~     std::cout << " Face: " << a[i] << " / " << t[i] << " / " << n[i] << std::endl;
                //~ }

                //swap(a[1], a[2]); //TODO:remove

                for (int j = 2; j < (int)words.size() - 1; ++j) {
                    int first = edges.size();
                    edges.resize(edges.size() + 3);
                    edges[first].vertex = a[0] - 1;
                    edges[first + 1].vertex = a[j - 1] - 1;
                    edges[first + 2].vertex = a[j] - 1;

                    edges[first].tvertex = t[0] - 1;
                    edges[first + 1].tvertex = t[j - 1] - 1;
                    edges[first + 2].tvertex = t[j] - 1;

                    edges[first].nvertex = n[0] - 1;
                    edges[first + 1].nvertex = n[j - 1] - 1;
                    edges[first + 2].nvertex = n[j] - 1;
                }

            }

            // otherwise continue -- unrecognized line
        }
    }

    void Mesh::writeObj(const std::string &filename) const
    {
        std::ofstream os(filename.c_str());

        for(int i = 0; i < (int)vertices.size(); ++i) {
            os << "v " << vertices[i].pos[0] << " " << vertices[i].pos[1] << " " << vertices[i].pos[2] << std::endl;
        }

        for(int i = 0; i < (int)normals.size(); ++i) {
            os << "vn " << normals[i].normal[0] << " " << normals[i].normal[1] << " " << normals[i].normal[2] << std::endl;
        }

        for(int i = 0; i < (int)texcoords.size(); ++i) {
            os << "vt " << texcoords[i].coords[0] << " " << texcoords[i].coords[1] << std::endl;
        }

        for(int i = 0; i < (int)edges.size(); i += 3) {
            os << "f " << edges[i].vertex + 1 << " " << edges[i + 1].vertex + 1 << " " << edges[i + 2].vertex + 1 << std::endl;
        }
    }

    bool Mesh::isConnected() const
    {
        if(vertices.size() == 0)
            return false;

        std::vector<bool> reached(vertices.size(), false);
        std::vector<int> todo(1, 0);
        reached[0] = true;
        unsigned int reachedCount = 1;

        int inTodo = 0;
        while(inTodo < (int)todo.size()) {
            int startEdge = vertices[todo[inTodo++]].edge;
            int curEdge = startEdge;
            do { // walk around
                curEdge = edges[edges[curEdge].prev].twin;
                int vtx = edges[curEdge].vertex;
                if(!reached[vtx]) {
                    reached[vtx] = true;
                    ++reachedCount;
                    todo.push_back(vtx);
                }
            } while(curEdge != startEdge);
        }

        return reachedCount == vertices.size();
    }

    #define CHECK(pred) { if(!(pred)) { Debugging::out() << "Mesh integrity error: " #pred << " in line " << __LINE__ << std::endl; return false; } }

    bool Mesh::integrityCheck() const
    {
        int i;
        int vs = vertices.size();
        int es = edges.size();

        // if there are no vertices, shouldn't be any edges either
        if(vs == 0) {
            CHECK(es == 0);
            return true;
        }

        // otherwise, there should be edges
        CHECK(es > 0);

        // check index range validity
        for(i = 0; i < vs; ++i) {
            CHECK(vertices[i].edge >= 0);
            CHECK(vertices[i].edge < es);
        }

        for(i = 0; i < es; ++i) {
            CHECK(edges[i].vertex >= 0 && edges[i].vertex < vs);
            CHECK(edges[i].prev >= 0 && edges[i].prev < es);
            CHECK(edges[i].twin >= 0 && edges[i].twin < es);
        }

        // check basic edge and vertex relationships
        for(i = 0; i < es; ++i) {
            CHECK(edges[i].prev != i);                         // no loops
            CHECK(edges[edges[edges[i].prev].prev].prev == i); // we have only triangles
            CHECK(edges[i].twin != i);                         // no self twins
            CHECK(edges[edges[i].twin].twin == i);             // twins are valid

            //twin's vertex and prev's vertex should be the same
            CHECK(edges[edges[i].twin].vertex == edges[edges[i].prev].vertex);
        }

        for(i = 0; i < vs; ++i) { // make sure the edge pointer is correct
            CHECK(edges[edges[vertices[i].edge].prev].vertex == i);
        }

        // check that the edges around a vertex form a cycle -- by counting
        // how many edges adjacent to each vertex
        std::vector<int> edgeCount(vs, 0);
        for(i = 0; i < es; ++i)
            edgeCount[edges[i].vertex] += 1;

        for(i = 0; i < vs; ++i) {
            int startEdge = vertices[i].edge;
            int curEdge = startEdge;
            int count = 0;
            do { // walk around
                curEdge = edges[edges[curEdge].prev].twin;
                ++count;
            } while(curEdge != startEdge && count <= edgeCount[i]);
            CHECK(count == edgeCount[i] && "Non-manifold vertex found");
        }

        return true;
    }

} // namespace Pinocchio
