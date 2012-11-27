#define NS_OFFSET 0x9c		//offset in header where numbers begin
#define TEX_OFFSET 0xC0
#define NENTS_OFFSET 0x0c
#define NLIGHTS_OFFSET 0x44
#define ENTAD_OFFSET 0x28
#define ENTAD_OFFSET_ALSO 0x34
#include <vector>
#include <string>
#include "scn2obj.h"

using namespace std;

struct point3f
{
	f32 x,y,z;
};
struct point2f
{
	f32 u,v;
};
struct tempLight_t
{
    point3f color;
    f32 radius;
    point3f origin;
};

struct scnHeader_t
{
/* address | type| meaning*/
/*--------------------*/
/* 0x00 */	char    magic[4];
/* 0x04 */	u32     version;
/* 0x08 */	u32     datalen;
/* 0x0c */	u32     n_ents;
/* 0x10 */	u32     n_solids;
/* 0x14 */	u32     solid0_offset;  //should always be 0x98
/* 0x18 */	u32     solid0_length;	//lengths of solid[0], ie, worldspawn
/* 0x1c */	u32     solids_offset;  //offset to solids[1]
/* 0x20 */	u32     solids_length;    //length of all other solids
/* 0x24 */	u32     unk2;
/* 0x28 */	u32     ents_offset;
/* 0x2c */	u32     unk3;
/* 0x30 */	u32     unk4;
/* 0x34 */	u32     ents_offset2;
/* 0x38 */	u32     ents_size;
/* 0x4c */	u32     unk5;
/* 0x40 */	u32     lmaps_offset;   //lightmaps start address
/* 0x44 */	u32     n_lights;   //number of light enteties
/*zeros follow...*/

};
  struct scnSurf_t
    {
    //IMPORTANT: keep order, we rely on it to read
    char texture[32];
	f32 unk[2];
	u16 flags,alpha;
	u16 lmsize_h, lmsize_v;
	u16 width, height; //width and height of tex, in pixels
	u32 vertidxstart;
	u16 planeidx;
	u16 vertidxlen;
	u16 more;
	char stuff2[10];
	u8 * shading;
//	char extra[?]	??
    };

struct scnPlane_t
{
	f32 a,b,c,d;		//plane equation ax + by + cz + d = 0;
};
struct scnEntField_t
{
    char *key;
    char *value;
};

enum EScnMaterial
{
    ESM_DEFAULT     = 0x00,
    ESM_LIQUID      = 0x10,
    ESM_MUD         = 0x20,
    ESM_GRAVEL      = 0x30,
    ESM_PLASTER     = 0x40,
    ESM_CARPET      = 0x50,
    ESM_GLASS       = 0x60,
    ESM_WOOD        = 0x70,
    ESM_CREAKWOOD   = 0x80,
    ESM_BRICK       = 0x90,
    ESM_SHEETMETAL  = 0xA0,
    ESM_STEEL       = 0xB0
};

struct scnNode_t  //16 bytes
{
    s16 plane;  //splitting plane idx
    s8 area;   //?
    u8 material;

    s16 node1;  //node in front of plane
    s16 node2;  //node behind plane
    s16 nodep;  //parent node

    s16 cell;   //cell index
    s32 more;   //?
};

struct scnPortal_t
{
    char name[32];
    s32 nextcell; //cell idx this portal looks into
    scnPlane_t plane;
    f32 unk;        //float ?
    s32 n_verts;    //number of verts defining the portal
    point3f bb_verts[2]; //portal bounding box points
    point3f * verts;
};

struct scnRawCell_t //raw cell means it's the cell read not from the entity list
{
    char name[32];
    s32 n_nodesidxs;
    s32 n_portals;
    s32 n3; //?
    char skyname[32];
    u16 * nodesidxs; //index of nodes
    scnPortal_t * portals;
    //there is also more data here
};

class CScnBSPTree
{
public:
    u32 n_nodes;
    scnNode_t * nodes;
    scnPlane_t ** planes_ad;    //address to pointer to data, planes pointer is set only later
    scnPlane_t * planes; //also needs pointer to planes given from parent cscnsolid


    //constructor needs number of nodes
    CScnBSPTree(u32);
    ~CScnBSPTree();

    int loadNodes(std::ifstream*);

    //gives node in wich point is located)
    u32 findNodePos(f32 x, f32 y, f32 z);
    u32 findNodePos(point3f pos);

    //returns 1 if pos is in front of splitting plane, -1 if behind, 0 if in plane
    s32 evalNodePos(int nodeidx, f32 x, f32 y, f32 z);

};


enum N_TYPE{
    N_UNK1=0, N_VERTS, N_UVPOS, N_VERTIDXS, N_PLANES, N_NODES, N_SURFS, N_CELLS, N_UNK2
};

class CScnSolid
{
private:

    int loadSurfs(std::ifstream *);
    int loadNodes(std::ifstream *);
    int loadPlanes(std::ifstream *);
    int loadVerts(std::ifstream *);
    int loadUVPos(std::ifstream *);
    int loadVertIdxs(std::ifstream *);
    int loadUVIdxs(std::ifstream *);
    int loadUnk(std::ifstream *);
    int loadCells(std::ifstream *);
    int loadPortal(scnPortal_t *, std::ifstream *);
    int loadCellData(std::ifstream * file);


    int calcUniqueTexturesNames(std::ifstream *);


//    const static defColor = video::SColor(128,255,255,255); //doesn't work, why?
public:
    u32 n_unk1, n_verts, n_uvpos, n_vertidxs, n_planes, n_nodes, n_surfs, n_cells, n_unk2;
    u32 offset;
    u32 length;
    int * surfsad;

    scnSurf_t * surfs;
    scnPlane_t * planes;
	point3f	* verts;
	point2f * uvpos;
	u32 * vertidxs;
	u32 * uvidxs;
	CScnBSPTree * tree;
	scnRawCell_t * rawcells;

	//all texture names in the solid, each only once
	std::vector<std::string> textures;

    //constructor - do nothing for now
    CScnSolid ();
    ~CScnSolid();

    //load from file
    int loadSolid(std::ifstream*);

    //function only returns vertice from array
    //so it doesn't include surface specifics like color, tcoords or normal
    //irrlicht specific
    /*video::S3DVertex getVertice(u32 idx)
    {
        video::S3DVertex vert = video::S3DVertex(
                    verts[idx].x,verts[idx].y,verts[idx].z,        //coords
                    1,1,1,video::SColor(255,255,255,255),0,1);
        return vert;
    }

    core::array<video::S3DVertex> calcSurfVertices(u32 surfidx);
    */
    vector<u16> calcSurfIndices(u32 surfidx);


};


class CScnEnt
{
public:
    u32 srefidx;            //index into a solid, like doors
    u32 n_fields;

    //IMPORTANT - do not sort or else indexes will not match
    vector<string> keys;   //Do we need a vector for this?
    vector<string> values;

    string getField(string key);

    //constructor
    CScnEnt() {srefidx=0; n_fields=0; keys.clear(); values.clear();};
};

class CScn
{
private:
    void reset();
    CScnEnt * ents;
    int loadFile(std::ifstream * file);

    int loadHeader(std::ifstream * file);
    int loadEntities(std::ifstream * file);



public:
    scnHeader_t * header;
    CScnSolid * solids;

    u32 getVersion(){
        return header->version;}

    vector<CScnEnt*> cells;

    //get cell by index as defined by cell_index field
    CScnEnt * getCell(u32 cell_index);

    CScn ();
    CScn (std::ifstream * );
    ~CScn ();


    //returns pointer to CScnSolid from index or NULL if none
    CScnSolid * getSolid(u32 idx)
    {
        if (idx < header->n_solids)
            return &solids[idx];
        else
            return NULL;
    }

    //returns pointer to CScnEnt from index or NULL if none
    CScnEnt * getEnt(u32 idx)
    {
        if (idx < header->n_ents)
            return &ents[idx];
        else
            return NULL;
    }

    u32 getTotalSolids()
    {
        return header->n_solids;
    }

    u32 getTotalEnts()
    {
        return header->n_ents;
    }

};



