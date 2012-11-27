#include "CScn.h"
#include "export.h"
#include "util.h"
#include <set>
#include <string.h>

enum EVertOrder {
   EVORDERXYZ,
   EVORDERXZY,
   EVORDERXY_Z
};

void orderxyz(f32 x,f32 y, f32 z, f32 * a, f32 * b, f32 * c,EVertOrder vertOrder)
{
    switch (vertOrder)
    {
        case(EVORDERXYZ):
            *a = x;
            *b = y;
            *c = z;
            break;
        case (EVORDERXZY):
            *a = x;
            *b = z;
            *c = y;
            break;
        case (EVORDERXY_Z):
            *a = x;
            *b = y;
            *c = -z;
            break;
        default:
            ERROR("Invalid EVertOrder");
   }
}

void vert2str(char * outstr,size_t outstr_size, point3f * vert,float scaling )
{
   f32 a,b,c;
   orderxyz(vert->x*scaling, vert->y*scaling, vert->z*scaling, &a, &b, &c,EVORDERXYZ);
   snprintf(outstr,outstr_size,"%f %f %f",a,b,c);
}

void scnExportObj(CScn * scn, char * name, float scaling)
{
    int i;
    int SIZETMP = 128;
    char tmp[SIZETMP];

    char objname[SIZETMP];
    char mtlname[SIZETMP];
    std::set<std::string> unique_texs;
    std::string prevTex;
    std::string newTex;
    f32 a,b,c;


    sprintf(objname,"%s.obj",name);
    sprintf(mtlname,"%s.mtl",name);

    SAY("Building obj and mtl file...");

    FILE * obj = fopen(objname,"wt");
    if (obj==NULL)
        ERROR("Can't write to file %s\n",obj);

    FILE * mtl = fopen(mtlname,"wt");
    if (mtl==NULL)
        ERROR("Can't write to file %s\n",obj);

    fprintf(obj,"mtllib %s\n",mtlname);

    int nvp=0; //number of vertices in previous solid
    int nuvp=0; //number of uvpos in previous solid

    for (u32 idx=0; idx < scn->header->n_solids; idx++)
    {
        prevTex = "";

        CScnSolid * solid = scn->getSolid(idx);

        u32 n_surfs=solid->n_surfs;
        u32 n_verts=solid->n_verts;
        u32 n_uvpos=solid->n_uvpos;

        fprintf(obj,"\ng solid%03u\n",idx);

        for (i=0; i < n_verts; i++) {
            vert2str(tmp,sizeof tmp, &solid->verts[i],scaling);
            fprintf(obj, "v %s\n", tmp);
        }

        for (i=0; i < n_uvpos; i++) {
            point2f * uvi = &solid->uvpos[i];
            fprintf(obj, "vt %f %f \n", uvi->u, -uvi->v);
        }

        for (i=0; i< n_surfs; i++)
        {
            scnSurf_t * surfi = &(solid->surfs[i]);
            //if (solid->surfs[i].alpha <= 1)   //don't draw transparent solids
            //    continue;

            newTex = std::string(surfi->texture);
            if (newTex != prevTex)
               fprintf(obj,"usemtl %s\n",newTex.c_str());
            prevTex = newTex;

            //write to string instead of directly to file
            sprintf(tmp, "f ");

            u32 vstart = surfi->vertidxstart;
            std::set<u32> all_vidxs;
            for (u16 j=0; j < surfi->vertidxlen; j++)
            {
                u32 vi = solid->vertidxs[vstart+j]+1 + nvp;
                all_vidxs.insert(vi);
                snprintf(tmp + strlen(tmp),SIZETMP-strlen(tmp),
                          "%u/%u ", vi, solid->uvidxs[vstart+j]+1 + nuvp);
            }
            //all_vidxs is a set. If the size equals < nverts this means that some vertices overlap.
            //Which is wierd and I don't understand why the scn does that. Anyway, if there are
            //less than three different vertices, we should ignore
            // (for some reason Maya complains some times, other not)
            if ((int) all_vidxs.size() < 3)
               fprintf(obj,"#%s\n",tmp); //comment out this line
            else
               fprintf(obj,"%s\n",tmp);

        }
        nvp += n_verts;
        nuvp += n_uvpos;

        for (i=0; i < solid->textures.size(); i++)
            unique_texs.insert(solid->textures[i]);

    }
    for (std::set<std::string>::iterator it=unique_texs.begin();
            it != unique_texs.end();
            it ++)
    {
        fprintf(mtl, "newmtl %s\n", it->c_str());
        fprintf(mtl, "map_Kd textures/%s.bmp\n", it->c_str());
    }

    //--------------------------------------------------------------
    //export portals
    //--------------------------------------------------------------
    fprintf(obj,"\n# portals\n");
    fprintf(obj,"usemtl portal\n");
    for (u32 idx=0; idx < scn->header->n_solids; idx++)
    {
        CScnSolid * solid = scn->getSolid(idx);
        for (u32 ci =0; ci < solid->n_cells; ci++)
        {
            scnRawCell_t * cell = &(solid->rawcells[ci]);
            for (u32 pi = 0; pi < cell->n_portals; pi++)
            {
               scnPortal_t * portal = &(cell->portals[pi]);
               fprintf(obj,"\ng PORTAL_%s\n",portal->name);

               for (u32 i=0; i < portal->n_verts; i++)
               {
                  vert2str(tmp,sizeof tmp, &portal->verts[i],scaling);
                  fprintf(obj, "v %s\n", tmp);

               }
               //draw the portal as a polygon with
               //negative indexing.
               //For each polygon that defines a portal in a cell there will
               //be an opposite polygon in another cell. Not only are the order
               //of their vertices slightly different they are also slightly separated: 0.04?

               //No uv information.

//               fprintf(obj, "f ");
//               for (u32 i=1; i <= portal->n_verts; i++)
//               {
//                  fprintf(obj,"-%i ",i);
//               }
//               fprintf(obj,"\n");

              //this is the right ordder:
               fprintf(obj, "f ");
               for (u32 i=portal->n_verts; i > 0; i--)
               {
                  fprintf(obj,"-%i ",i);
               }
               fprintf(obj,"\n");

            }
        }
    }

    //add portal material
    fprintf(mtl,"newmtl portal\n");
    fprintf(mtl,"map_Kd textures/portal.bmp\n");


    fclose(obj);
    fclose(mtl);
    SAY("done.\n");
}



