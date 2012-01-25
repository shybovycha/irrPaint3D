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
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
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

vector2di findCoVerticesForEdge(u16 e1, u16 e2, u16* indices, u16 indicesCnt, S3DVertex* vertices, f32 threshold = 0.001f)
{
	s16 V[] = { -1, -1 }, cnt = 0;

    for (u16 v = 0; v < indicesCnt; v += 3)
    {
		u32 a = indices[v], b = indices[v + 1], c = indices[v + 2];

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

		if ((vertices[a].Pos - vertices[e1].Pos).getLength() < threshold && (vertices[c].Pos - vertices[e2].Pos).getLength() < threshold)
			V[cnt++] = b; else
		if ((vertices[a].Pos - vertices[e1].Pos).getLength() < threshold && (vertices[b].Pos - vertices[e2].Pos).getLength() < threshold)
			V[cnt++] = c; else
		if ((vertices[a].Pos - vertices[e2].Pos).getLength() < threshold && (vertices[b].Pos - vertices[e1].Pos).getLength() < threshold)
			V[cnt++] = c; else
		if ((vertices[b].Pos - vertices[e1].Pos).getLength() < threshold && (vertices[c].Pos - vertices[e2].Pos).getLength() < threshold)
			V[cnt++] = a; else
		if ((vertices[b].Pos - vertices[e2].Pos).getLength() < threshold && (vertices[c].Pos - vertices[e1].Pos).getLength() < threshold)
			V[cnt++] = a; else
		if ((vertices[a].Pos - vertices[e2].Pos).getLength() < threshold && (vertices[c].Pos - vertices[e1].Pos).getLength() < threshold)
			V[cnt++] = b;

        if (cnt > 1)
            break;
    }

    return vector2di(V[0], V[1]);
}

HalfEdge* createHaldEdge(u32 e1, u32 e2, u16* indices, u32 indexCount, S3DVertex* vertices)
{
	vector2di coVerts = findCoVerticesForEdge(e1, e2, indices, indexCount, vertices);

    if (coVerts.Y < 0)
        coVerts.Y = coVerts.X; else
	if (coVerts.X < 0)
		return 0;

    u32 v1 = coVerts.X, v2 = coVerts.Y;

    vector3df a1 = (vertices[v1].Pos - vertices[e1].Pos), a2 = (vertices[v2].Pos - vertices[e1].Pos), edge = (vertices[e2].Pos - vertices[e1].Pos);
    vector3df n1 = a1.crossProduct(edge), n2 = a2.crossProduct(edge);
    f32 w = n1.dotProduct(n2) / (n1.getLength() * n2.getLength());

    HalfEdge* he = new HalfEdge(indices[e1], indices[e2], v1, v2, w);

    he->setPositions(vertices[e1].Pos, vertices[e2].Pos, vertices[v1].Pos, vertices[v2].Pos);

	return he;
}

array<HalfEdge*> fillHalfEdges(IMesh* mesh)
{
    array<HalfEdge*> halfEdges;
    u16 meshBufferCount = mesh->getMeshBufferCount();

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

                HalfEdge* he = createHaldEdge(e1, e2, indices, indexCount, vertices);

                if (!he)
                    return 1;

                halfEdges.push_back(he);
            }

            {
                u32 e1 = indices[e + 1], e2 = indices[e + 2];

                HalfEdge* he = createHaldEdge(e1, e2, indices, indexCount, vertices);

                if (!he)
                    return 1;

                halfEdges.push_back(he);
            }

            {
                u32 e1 = indices[e + 2], e2 = indices[e];

                HalfEdge* he = createHaldEdge(e1, e2, indices, indexCount, vertices);

                if (!he)
                    return 1;

                halfEdges.push_back(he);
            }
        }
    }

    return halfEdges;
}

array<HalfEdge*> detectAndGrowFeatures(array<HalfEdge*> halfEdges, f32 Bl, f32 Bu)
{
    array<HalfEdge*> paths;
    HalfEdge* selected = 0;

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
            if (selected && ((selected->e1 == he->e1) || (selected->e1 == he->e2) || (selected->e2 == he->e1) || (selected->e2 == he->e2)))
            {
                selected = he;
                paths.push_back(he);
            }
        }
    }

    return paths;
}

int main()
{
    IrrlichtDevice *device =
            createDevice(video::EDT_BURNINGSVIDEO, dimension2d<u32>(1024, 768), 32,
                    false, false, true, 0);

    if (!device)
        return 1;

    device->setWindowCaption(L"Hello World! - Irrlicht Engine Demo");

    IVideoDriver* driver = device->getVideoDriver();
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();

    guienv->addStaticText(L"Hello World! This is the Irrlicht Software renderer!",
            rect<s32>(10,10,260,22), true);

    //IAnimatedMesh* modelMesh = smgr->getMesh("../../media/Biomech_Fiera.3DS");
	//IAnimatedMesh* modelMesh = smgr->getMesh("../../media/Sovereign_1.obj");
	//IAnimatedMesh* modelMesh = smgr->getMesh("../../media/dwarf.x");
	//IAnimatedMesh* modelMesh = smgr->getMesh("../../media/earth.x");
	//IAnimatedMesh* modelMesh = smgr->getMesh("../../media/bun_zipper.ply");

	IAnimatedMesh* modelMesh = smgr->getMesh("../../media/sydney.md2");
	
    if (!modelMesh)
    {
        device->drop();

        return 1;
    }

    IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(modelMesh);

	//smgr->addLightSceneNode(0, vector3df(0, 100, 0), SColorf(100.f, 100.f, 100.f));

    if (node)
    {
        node->setMaterialFlag(EMF_LIGHTING, false);
        node->setAnimationSpeed(0);
        //node->setMaterialTexture( 0, driver->getTexture("../../media/Sovereign_1.jpg") );
		//node->setScale(vector3df(0.f, 0.f, 0.f));
    }

	//node->setScale(vector3df(10.f, 10.f, 10.f));

	IMesh* mesh = modelMesh->getMesh(0);
    array<HalfEdge*> halfEdges = fillHalfEdges(mesh);

    f32 Bu = 0.995f, Bl = 0.92f;

    array<HalfEdge*> paths = detectAndGrowFeatures(halfEdges, Bl, Bu);

    smgr->addCameraSceneNodeFPS(0, 50.f, 0.0125f);
	smgr->getActiveCamera()->setNearValue(0.01);

	u32 edgeCount = 0;

	for (u16 i = 0; i < modelMesh->getMeshBufferCount(); i++)
		edgeCount += modelMesh->getMeshBuffer(i)->getIndexCount();

	printf("Scars: %ld\nEdge count: %ld\n", paths.size(), edgeCount);

	node->setVisible(false);

	while (device->run())
    {
		if (!device->isWindowActive() || !device->isWindowFocused() || device->isWindowMinimized())
			continue;

		driver->beginScene(true, true, SColor(255,100,101,140));

		smgr->drawAll();
        guienv->drawAll();

		driver->setTransform(video::ETS_WORLD, matrix4::EM4CONST_IDENTITY);

		for (u16 i = 0; i < paths.size(); i++)
        {
            driver->draw3DLine(paths[i]->pe1 * 1.005, paths[i]->pe2 * 1.005, SColor(55, 100, 255, 140));
        }

        driver->endScene();
    }

    device->drop();

    return 0;
}
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
