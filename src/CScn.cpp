#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "CScn.h"
#include "util.h"

void CScn::reset()
{
    solids=0;
    ents=0;
    header=0;
}

//default constructor
CScn::CScn ()
{
    reset();
}

//constructor that reads scn file
CScn::CScn(std::ifstream * file)
{
    reset();
    loadFile(file);
}

//destructor
CScn::~CScn()
{
    if (solids)
        delete [] solids;

    if (header)
        delete header;

    if (ents)
        delete [] ents;

}

int CScn::loadFile(std::ifstream * file)
{
    //IMPORTANT: load must be in this order so we don't have to store addresses
    loadHeader(file);
    solids = (CScnSolid*) new (std::nothrow) CScnSolid[header->n_solids];

    if (!solids)
        ERROR("Error allocating %u CScnSolid's", header->n_solids);

    solids[0].offset=header->solid0_offset;
    solids[0].length=header->solid0_length;

    for (u32 i=0; i < header->n_solids; i++)
    {

        SAY("Getting solid %i/%u...\n",i,header->n_solids-1);
        solids[i].loadSolid(file);

        if (i < header->n_solids - 1)
            solids[i+1].offset=solids[i].offset + solids[i].length;

        SAY("done.\n");
    }

    loadEntities(file);
    return 0;
}

int CScn::loadHeader(ifstream * file)
{
 	SAY("Getting header...");

 	//header=(scnHeader_t*) malloc(sizeof(struct scnHeader_t));
    header = new scnHeader_t;

	file->seekg(0);
    read_generic(header,file,sizeof(scnHeader_t));

	if (!str_nequals(header->magic,"NCSM",4))
		ERROR("CScn::loadHeader: Magic doesn't match, not a scn file!");

    if (header->version!=269)
		ERROR("CScn::loadHeader: Wrong scn file version!");

    SAY("done.\n");
	return 0;
}

CScnSolid::~CScnSolid()
{
    //pointers must be set to zero initially otherwise errors
    if (rawcells)
    {
        for (u32 i=0; i < n_cells; i++)
        {
            if (rawcells[i].nodesidxs)
                delete [] rawcells[i].nodesidxs;

            if (rawcells[i].portals)
            {
                for (s32 j=0; j < rawcells[i].n_portals; j++)
                    if (rawcells[i].portals[j].verts)
                        delete [] rawcells[i].portals[j].verts;

                delete [] rawcells[i].portals;
            }
        }
        delete [] rawcells;
    }

    if (uvidxs)
        delete [] uvidxs;

    if (vertidxs)
        delete [] vertidxs;

    if (uvpos)
        delete [] uvpos;

    if (verts)
        delete [] verts;

    if (planes)
        delete [] planes;

    if (tree)
        delete tree;

    if (surfsad)
        delete [] surfsad;

    if (surfs)
    {
        for (u32 i=0;i < n_surfs ;i++)
            if (surfs[i].more==1)
                delete [] surfs[i].shading;

        delete [] surfs;
    }
}

int CScnSolid::loadSolid(std::ifstream * file)
{

    file->seekg(offset);
    SAY("offset: %u\n",offset);
    n_unk1=read_u32(file);
    n_verts=read_u32(file);
    n_uvpos=read_u32(file);
    n_vertidxs=read_u32(file);
    n_planes=read_u32(file);
    n_nodes=read_u32(file);
    n_surfs=read_u32(file);
    n_cells=read_u32(file);
    n_unk2=read_u32(file);

    length=read_u32(file);

    //must be in this order
    loadSurfs(file);
    loadNodes(file);
    loadPlanes(file);
    loadVerts(file);
    loadUVPos(file);
    loadVertIdxs(file);
    loadUVIdxs(file);

    loadUnk(file);
    loadCells(file);

    s32 n_texs = calcUniqueTexturesNames(file);
    SAY("\n\t%u unique textures\n",n_texs);

    if (n_texs != textures.size())
        ERROR("loadSolid: Number of unique textures and texture array size doesn't match");



    return 1;
}

int CScnSolid::loadCells(std::ifstream * file)
{
    SAY("\tGetting Cells...");
    rawcells = new scnRawCell_t[n_cells];

    for (u32 i=0; i < n_cells; i++)
    {
        rawcells[i].nodesidxs=0;
        rawcells[i].portals=0;

        SAY("\n\t\tcell[%u]: ",i);
        read_generic(rawcells[i].name,file,32);
        SAY("%s ", rawcells[i].name);
        rawcells[i].n_nodesidxs=read_s32(file);
        SAY("%d ", rawcells[i].n_nodesidxs);
        rawcells[i].n_portals=read_s32(file);
        SAY("%d ", rawcells[i].n_portals);
        rawcells[i].n3=read_s32(file);
        SAY("%d ", rawcells[i].n3);
        read_generic(rawcells[i].skyname,file,32);
        SAY("%s ", rawcells[i].skyname);

        rawcells[i].nodesidxs = new u16[rawcells[i].n_nodesidxs];
        read_generic(rawcells[i].nodesidxs,file,2 * rawcells[i].n_nodesidxs);

        rawcells[i].portals = new scnPortal_t[rawcells[i].n_portals];
        SAY("Reading %u portals... ", rawcells[i].n_portals);
        for (s32 j=0; j < rawcells[i].n_portals; j++)
        {
            rawcells[i].portals[j].verts=0;
            loadPortal(&(rawcells[i].portals[j]),file);
        }
        SAY("done.");

        loadCellData(file);
    }


   SAY("done.");

    return n_cells;
}
//read bboxes and surfaces
int CScnSolid::loadCellData(std::ifstream * file)
{
    u16 nn;
    s32 n=0;
    u16 n_surfs;

    //just skip for now
    while(n>-1)
    {
        file->seekg(sizeof(point3f)*2,ios::cur);

        nn=read_u16(file);
        n += nn;

        n_surfs=read_u16(file);
        file->seekg(2*n_surfs,ios::cur);

        n--;

    }

    return 0;
}

int CScnSolid::loadPortal(scnPortal_t * portal, std::ifstream * file)
{
    //SAY("\n\t\t\tReading portal...");
    read_generic(portal->name,file,32);
    //SAY("(%i)", strlen(portal->name));
    //SAY("%s ", portal->name);

    portal->nextcell=read_s32(file);
    //SAY("%u ",portal->nextcell);
    read_generic(&(portal->plane),file,sizeof(scnPlane_t));
    portal->unk=read_f32(file);
    //SAY("(%.1f %.1f %.1f %.1f %.1f)",portal->plane.a, portal->plane.b, portal->plane.c, portal->plane.d, portal->unk);

    portal->n_verts=read_s32(file);
    read_generic(portal->bb_verts,file,sizeof(point3f)*2);

    portal->verts = new point3f[portal->n_verts];
    read_generic(portal->verts, file, sizeof(point3f) * portal->n_verts);

    //SAY("done reading portal. ");
    return 1;
}

int CScnSolid::loadUnk(std::ifstream * file)
{
    SAY("\tSkipping Unk lump...");

    //About this lump only know it's 9 floats per surface
    file->seekg(36*n_surfs,ios::cur);

    SAY("done.\n");
    return 0;
}


int CScnSolid::calcUniqueTexturesNames(std::ifstream * file)
{
    int ret=0;
    bool found=false;

    for (u32 i=0; i < n_surfs; i++)
    {
        string texname(surfs[i].texture);
        found=false;
        for (int j=0; i < textures.size(); i++)
        {
            if (textures[j]==texname)
            {
                found=true;
                break;
            }
        }
        if (!found)
        {
            textures.push_back(texname);
            ret++;
        }
    }
    return ret;

}

int CScnSolid::loadUVIdxs(std::ifstream * file)
{
    u32 nvui = n_vertidxs;
    SAY("\tGetting UV coordinates indices...");

    uvidxs = new u32[nvui];   //allocate   n_uvidxs=n_vertidxs
    read_generic(uvidxs,file,sizeof(u32)*nvui);
    SAY("%u done.\n",nvui);

    return nvui;

}

int CScnSolid::loadVertIdxs(std::ifstream * file)
{
    SAY("\tGetting Vertex indices...");

    vertidxs = new u32[n_vertidxs];   //allocate
    read_generic(vertidxs,file,sizeof(u32)*n_vertidxs); //read all, should work

    SAY("%u done.\n",n_vertidxs);

    return n_vertidxs;
}

int CScnSolid::loadUVPos(std::ifstream * file)
{
    SAY("\tGetting UV coordinates...");

    uvpos = new point2f[n_uvpos];   //allocate
    read_generic(uvpos,file,sizeof(point2f)*n_uvpos); //read all, should work

    SAY("%u done.\n",n_uvpos);

    return n_uvpos;
}

int CScnSolid::loadVerts(std::ifstream * file)
{
    SAY("\tGetting Vertices...");
    verts = new point3f[n_verts];   //allocate
    read_generic(verts,file,sizeof(point3f)*n_verts); //read all, should work
    SAY("%u done.\n",n_verts);

    return n_verts;

}

int CScnSolid::loadPlanes(std::ifstream * file)
{
    SAY("\tGetting planes...");
    planes = new scnPlane_t[n_planes];   //allocate

    tree->planes = planes; //IMPORTANT
                            //TODO: make better

    read_generic(planes,file,sizeof(scnPlane_t)*n_planes); //read all, should work
    SAY("%u done.\n",n_planes);

    return n_planes;

}

int CScnSolid::loadNodes(std::ifstream * file)
{
    SAY("\tReading nodes...");
    //file->seek(n[N_NODES]*16,true);

    tree = new CScnBSPTree(n_nodes);

    if (tree->loadNodes(file) == -1)
        ERROR("loadNodes: Error reading nodes from solid,n_nodes < 1");

    SAY("done.\n");

    return 0;

}

int CScnSolid::loadSurfs(std::ifstream * file)
{
    SAY("\tGetting Surfaces...");

    surfs = new scnSurf_t[n_surfs];
    surfsad = new int[n_surfs];
    int i;
    for (i=0;i < n_surfs ;i++)
	{
		surfsad[i]=file->tellg();

		read_generic(&surfs[i],file,72);   //read the usual 72 first bytes

		if (surfs[i].more==1)  //means there are more bytes - the shading or smoothing or whatever we call it
        {
            surfs[i].shading = new u8[4*surfs[i].vertidxlen];     //allocate
            read_generic(surfs[i].shading,file,4*surfs[i].vertidxlen);
            //REMEMBER: because shading is initially set to  a random value, we must
            //make sure we only try to draw shading only when more is set to !0 or
            //make constructor to set initial value 0;
        }
        else if (surfs[i].more !=0)
			ERROR("CScnSolid: loadSurfs - Unexpected surface[%u].more value - expected 0 or 1",i);
	}
    SAY("%u done.\n",i);
    return i;
}

//default constructor
CScnSolid::CScnSolid()
{
    offset=0;
    length=0;
    //TODO: do something
    textures.clear();

	//setting pointers to zero
	rawcells=0;
	uvidxs=0;
	vertidxs=0;
	uvpos=0;
	verts=0;
	planes=0;
	tree=0;
	surfs=0;
	surfsad=0;

}

//return array of surface vertices with alpha, shading and uv information
//irr_specific
/*
core::array<S3DVertex> CScnSolid::calcSurfVertices(u32 surfidx)
{
    core::array<S3DVertex> v;
    scnSurf_t * surfi = &surfs[surfidx];

    S3DVertex tVert;

    u8 * ps= surfi->shading;

    for (u32 i=0; i < surfi->vertidxlen; i++)
    {
        tVert = getVertice(vertidxs[surfi->vertidxstart+i]);

        tVert.TCoords.X=uvpos[uvidxs[surfi->vertidxstart+i]].u;
        tVert.TCoords.Y=uvpos[uvidxs[surfi->vertidxstart+i]].v;

        if (surfi->alpha!=255)
                tVert.Color=video::SColor(surfi->alpha,255,255,255);

        if (surfi->more)
        {
            tVert.Color = video::SColor(ps[3],ps[0],ps[1],ps[2]);
            ps+=4;
        }
        v.push_back(tVert);
    }

    return v;
}
*/

vector<u16> CScnSolid::calcSurfIndices(u32 surfidx)
{
    vector<u16> idxs;
    scnSurf_t * surfi = &surfs[surfidx];

    idxs.push_back(0);
    idxs.push_back(1);
    idxs.push_back(2);

    //each surface is a new meshbuffer - because they may have different textures
    //draw mesh like a triangle fan - first three vertices define a triangle,
    //from then, each new one defines a tringle with the last and the origin (0)

    for (u16 j=3; j<surfi->vertidxlen ;j++)
    {
        idxs.push_back(j-1);
        idxs.push_back(j);
        idxs.push_back(0);
    }
    return idxs;

}

//constructor
CScnBSPTree::CScnBSPTree(u32 n)
{
    n_nodes=n;
                            //because at the time this is called planes in CScnsolid hasen't been set
                            //solid->plane point to nowhere, so we have to use a pointer to a pointer
                            //TODO: make less ugly, it's horrible

    if (n>0)
        nodes = new scnNode_t[n];
    else
        nodes=0;
}

//destructor
CScnBSPTree::~CScnBSPTree()
{
    if (nodes)
        delete[] nodes;

}

int CScnBSPTree::loadNodes(std::ifstream * file)
{
    if (n_nodes <= 0)
        return -1;

    read_generic(nodes, file,n_nodes*sizeof(scnNode_t));

    SAY("done.\n");

    return n_nodes;
}

u32 CScnBSPTree::findNodePos(point3f pos)
{
    return findNodePos(pos.x, pos.y, pos.z);
}

u32 CScnBSPTree::findNodePos(f32 x, f32 y, f32 z)
{
    s16 idx=0;
    while (idx < n_nodes)
    {
        if (nodes[idx].plane == -1){  //it's a leaf, no more nodes
           break;
        }

        //TODO: use recursive function
        s32 eval = evalNodePos(idx,x,y,z);

        if (eval == 0)
            ERROR("findNodePos: eval=0, what to do now?");
        else if (eval>0)
            idx=nodes[idx].node1;
        else if (eval<0)
            idx=nodes[idx].node2;
    }
    return idx;
}

//returns 1 if pos is in front of splitting plane, -1 if behind, 0 if in plane
s32 CScnBSPTree::evalNodePos(int nodeidx, f32 x, f32 y, f32 z)
{
    scnPlane_t * plane = &planes[nodes[nodeidx].plane];

    //dot producct
    f32 prod= x * plane->a + y * plane->b + z * plane->c + plane->d;
    //SAY("dot product= %f",prod);

    //deve haver uma maneira mais inteligente de fazer isto,
    if (prod==0)
        return 0;

    else if (prod>0)
        return 1;
    else //prod<0
        return -1;

}

int CScn::loadEntities(std::ifstream * file)
{

    u32 n_ents = header->n_ents;
    SAY("Getting %u Entities...",n_ents);

    ents = new (std::nothrow) CScnEnt[n_ents];

    if (ents==NULL)
        ERROR("Error allocating memory for CScnEnt");

    file->seekg(header->ents_offset2);

    u16 keylen;
    u16 vallen;
    CScnEnt * enti;
    char str1[128];
    char str2[128];

    for (u32 i=0;i<n_ents;i++)
    {
        enti=&ents[i];

        enti->n_fields=read_u32(file);
        enti->srefidx=read_u32(file);

        for (int n=0;n<enti->n_fields;n++)
        {
            keylen=read_u16(file);
            vallen=read_u16(file);
            if (keylen >= 128 || vallen >= 128)
                ERROR("Ent[%i] has field %i with too large a string",i,n);

            read_generic(str1,file,keylen);
            enti->keys.push_back(string(str1));

            read_generic(str2,file,vallen);
            enti->values.push_back(string(str2));

            //TODO: make sorted according to cell index
            if (str_equiv(str1,"Classname") && str_equiv(str2,"Cell"))
                cells.push_back(enti);
        }
    }
    SAY("%u done.\n",n_ents);
    return n_ents;
}

CScnEnt * CScn::getCell(u32 idx)
{
    CScnEnt * celli;
    string val;
    for (int i=0; i < cells.size(); i++)
    {
        celli = cells[i];
        val=celli->getField("cell_index");
        if (atoi(val.c_str()) == idx)
            return celli;
    }
    return NULL;
}

string CScnEnt::getField(string key)
{
    for (int i=0; i<n_fields; i++)
        if (str_equiv(keys[i].c_str(),key.c_str()))
            return values[i];

    return NULL;
}








