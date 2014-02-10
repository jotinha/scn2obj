# -*- coding: utf-8 -*-
"""
Created on Thu Apr 18 01:27:04 2013

@author: jsousa
"""

from scn import *
import os

#MAP = 'cubes'
#scn = CScn(MAP + '.scn')

ws = scn.solids[0]

verts = [[v.x,v.y,v.z] for v in ws.verts]
uvs = [[uv.u,uv.v] for uv in ws.uvs]

idxs = list(ws.vertidxs)
uvidxs = list(ws.uvidxs)

def exists(texture):
    return os.path.exists('textures/'+texture+'.bmp') or \
           os.path.exists('textures/'+texture+'.tga') 

def tex2path(texture):
    return 'pngs/' + texture + '.png'


#create textures array
textures = list(set(surf.texture for surf in ws.surfs)) + \
           list(set(surf2lmap))
           
faces =[]
faceVertexUvs = []
materials = []

def addface(i1,i2,i3,surf,lmap):
    texture = surf.texture
    istart = surf.vertidxstart
    face = [
        idxs[istart+i1],idxs[istart+i2],idxs[istart+i3],
        uvidxs[istart+i1],uvidxs[istart+i2],uvidxs[istart+i3],
    ]

    #define material as indexes into texture array    
    mat = (textures.index(texture),textures.index(lmap), surf.alpha,surf.hasVertexColors)
    if mat in materials:
        materialIdx = materials.index(mat)
    else:
        materials.append(mat)
        materialIdx = len(materials)-1

    face.append(materialIdx)

    if surf.hasVertexColors:       #if shading information
        face.append(surf.vertexColors[i1])  #RGBA colors
        face.append(surf.vertexColors[i2])
        face.append(surf.vertexColors[i3])
    
    faces.append(face)

face2surf = []  
surf_lmap_floats=  []
surf_unk_floats = []
for isurf,surf in enumerate(ws.surfs):
    #create triangle fan
    istart = surf.vertidxstart
    for i in range(1,surf.n_verts-1):
        addface(0,i,i+1,surf,surf2lmap[isurf])
        face2surf.append(isurf)
    
    surf_lmap_floats.append(hlmaps_all[0][isurf].floats[:])
    surf_unk_floats.append(surf.unk[:])

#faceVertexUVs2 =[]
#for f in range(len(faces)):
#    surf = face2surf[f]
#    floats = surf.floats
#    plane = scn.solids[0].plane[surf.planeidx]
#    plane_norm = array([plane.a,plane.b,plane.c])
#
#    u_axis = array([floats[0],0])
#    v_axis = array([0,floats[1]])
#    u_offset = floats[2]
#    v_offset = floats[3]
#    
#    faceuv = []
#    for uvidx in facesUVs[f]:
#        uvs[uvidx][0] + fl
#        
#        
#        faceuv.append()
#        

#expand textures to full path
for i,tex in enumerate(textures):
    if isinstance(tex,str): #diffuse texture
        textures[i] = tex2path(tex)
    elif isinstance(tex,tuple): #light map
        textures[i] = lmi2path(*tex)
    else:
        raise SystemError
           
import simplejson
j = simplejson.dumps({
    'verts':verts,
    'uvs':uvs,
    'faces':faces,
#    'faceVertexUvs':faceVertexUvs,
    'materials':materials,
    'textures': textures,
    'face2surf':face2surf,
    'surf_lmap_floats': surf_lmap_floats,
    'surf_unk_floats': surf_unk_floats,
#    'surf2lmap':surf2lmap
    })

open(EXPORT_PATH + MAP + '.json','wt').write(j)