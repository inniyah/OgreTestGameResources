/*
Copyright (c) 2007 Ilya Baran

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <Pinocchio/skeleton.h>
#include <Pinocchio/utils.h>
#include <Pinocchio/debugging.h>
#include <Pinocchio/attachment.h>
#include <Pinocchio/pinocchioApi.h>

#include <fstream>
#include <istream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace Pinocchio;

struct ArgData {
    ArgData() :
    stopAtMesh(false), stopAfterCircles(false), skelScale(1.), noFit(false),
        skeleton(HumanSkeleton()), stiffness(1.),
        skelOutName(""), weightOutName(""),
        objOutName(""), meshOutName("")
    {
    }

    bool stopAtMesh;
    bool stopAfterCircles;
    std::string filename;
    Quaternion<> meshTransform;
    double skelScale;
    bool noFit;
    Skeleton skeleton;
    std::string skeletonname;
    double stiffness;
    std::string skelOutName;
    std::string weightOutName;
    std::string objOutName;
    std::string meshOutName;
};

void printUsageAndExit() {
    std::cout << "Usage: AttachWeights filename.obj" << std::endl;
    std::cout << "              [-skel skelname] [-rot x y z deg]* [-scale s]" << std::endl;
    std::cout << "              [-meshonly | -mo] [-circlesonly | -co]" << std::endl;
    std::cout << "              [-fit] [-nofit] [-stiffness s]" << std::endl;
    std::cout << "              [-skelOut skelOutFile] [-weightOut weightOutFile]" << std::endl;
    std::cout << "              [-objOut objOutFile]" << std::endl;
    std::cout << "              [-meshOut meshOutFile]" << std::endl;

    exit(0);
}

ArgData processArgs(const std::vector<std::string> &args) {
    ArgData out;
    int cur = 2;
    int num = args.size();
    if (num < 2) {
        printUsageAndExit();
    }

    out.filename = args[1];

    while (cur < num) {
        std::string curStr = args[cur++];

        if (curStr == std::string("-skel")) {
            if (cur == num) {
                std::cout << "No skeleton specified; ignoring." << std::endl;
                continue;
            }

            curStr = args[cur++];

            if (curStr == std::string("human")) {
                out.skeleton = HumanSkeleton();
            } else if(curStr == std::string("horse")) {
                out.skeleton = HorseSkeleton();
            } else if(curStr == std::string("quad")) {
                out.skeleton = QuadSkeleton();
            } else if(curStr == std::string("centaur")) {
                out.skeleton = CentaurSkeleton();
            } else {
                out.skeleton = Pinocchio::CsvFileSkeleton(curStr);
            }

            out.skeletonname = curStr;
            continue;
        }

        if (curStr == std::string("-rot")) {
            if (cur + 3 >= num) {
                std::cout << "Too few rotation arguments; exiting." << std::endl;
                printUsageAndExit();
            }

            double x, y, z, deg;
            sscanf(args[cur++].c_str(), "%lf", &x);
            sscanf(args[cur++].c_str(), "%lf", &y);
            sscanf(args[cur++].c_str(), "%lf", &z);
            sscanf(args[cur++].c_str(), "%lf", &deg);

            out.meshTransform = Quaternion<>(Vector3(x, y, z), deg * M_PI / 180.) * out.meshTransform;
            continue;
        }

        if (curStr == std::string("-scale")) {
            if(cur >= num) {
                std::cout << "No scale provided; exiting." << std::endl;
                printUsageAndExit();
            }
            sscanf(args[cur++].c_str(), "%lf", &out.skelScale);
            continue;
        }

        if (curStr == std::string("-meshonly") || curStr == std::string("-mo")) {
            out.stopAtMesh = true;
            continue;
        }

        if (curStr == std::string("-circlesonly") || curStr == std::string("-co")) {
            out.stopAfterCircles = true;
            continue;
        }

        if (curStr == std::string("-fit")) {
            out.noFit = false;
            continue;
        } else if (curStr == std::string("-nofit")) {
            out.noFit = true;
            continue;
        }

        if (curStr == std::string("-stiffness")) {
            if (cur >= num) {
                std::cout << "No stiffness provided; exiting." << std::endl;
                printUsageAndExit();
            }
            sscanf(args[cur++].c_str(), "%lf", &out.stiffness);
            continue;
        }

        if (curStr == std::string("-skelOut")) {
            if(cur == num) {
                std::cout << "No skeleton output specified; ignoring." << std::endl;
                continue;
            }
            curStr = args[cur++];
            out.skelOutName = curStr;
            continue;
        }

        if (curStr == std::string("-weightOut")) {
            if (cur == num) {
                std::cout << "No weight output specified; ignoring." << std::endl;
                continue;
            }
            curStr = args[cur++];
            out.weightOutName = curStr;
            continue;
        }

        if (curStr == std::string("-objOut")) {
            if (cur == num) {
                std::cout << "No object output specified; ignoring." << std::endl;
                continue;
            }
            curStr = args[cur++];
            out.objOutName = curStr;
            continue;
        }

        if (curStr == std::string("-meshOut")) {
            if (cur == num) {
                std::cout << "No object output specified; ignoring." << std::endl;
                continue;
            }
            curStr = args[cur++];
            out.meshOutName = curStr;
            continue;
        }

        std::cout << "Unrecognized option: " << curStr << std::endl;
        printUsageAndExit();
    }

    return out;
}

void dumpObj(std::string filename, const Mesh & m, const Skeleton & skeleton, const PinocchioOutput & o) {
    std::ofstream os(filename.c_str());

    int num = 1;

    for (int i = 0; i < (int)m.edges.size(); ++i) {
        int v_id = m.edges[i].vertex;
        const Vector3 &v_pos = m.vertices[v_id].pos;
        const Vector3 &v_normal = m.vertices[v_id].normal;

        //~ const Vector3 &p2 = m.vertices[m.edges[i + 1].vertex].pos;
        //~ const Vector3 &p3 = m.vertices[m.edges[i + 2].vertex].pos;
        //~ Vector3 calc_normal = ((p2 - v_pos) % (p3 - v_pos)).normalize();

        //~ std::cout << "Mesh Point: " << v_pos << " (Normal: " <<  v_normal << ")" << std::endl;

        os << "v "  << v_pos[0] << " " << v_pos[1] << " " << v_pos[2] << std::endl;

        int vt_id = m.edges[i].tvertex;
        if (vt_id > 0) {
            const Vector2 &vt = m.texcoords[vt_id].coords;
            os << "vt " << vt[0] << " " << vt[1] << " " << std::endl;
        }

        int vn_id = m.edges[i].nvertex;
        if (vn_id > 0) {
            const Vector3 &vn = m.normals[vn_id].normal;
            os << "vn " << vn[0] << " " << vn[1] << " " << vn[2] << std::endl;
        } else {
            os << "vn " << v_normal[0] << " " << v_normal[1] << " " << v_normal[2] << std::endl;
        }

        if (num % 3 == 0) {
            os << "f " << (num-2) << "//" << (num-2)
               << " "  << (num-1) << "//" << (num-1)
               << " "  << (num)   << "//" << (num)   << std::endl;
        }
        num++;
    }

    for (int i = 1; i < (int)o.embedding.size(); ++i) {
        const Vector3 & lf = o.embedding[skeleton.fPrev()[i]];
        const Vector3 & lt = o.embedding[i];
        std::cout << "Skeleton Line Segment ("
            << skeleton.getNameForJoint(skeleton.fPrev()[i]) << " - " << skeleton.getNameForJoint(i) << "): "
            << lf << " - " << lt << std::endl;
        os << "v " << lf[0] << " " << lf[1] << " " << lf[2] << std::endl;
        os << "v " << lt[0] << " " << lt[1] << " " << lt[2] << std::endl;
        os << "l " << (num) << " " << (num+1) << std::endl;
        num+=2;
    }
}

void dumpMesh(std::string meshname, const Mesh & m, const Skeleton & skeleton, const PinocchioOutput & o) {
    size_t lastindex = meshname.find_last_of("."); 
    std::string rawname = meshname.substr(0, lastindex); 
    if (lastindex == std::string::npos) meshname = meshname + ".mesh.xml";
    std::string skelname = rawname + ".skeleton.xml"; 

    // Save Skeleton

    std::cout << "Skeleton: " << skelname << std::endl;
    std::ofstream skel_os(skelname.c_str());

    //int num_bones = o.embedding.size();

    skel_os << "<?xml version=\"1.0\"?>" << std::endl;
    skel_os << "<skeleton blendmode=\"average\">" << std::endl;

    skel_os << "	<bones>" << std::endl;
    for (int i = 0; i < (int)o.embedding.size(); ++i) {
        Vector3 bone_pos = ( i ? o.embedding[i] - o.embedding[skeleton.fPrev()[i]] : o.embedding[i] );

        skel_os << "		<bone id=\"" << i << "\" name=\"" << skeleton.getNameForJoint(i) << "\">" << std::endl;
        skel_os << "			<position x=\"" << bone_pos[0] << "\" y=\"" << bone_pos[1] << "\" z=\"" << bone_pos[2] << "\" />" << std::endl;
        skel_os << "			<rotation angle=\"0\">" << std::endl;
        skel_os << "				<axis x=\"1\" y=\"0\" z=\"0\" />" << std::endl;
        skel_os << "			</rotation>" << std::endl;
        skel_os << "		</bone>" << std::endl;
    }
    skel_os << "	</bones>" << std::endl;

    skel_os << "	<bonehierarchy>" << std::endl;
    for (int i = 1; i < (int)o.embedding.size(); ++i) {
        skel_os << "		<boneparent bone=\"" << skeleton.getNameForJoint(i) << "\" parent=\"" << skeleton.getNameForJoint(skeleton.fPrev()[i]) << "\" />" << std::endl;
    }
    skel_os << "	</bonehierarchy>" << std::endl;

    skel_os << "	<animations>" << std::endl;
    skel_os << "		<animation name=\"Test\" length=\"1\">" << std::endl;
    skel_os << "			<tracks>" << std::endl;

    for (int i = 0; i < (int)o.embedding.size(); ++i) {
        const std::string & bone_name = skeleton.getNameForJoint(i);

        skel_os << "				<track bone=\"" << bone_name << "\">" << std::endl;
        skel_os << "					<keyframes>" << std::endl;
        skel_os << "						<keyframe time=\"0\">" << std::endl;
        skel_os << "							<translate x=\"0\" y=\"0\" z=\"0\" />" << std::endl;
        skel_os << "							<rotate angle=\"0\">" << std::endl;
        skel_os << "								<axis x=\"1\" y=\"0\" z=\"0\" />" << std::endl;
        skel_os << "							</rotate>" << std::endl;
        skel_os << "						</keyframe>" << std::endl;
        skel_os << "						<keyframe time=\"1\">" << std::endl;
        skel_os << "							<translate x=\"0.1\" y=\"0.1\" z=\"0.1\" />" << std::endl;
        skel_os << "							<rotate angle=\"0\">" << std::endl;
        skel_os << "								<axis x=\"1\" y=\"0\" z=\"0\" />" << std::endl;
        skel_os << "							</rotate>" << std::endl;
        skel_os << "						</keyframe>" << std::endl;
        skel_os << "					</keyframes>" << std::endl;
        skel_os << "				</track>" << std::endl;
    }

    skel_os << "			</tracks>" << std::endl;
    skel_os << "		</animation>" << std::endl;
    skel_os << "	</animations>" << std::endl;

    skel_os << "</skeleton>" << std::endl;

    // Save Mesh

    std::cout << "Mesh: " << meshname << std::endl;
    std::ofstream mesh_os(meshname.c_str());

    mesh_os << "<?xml version=\"1.0\"?>" << std::endl;
    mesh_os << "<mesh>" << std::endl;
    mesh_os << "	<submeshes>" << std::endl;

    mesh_os << "		<submesh material=\"" << "Material" << "\" usesharedvertices=\"false\" use32bitindexes=\"false\" operationtype=\"triangle_list\">" << std::endl;

    int num_faces = m.edges.size() / 3;

    mesh_os << "			<faces count=\"" << num_faces << "\">" << std::endl;
    for (int f_id = 0; f_id < num_faces; f_id++) {
        mesh_os << "				<face v1=\"" << m.edges[3*f_id].vertex
                                   << "\" v2=\"" << m.edges[3*f_id + 1].vertex
                                   << "\" v3=\"" << m.edges[3*f_id + 2].vertex << "\" />" << std::endl;
    }
    mesh_os << "			</faces>" << std::endl;

    int num_vertex = m.vertices.size();

    mesh_os << "			<geometry vertexcount=\"" << num_vertex << "\">" << std::endl;

    mesh_os << "				<vertexbuffer positions=\"true\" normals=\"true\">" << std::endl;
    for (int v_id = 0; v_id < num_vertex; v_id++) {
        const Vector3 &v_pos = m.vertices[v_id].pos;
        const Vector3 &v_normal = m.vertices[v_id].normal;

        mesh_os << "					<vertex>" << std::endl;
        mesh_os << "						<position x=\"" << v_pos[0] << "\" y=\"" << v_pos[1] << "\" z=\"" << v_pos[2] << "\" />" << std::endl;
        mesh_os << "						<normal x=\"" << v_normal[0] << "\" y=\"" << v_normal[1] << "\" z=\"" << v_normal[2] << "\" />" << std::endl;
        mesh_os << "					</vertex>" << std::endl;
    }
    mesh_os << "				</vertexbuffer>" << std::endl;

    mesh_os << "				<vertexbuffer texture_coord_dimensions_0=\"float2\" texture_coords=\"1\">" << std::endl;
    for (int i = 0; i < num_vertex; i++) {
        int vt_id = m.edges[i].tvertex;
        mesh_os << "					<vertex>" << std::endl;
        if (vt_id > 0) {
            const Vector2 &vt = m.texcoords[vt_id].coords;
            mesh_os << "						<texcoord u=\"" << vt[0] << "\" v=\"" << vt[1] << "\" />" << std::endl;
        } else {
            mesh_os << "						<texcoord u=\"" << 0 << "\" v=\"" << 0 << "\" />" << std::endl;
        }
        mesh_os << "					</vertex>" << std::endl;
    }
    mesh_os << "				</vertexbuffer>" << std::endl;

    mesh_os << "			</geometry>" << std::endl;


    mesh_os << "			<boneassignments>" << std::endl;
    for (int vertex = 0; vertex < (int)m.vertices.size(); ++vertex) {
        std::vector<std::pair<double, int>> list;

        Vector<double, -1> weights = o.attachment->getWeights(vertex);

        for (int j = 0; j < weights.size(); ++j) {
            double d = floor(0.5 + weights[j] * 10000.) / 10000.;
            if (d >= 0.02) {
                list.push_back( std::pair<double, int>(d, j));
            }
        }

        std::sort(list.rbegin(), list.rend());

        double sum = 0;
        unsigned int count = 0;
        for (unsigned int i = 0; i < list.size() && count < 4; i++) {
            if (list[i].first > sum * 0.08) {
                sum += list[i].first;
                count++;
            }
        }

        for (unsigned int i = 0; i < count; i++) {
            int bone_index = 1 + list[i].second ;
            double weight = list[i].first / sum;
            mesh_os << "				<vertexboneassignment vertexindex=\"" << vertex << "\" boneindex=\"" << bone_index << "\" weight=\"" << weight << "\" />" << std::endl;
        }
    }
    mesh_os << "			</boneassignments>" << std::endl;

    mesh_os << "		</submesh>" << std::endl;

    mesh_os << "	</submeshes>" << std::endl;
    mesh_os << "	<skeletonlink name=\"" << rawname << ".skeleton" << "\" />" << std::endl;
    mesh_os << "</mesh>" << std::endl;
}

int process(const std::vector<std::string> &args) {
    ArgData a = processArgs(args);

    Debugging::setOutStream(std::cout);

    Mesh m(a.filename);

    if (m.vertices.size() == 0) {
        std::cout << "Error reading file.  Aborting." << std::endl;
        exit(0);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < (int)m.vertices.size(); ++i) {
        m.vertices[i].pos = a.meshTransform * m.vertices[i].pos;
    }

    m.normalizeBoundingBox();
    m.computeVertexNormals();

    Skeleton given = a.skeleton;
    given.scale(a.skelScale * 0.7);

    if (a.stopAtMesh) { // if early bailout
        return EXIT_FAILURE;
    }

    PinocchioOutput o;
    // do everything
    if (!a.noFit) {
        o = autorig(given, m);
    } else { // skip the fitting step--assume the skeleton is already correct for the mesh
        TreeType *distanceField = constructDistanceField(m);
        VisTester<TreeType> *tester = new VisTester<TreeType>(distanceField);

        // Add offset to skeleton
        o.embedding = a.skeleton.fGraph().verts;
        for (int i = 0; i < (int)o.embedding.size(); ++i) {
            o.embedding[i] = m.toAdd + o.embedding[i] * m.scale;
        }

        o.attachment = new Attachment(m, a.skeleton, o.embedding, tester, a.stiffness);

        delete tester;
        delete distanceField;
    }

    if (o.embedding.size() == 0) {
        std::cout << "Error embedding" << std::endl;
        return EXIT_FAILURE;
    }

    //~ a.skeleton.dump();

    // output model
    if (a.objOutName.length()) {
        //~ m.writeObj(a.objOutName);
        dumpObj(a.objOutName, m, given, o);
    }

    if (a.meshOutName.length()) {
        //~ m.writeObj(a.objOutName);
        dumpMesh(a.meshOutName, m, given, o);
    }

    // Remove offset from skeleton
    for (int joint = 0; joint < (int)o.embedding.size(); ++joint) {
        o.embedding[joint] = (o.embedding[joint] - m.toAdd) / m.scale;
    }

    // output skeleton embedding
    if (a.skelOutName.length()) {
        std::ofstream os(a.skelOutName.c_str());

        os << "\"BoneNum\",\"Bone\",\"ParentNum\",\"Parent\",\"X\",\"Y\",\"Z\"" << std::endl;

        for (int joint = 0; joint < (int)o.embedding.size(); ++joint) {
            int parent = a.skeleton.fPrev()[joint];
            os << joint << ",\"" << a.skeleton.getNameForJoint(joint) << "\","
                << parent << ",\"" << a.skeleton.getNameForJoint(parent) << "\","
                << o.embedding[joint][0] << ","
                << o.embedding[joint][1] << ","
                << o.embedding[joint][2] << std::endl;
        }
    }

    // output attachment
    if (a.weightOutName.length()) {
        std::ofstream astrm(a.weightOutName.c_str());

        for (int joint = 1; joint < (int)o.embedding.size(); ++joint) {
            if (joint > 1) astrm << ",";
            astrm << "\"" << a.skeleton.getNameForJoint(joint) << "\"";
        }
        astrm << std::endl;

        for (int vertex = 0; vertex < (int)m.vertices.size(); ++vertex) {
            Vector<double, -1> weights = o.attachment->getWeights(vertex);
            for (int j = 0; j < weights.size(); ++j) {
                if (j) astrm << ",";
                double d = floor(0.5 + weights[j] * 10000.) / 10000.;
                astrm << d;
            }
            astrm << std::endl;
        }
    }

    delete o.attachment;

    return EXIT_SUCCESS;
}


int main (int argc, const char * const * argv, const char * const * envp) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return process(args);
}
