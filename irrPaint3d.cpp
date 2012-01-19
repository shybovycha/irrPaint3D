/*
====== LSCM ======

Let's perform LCSM (Least Square Conformal Maps) algorithm!

First of all, here's the algorithm:
    * split entire model to charts
    * unwrap charts
    * pack charts to one map

More detailed algorithm description:

===== split entire model to charts =====
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

#ifdef _WIN32
#include <irrlicht.h>
#else
#include <irrlicht/irrlicht.h>
#endif

using namespace irr;

using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

struct HalfEdge
{
    u32 e1, e2; // edge vertices
    u32 v1, v2; // side vertices
    f32 w; // weight
    vector3df pe1, pe2, pv1, pv2;
    
    HalfEdge(u32 _e1, u32 _e2, u32 _v1, u32 _v2, f32 _w = 0.f)
    {
        e1 = _e1;
        e2 = _e2;
        v1 = _v1;
        v2 = _v2;
        w = _w;
    }

    void setPositions(vector3df _e1, vector3df _e2, vector3df _v1, vector3df _v2)
    {
        pe1 = _e1;
        pe2 = _e2;
        pv1 = _v1;
        pv2 = _v2;
    }
};

vector2di findCoVerticesForEdge(u16 e1, u16 e2, u16* indices, u16 indicesCnt)
{
	s16 V[] = { -1, -1 }, cnt = 0;

    for (u16 v = 0; v < indicesCnt; v += 3)
    {
		u32 a = indices[v], b = indices[v + 1], c = indices[v + 2];

		if (a == e1 && c == e2)
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
			V[cnt++] = b;

        if (cnt > 1)
            break;
    }

    return vector2di(V[0], V[1]);
}

int main()
{
    IrrlichtDevice *device =
            createDevice( video::EDT_OPENGL, dimension2d<u32>(640, 480), 16,
                    false, false, false, 0);

    if (!device)
        return 1;

    device->setWindowCaption(L"Hello World! - Irrlicht Engine Demo");

    IVideoDriver* driver = device->getVideoDriver();
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();

    guienv->addStaticText(L"Hello World! This is the Irrlicht Software renderer!",
            rect<s32>(10,10,260,22), true);

    IAnimatedMesh* modelMesh = smgr->getMesh("../../media/dwarf.x");

    if (!modelMesh)
    {
        device->drop();

        return 1;
    }

    IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode( modelMesh );

    if (node)
    {
        node->setMaterialFlag(EMF_LIGHTING, false);
        node->setAnimationSpeed(0);
        node->setMaterialTexture( 0, driver->getTexture("../../media/dwarf.jpg") );
    }

	IMesh* mesh = modelMesh->getMesh(0);
    u16 meshBufferCount = mesh->getMeshBufferCount();
    array<HalfEdge*> halfEdges;

    // create source data for feature detection - form half-edge list

    for (u16 i = 0; i < meshBufferCount; i++)
    {
        IMeshBuffer* mb = mesh->getMeshBuffer(i);

        u16* indices = mb->getIndices();
		u32 indexCount = mb->getIndexCount();

		S3DVertex* vertices = (S3DVertex*) mb->getVertices();

        for (u32 e = 0; e < indexCount; e += 3)
        {
            {
                u32 e1 = indices[e], e2 = indices[e + 1];

                vector2di coVerts = findCoVerticesForEdge(e1, e2, indices, indexCount);

                if (coVerts.Y < 0)
                    coVerts.Y = coVerts.X; else
				if (coVerts.X < 0)
					return 1;

                u32 v1 = coVerts.X, v2 = coVerts.Y;

                vector3df a1 = (vertices[v1].Pos - vertices[e1].Pos), a2 = (vertices[v2].Pos - vertices[e1].Pos), edge = (vertices[e2].Pos - vertices[e1].Pos);
                vector3df n1 = a1.crossProduct(edge), n2 = a2.crossProduct(edge);
                f32 w = n1.dotProduct(n2) / (n1.getLength() * n2.getLength());

                HalfEdge* he = new HalfEdge(indices[e1], indices[e2], v1, v2, w);

                he->setPositions(vertices[e1].Pos, vertices[e2].Pos, vertices[v1].Pos, vertices[v2].Pos);

                halfEdges.push_back(he);
            }

            {
                u32 e1 = indices[e + 1], e2 = indices[e + 2];

                vector2di coVerts = findCoVerticesForEdge(e1, e2, indices, indexCount);

                if (coVerts.Y < 0)
                    coVerts.Y = coVerts.X; else
				if (coVerts.X < 0)
					return 1;

                u32 v1 = coVerts.X, v2 = coVerts.Y;

                vector3df a1 = (vertices[v1].Pos - vertices[e1].Pos), a2 = (vertices[v2].Pos - vertices[e1].Pos), edge = (vertices[e2].Pos - vertices[e1].Pos);
                vector3df n1 = a1.crossProduct(edge), n2 = a2.crossProduct(edge);
                f32 w = n1.dotProduct(n2) / (n1.getLength() * n2.getLength());

                HalfEdge* he = new HalfEdge(indices[e1], indices[e2], v1, v2, w);

                he->setPositions(vertices[e1].Pos, vertices[e2].Pos, vertices[v1].Pos, vertices[v2].Pos);

                halfEdges.push_back(he);
            }

            {
                u32 e1 = indices[e + 2], e2 = indices[e];

                vector2di coVerts = findCoVerticesForEdge(e1, e2, indices, indexCount);

                if (coVerts.Y < 0)
                    coVerts.Y = coVerts.X; else
				if (coVerts.X < 0)
					return 1;

                u32 v1 = coVerts.X, v2 = coVerts.Y;

                vector3df a1 = (vertices[v1].Pos - vertices[e1].Pos), a2 = (vertices[v2].Pos - vertices[e1].Pos), edge = (vertices[e2].Pos - vertices[e1].Pos);
                vector3df n1 = a1.crossProduct(edge), n2 = a2.crossProduct(edge);
                f32 w = n1.dotProduct(n2) / (n1.getLength() * n2.getLength());

                HalfEdge* he = new HalfEdge(indices[e1], indices[e2], v1, v2, w);

                he->setPositions(vertices[e1].Pos, vertices[e2].Pos, vertices[v1].Pos, vertices[v2].Pos);

                halfEdges.push_back(he);
            }
        }
    }

    array<HalfEdge*> paths;
    HalfEdge* selected = 0;
    f32 Bu = 0.96f, Bl = 0.92f;

    // detect features
    for (u32 i = 0; i < halfEdges.size(); i++)
    {
        HalfEdge* he = halfEdges[i];

        if (he->w >= Bu)
        {
            selected = he;
            paths.push_back(he);
        }

        if (he->w >= Bl)
        {
            if ((selected->e1 == he->e1) || (selected->e1 == he->e2) || (selected->e2 == he->e1) || (selected->e2 == he->e2))
            {
                selected = he;
                paths.push_back(he);
            }
        }
    }

    smgr->addCameraSceneNodeFPS(0, 50.f, 0.15f);

    while (device->run())
    {
		if (!device->isWindowActive() || !device->isWindowFocused() || device->isWindowMinimized())
			continue;

        driver->beginScene(true, true, SColor(255,100,101,140));

        for (u16 i = 0; i < paths.size(); i++)
        {
            driver->draw3DLine(paths[i]->pe1, paths[i]->pe2, SColor(55, 100, 255, 140));
        }

        smgr->drawAll();
        guienv->drawAll();

        driver->endScene();
    }

    device->drop();

    return 0;
}
