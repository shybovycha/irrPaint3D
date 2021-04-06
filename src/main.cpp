/***************

The original LSCM algorithm for growing charts :

-----------------------

EdgeHeap : heap of directed edges (with associated face F(edge))
    sorted by dist(F(edge)) (dist is distance to feature)

mark all edges as "chart boundaries"
track is_boundary(edge)

track chart(face) , tells you which chart a face is in

EdgeHeap contains all edge to consider growing a chart from
start with EdgeHeap clear

Seed charts :
{
    chart[i] = seed face
    push edges of seed face to EdgeHeap
}
// each connected area must have at least one seed
// each connected area that is not a disk must have at least *two* seeds

// grow charts :
while EdgeHeap not empty :
{
    edge = EdgeHeap.pop
    // assert is_boundary(edge)
    face = F(edge) // is the face to consider adding
    prev = F(edge flipped) // is the face growing off of
    // assert chart(prev) != none

    if ( chart(face) = none )
    {
        add face to chart(prev)
        set is_boundary(edge) = false
        consider edges of "face" and "prev" ; any edge which is not connected to two other
            is_boundary() edges, mark as not being is_boundary() either
        push edges of face to EdgeHeap (if they are is_boundary() true)
    }
    else if ( chart(face) != chart(prev) )
    {
        // consider merging the charts

    }
}

*******************/
/*
====== LSCM ======

Let's perform LCSM (Least Square Conformal Maps) algorithm!

First of all, here's the algorithm:
    * split entire model to charts
    * unwrap charts
    * pack charts to one map

More detailed algorithm description:

===== split entire model into charts =====

    * detect features defining chart boundaries
    * grow features (thin them)
    * expand charts (create them)
    * validate charts (optional; overlaps)

=====  unwrap charts (LSCM itself) =====
<latex>X = (A^{T} A)^{-1} A^{T} b</latex>.

<latex>\text{x}</latex> is a vector of <latex>(u, v)</latex> pairs. It is our goal!

<latex>A = \begin{pmatrix}M^{1}_{f} & -M^{2}_{f}\\ M^{2}_{f} & M^{1}_{f}\\ \end{pmatrix}</latex>

<latex>\text{M}</latex> is a matrix (see below) of complex numbers. It is split into two parts :

<latex>M \Leftrightarrow \begin{pmatrix} M_{f} & M_{p} \end{pmatrix} \Leftrightarrow \begin{pmatrix} M_{n' \times (n - p)} & M_{n' \times p} \end{pmatrix}</latex> \\
<latex>p = 2</latex> \\
<latex>n' = triangleCount</latex> \\
<latex>n = vertexCount</latex> \\

<latex>M \Leftrightarrow \begin{pmatrix} M_{n' \times (n - 2) & M_{n' \times 2}} \end{pmatrix}</latex>

Up-indices //1// and //2// stand for //real// and //imaginary// parts of complex numbers, respectively.

<latex>b = \begin{pmatrix}M^{1}_{p} & -M^{2}_{p}\\ M^{2}_{p} & M^{1}_{p}\\ \end{pmatrix} \begin{pmatrix}U^{1}_{p} \\ U^{2}_{p}\\ \end{pmatrix}</latex>

<latex>M = (m_{i,j})</latex> \\ <latex>m_{i,j} = \begin{cases} \frac{W_{j, T_{i}}}{\sqrt{d_{T_{i}}}} & \text{ if Vert[j] } \in \triangle T_{i} \\ 0 \end{cases}</latex>

Here, <latex>d_{T_{i}}</latex> is the doubled square of <latex>\triangle T_{i}</latex>:

<latex>d_{T_{i}} = 2S_{\triangle T_{i}}</latex>

<latex>\begin{cases} W_{j,1} = (x_{3} - x_{2}) + i(y_{3} - y_{2}) \\ W_{j,2} = (x_{1} - x_{3}) + i(y_{1} - y_{3}) \\ W_{j,3} = (x_{2} - x_{1}) + i(y_{2} - y_{1}) \\ \end{cases}</latex>

So, just calculate the right index for i (<latex>W_{j,i}</latex>, in the first formula, <latex>i = \sqrt{-1}</latex>) and use either <latex>W_{j,1}</latex>, <latex>W_{j,2}</latex> or <latex>W_{j,3}</latex>

<latex>U_{p}</latex> is a vector of <latex>u + iv</latex> complex numbers determining UV-coordinates for //pinned// vertices (sub-index of //p//)
*/

#include <irrlicht.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class HalfEdge {
public:
    HalfEdge(irr::u32 e1, irr::u32 e2, irr::u32 v1, irr::u32 v2, irr::f32 w = 0.f) {
        this->e1 = e1;
        this->e2 = e2;
        this->v1 = v1;
        this->v2 = v2;
        this->w = w;
    }

    void setPositions(irr::core::vector3df e1, irr::core::vector3df e2, irr::core::vector3df v1, irr::core::vector3df v2) {
        this->pe1 = e1;
        this->pe2 = e2;
        this->pv1 = v1;
        this->pv2 = v2;
    }

public:
    irr::u32 e1, e2; // edge vertices
    irr::u32 v1, v2; // side vertices
    irr::f32 w; // weight
    irr::core::vector3df pe1, pe2, pv1, pv2;
};

class Triangle {
public:
    Triangle(irr::u32 a, irr::u32 b, irr::u32 c, irr::u16 meshBuffer = 0, irr::u16 feature = -1) {
        vertices = std::vector<irr::u32>();
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        featureId = feature;
        meshBufferId = meshBuffer;
    }

    void setPositions(irr::core::vector3df a, irr::core::vector3df b, irr::core::vector3df c) {
        positions.push_back(a);
        positions.push_back(b);
        positions.push_back(c);
    }

    std::vector<irr::u32> commonVertices(Triangle& tri) {
        std::vector<irr::u32> res;

        for (int i = 0; i < 3; i++) {
            for (int t = 0; t < 3; t++) {
                if (vertices[i] == tri.vertices[t]) {
                    res.push_back(vertices[i]);
                }
            }
        }

        return res;
    }

    void assignFeatureId(irr::s16 f) {
        if (featureId < 0) {
            featureId = f;
        }
    }

public:
    std::vector<irr::u32> vertices;
    std::vector<irr::core::vector3df> positions;
    std::vector<irr::core::vector2di> UVs;

    irr::s16 featureId;
    irr::u16 meshBufferId;
};

irr::core::vector2di findCoVerticesForEdge(irr::u16 e1, irr::u16 e2, irr::u16* indices, irr::u16 indicesCnt, irr::video::S3DVertex* vertices, irr::f32 threshold = 0.001f) {
    irr::s16 V[] = { -1, -1 };
    irr::s16 cnt = 0;

    for (irr::u16 v = 0; v < indicesCnt; v += 3) {
        irr::u32 a = indices[v];
        irr::u32 b = indices[v + 1];
        irr::u32 c = indices[v + 2];

        /*if (a == e1 && c == e2)
            V[cnt++] = b; else
        if (a == e1 && b == e2)
            V[cnt++] = c; else
        if (a == e2 && b == e1)
            V[cnt++] = c; else
        if (b == e1 && c == e2)
            V[cnt++] = a; else
        if (b == e2 && c == e1)
            V[cnt++] = a; else
        if (a == e2 && c == e1)
            V[cnt++] = b; else*/

        if ((vertices[a].Pos - vertices[e1].Pos).getLength() < threshold &&
            (vertices[c].Pos - vertices[e2].Pos).getLength() < threshold) {

            V[cnt++] = b;
        } else
        if ((vertices[a].Pos - vertices[e1].Pos).getLength() < threshold &&
            (vertices[b].Pos - vertices[e2].Pos).getLength() < threshold) {

            V[cnt++] = c;
        } else
        if ((vertices[a].Pos - vertices[e2].Pos).getLength() < threshold &&
            (vertices[b].Pos - vertices[e1].Pos).getLength() < threshold) {

            V[cnt++] = c;
        } else
        if ((vertices[b].Pos - vertices[e1].Pos).getLength() < threshold &&
            (vertices[c].Pos - vertices[e2].Pos).getLength() < threshold) {

            V[cnt++] = a;
        } else
        if ((vertices[b].Pos - vertices[e2].Pos).getLength() < threshold &&
            (vertices[c].Pos - vertices[e1].Pos).getLength() < threshold) {

            V[cnt++] = a;
        } else
        if ((vertices[a].Pos - vertices[e2].Pos).getLength() < threshold &&
            (vertices[c].Pos - vertices[e1].Pos).getLength() < threshold) {

            V[cnt++] = b;
        }

        if (cnt > 1) {
            break;
        }
    }

    return irr::core::vector2di(V[0], V[1]);
}

HalfEdge* createHaldEdge(irr::u32 e1, irr::u32 e2, irr::u16* indices, irr::u32 indexCount, irr::video::S3DVertex* vertices) {
    irr::core::vector2di coVerts = findCoVerticesForEdge(e1, e2, indices, indexCount, vertices);

    if (coVerts.Y < 0) {
        coVerts.Y = coVerts.X;
    } else if (coVerts.X < 0) {
        return NULL;
    }

    irr::u32 v1 = coVerts.X;
    irr::u32 v2 = coVerts.Y;

    irr::core::vector3df a1 = (vertices[v1].Pos - vertices[e1].Pos);
    irr::core::vector3df a2 = (vertices[v2].Pos - vertices[e1].Pos);

    irr::core::vector3df edge = (vertices[e2].Pos - vertices[e1].Pos);

    irr::core::vector3df n1 = a1.crossProduct(edge);
    irr::core::vector3df n2 = a2.crossProduct(edge);

    irr::f32 w = n1.dotProduct(n2) / (n1.getLength() * n2.getLength());

    HalfEdge* he = new HalfEdge(indices[e1], indices[e2], v1, v2, w);

    he->setPositions(vertices[e1].Pos, vertices[e2].Pos, vertices[v1].Pos, vertices[v2].Pos);

    return he;
}

std::vector<HalfEdge*> fillHalfEdges(irr::scene::IMesh* mesh) {
    std::vector<HalfEdge*> halfEdges;

    irr::u16 meshBufferCount = mesh->getMeshBufferCount();

    for (irr::u16 i = 0; i < meshBufferCount; ++i) {
        irr::scene::IMeshBuffer* mb = mesh->getMeshBuffer(i);

        irr::u16* indices = mb->getIndices();
        irr::u32 indexCount = mb->getIndexCount();

        irr::video::S3DVertex* vertices = (irr::video::S3DVertex*) mb->getVertices();

        for (irr::u32 e = 0; e < indexCount; e += 3) {
            {
                irr::u32 e1 = indices[e];
                irr::u32 e2 = indices[e + 1];

                HalfEdge* he = createHaldEdge(e1, e2, indices, indexCount, vertices);

                if (!he) {
                    return std::vector<HalfEdge*>();
                }

                halfEdges.push_back(he);
            }

            {
                irr::u32 e1 = indices[e + 1];
                irr::u32 e2 = indices[e + 2];

                HalfEdge* he = createHaldEdge(e1, e2, indices, indexCount, vertices);

                if (!he) {
                    return std::vector<HalfEdge*>();
                }

                halfEdges.push_back(he);
            }

            {
                irr::u32 e1 = indices[e + 2];
                irr::u32 e2 = indices[e];

                HalfEdge* he = createHaldEdge(e1, e2, indices, indexCount, vertices);

                if (!he) {
                    return std::vector<HalfEdge*>();
                }

                halfEdges.push_back(he);
            }
        }
    }

    return halfEdges;
}

std::vector<HalfEdge*> detectSeams(std::vector<HalfEdge*> halfEdges, irr::f32 Bu = 0.995f, irr::f32 Bl = 0.92f) {
    std::vector<HalfEdge*> paths;
    HalfEdge* selected = 0;
    std::vector< std::vector<irr::video::S3DVertex*> > features;

    for (irr::u32 i = 0; i < halfEdges.size(); ++i) {
        HalfEdge* he = halfEdges[i];

        if (he->w >= Bu) {
            selected = he;
            paths.push_back(he);
        }

        if (he->w >= Bl) {
            if (selected && ((selected->e1 == he->e1) || (selected->e1 == he->e2) || (selected->e2 == he->e1) || (selected->e2 == he->e2))) {
                selected = he;
                paths.push_back(he);
            }
        }
    }

    return paths;
}

std::vector< std::vector<Triangle> > growFeatures(irr::scene::IMesh* mesh) {
    std::vector<HalfEdge*> halfEdges = fillHalfEdges(mesh);
    std::vector<HalfEdge*> paths = detectSeams(halfEdges);

    std::vector< std::vector<irr::u16> > connectedTrianglesList;

    std::vector<Triangle> triangles;
    std::vector< std::vector<Triangle> > features;

    irr::u16 meshBufferCount = mesh->getMeshBufferCount();

    for (irr::u16 i = 0; i < meshBufferCount; ++i) {
        irr::scene::IMeshBuffer* mb = mesh->getMeshBuffer(i);

        irr::u16* indices = mb->getIndices();
        irr::u32 indexCount = mb->getIndexCount();

        irr::video::S3DVertex* vertices = (irr::video::S3DVertex*) mb->getVertices();

        for (irr::u32 t = 0; t < indexCount; t += 3) {
            // building connected triangles list with just one enhancement
            // if two triangles do have common edge and that edge belongs to seams
            // then each triangle is assigned new feature ID.
            // NOTE: if triangle has feature ID assigned already - it does not get new one. ever.
            irr::video::S3DVertex a = vertices[indices[t]], b = vertices[indices[t + 1]], c = vertices[indices[t + 2]];

            Triangle triangle(indices[t], indices[t + 1], indices[t + 2], i);

            triangle.setPositions(a.Pos, b.Pos, c.Pos);

            // u16 triangleIndex = triangles.size();

            connectedTrianglesList.push_back(std::vector<irr::u16>());

            triangles.push_back(triangle);
        }
    }

    irr::u16 T = triangles.size();
    irr::u16 P = paths.size();

    std::cout << "Generated " << T << " triangles\n";

    for (irr::u16 t1 = 0; t1 < T; ++t1) {
        for (irr::u16 t2 = 0; t2 < T; ++t2) {
            if (t1 == t2) {
                continue;
            }

            std::vector<irr::u32> c = triangles[t1].commonVertices(triangles[t2]);

            if (c.size() > 1) {
                connectedTrianglesList[t1].push_back(t2);
            }
        }
    }

    // printf("Connected list size: %d\n", connectedTrianglesList.size());

    for (irr::u16 t1 = 0; t1 < connectedTrianglesList.size(); ++t1) {
        for (irr::u16 t2 = 0; t2 < connectedTrianglesList[t1].size(); ++t2) {
            irr::u16 ti1 = t1, ti2 = connectedTrianglesList[t1][t2];

            std::vector<irr::u32> c = triangles[ti1].commonVertices(triangles[ti2]);

            int featureFound = 0;

            for (irr::u16 j = 0; j < P; ++j) {
                if (paths[j]->e1 == c[0] || paths[j]->e2 == c[0] || paths[j]->e1 == c[1] || paths[j]->e2 == c[2]) {
                    featureFound = 1;

                    if (triangles[ti1].featureId < 0) {
                        std::vector<Triangle> f;
                        f.push_back(triangles[ti1]);
                        features.push_back(f);
                        triangles[ti1].assignFeatureId(features.size() - 1);
                    }

                    if (triangles[ti2].featureId < 0) {
                        std::vector<Triangle> f;
                        f.push_back(triangles[ti2]);
                        features.push_back(f);
                        triangles[ti2].assignFeatureId(features.size() - 1);
                    }

                    break;
                }
            }

            if (featureFound) {
                continue;
            }

            if (triangles[ti1].featureId < 0) {
                std::vector<Triangle> f;
                f.push_back(triangles[ti1]);
                features.push_back(f);
                triangles[ti1].assignFeatureId(features.size() - 1);
            }

            if (triangles[ti2].featureId < 0) {
                features[triangles[ti1].featureId].push_back(triangles[ti2]);
                triangles[ti2].assignFeatureId(triangles[ti1].featureId);
            }
        }
    }

    return features;
}

void renderFeaturesToFiles(std::vector< std::vector<Triangle> > features) {
    char* filename = new char[255];

    for (int i = 0; i < features.size(); ++i) {
        std::string filename;
        std::ostringstream os;

        os << "tmp_" << i << ".obj";

        std::ofstream f(os.str());

        std::vector<irr::u16> indices;
        irr::u32 indexCount = 0;

        for (int t = 0; t < features[i].size(); ++t) {
            Triangle tri = features[i][t];

            f << "v " << tri.positions[0].X << tri.positions[0].Y << tri.positions[0].Z << "\n";
            f << "v " << tri.positions[1].X << tri.positions[1].Y << tri.positions[1].Z << "\n";
            f << "v " << tri.positions[2].X << tri.positions[2].Y << tri.positions[2].Z << "\n";

            indices.push_back(indexCount);
            indices.push_back(indexCount + 1);
            indices.push_back(indexCount + 2);

            indexCount += 3;
        }

        for (int t = 0; t < indices.size(); t += 3) {
            f << "f " << indices[t] << "//" << indices[t + 1] << "//" << indices[t + 2] << "//\n";
        }

        f.close();
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Please, run " << argv[0] << " <3D model file> <texture base filename>\n";

        return 0;
    }

    irr::IrrlichtDevice *device =
            irr::createDevice(irr::video::EDT_BURNINGSVIDEO, irr::core::dimension2d<irr::u32>(1024, 768), 32,
                    false, false, true, 0);

    if (!device) {
        std::cerr << "Could not initialize video device\n";

        return 1;
    }

    device->setWindowCaption(L"Hello World! - Irrlicht Engine Demo");

    irr::video::IVideoDriver* driver = device->getVideoDriver();
    irr::scene::ISceneManager* smgr = device->getSceneManager();
    irr::gui::IGUIEnvironment* guienv = device->getGUIEnvironment();

    irr::scene::IAnimatedMesh* modelMesh = smgr->getMesh(argv[1]);

    if (!modelMesh) {
        std::cerr << "Could not read model file " << argv[1] << "\n";

        device->drop();

        return 1;
    }

    irr::scene::IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(modelMesh);

    //smgr->addLightSceneNode(0, vector3df(0, 100, 0), SColorf(100.f, 100.f, 100.f));

    // if (node)
    // {
    //     node->setMaterialFlag(EMF_LIGHTING, false);
    //     node->setAnimationSpeed(0);
    //     node->setMaterialTexture(0, driver->getTexture("dwarf.jpg"));
    //     //node->setMaterialTexture( 0, driver->getTexture("../../media/Sovereign_1.jpg") );
    //     //node->setScale(vector3df(0.f, 0.f, 0.f));
    // }

    node->setScale(irr::core::vector3df(10.f, 10.f, 10.f));

    irr::scene::IMesh* mesh = modelMesh->getMesh(0);
    std::vector<HalfEdge*> halfEdges = fillHalfEdges(mesh);

    std::vector<HalfEdge*> seams = detectSeams(halfEdges);
    std::vector< std::vector<Triangle> > features = growFeatures(mesh);

    smgr->addCameraSceneNodeFPS(0, 50.f, 0.0125f);
    smgr->getActiveCamera()->setNearValue(0.01);

    irr::u32 edgeCount = 0;

    for (irr::u16 i = 0; i < modelMesh->getMeshBufferCount(); ++i) {
        edgeCount += modelMesh->getMeshBuffer(i)->getIndexCount();
    }

    std::cout << "Seams: " << seams.size() << "\n";
    std::cout << "Edge count: " << edgeCount << "\n";
    std::cout << "Feature count: " << features.size() << "\n";

    renderFeaturesToFiles(features);

    // node->setVisible(false);

    // while (device->run())
    // {
    //     if (!device->isWindowActive() || !device->isWindowFocused() || device->isWindowMinimized())
    //         continue;

    //     driver->beginScene(true, true, SColor(255,100,101,140));

    //     smgr->drawAll();
    //     guienv->drawAll();

    //     driver->setTransform(video::ETS_WORLD, matrix4::EM4CONST_IDENTITY);

    //     /*for (u16 i = 0; i < seams.size(); i++)
    //     {
    //         driver->draw3DLine(seams[i]->pe1 * 1.005, seams[i]->pe2 * 1.005, SColor(55, 100, 255, 140));
    //     }*/

    //     for (u16 i = 0; i < features.size(); i++)
    //     {
    //         for (u16 t = 0; t < features[i].size(); t++)
    //         {
    //             Triangle tri = features[i][t];

    //             driver->draw3DLine(tri.positions[0] * 1.005, tri.positions[1] * 1.005, SColor(55, 100, 255, 140));
    //             driver->draw3DLine(tri.positions[0] * 1.005, tri.positions[2] * 1.005, SColor(55, 100, 255, 140));
    //             driver->draw3DLine(tri.positions[1] * 1.005, tri.positions[2] * 1.005, SColor(55, 100, 255, 140));
    //         }
    //     }

    //     driver->endScene();
    // }

    device->drop();

    return 0;
}
