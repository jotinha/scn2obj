# -*- coding: utf-8 -*-
"""
Created on Fri Apr 19 02:01:37 2013

@author: jsousa
"""
from scn import Scn
from scntypes import *
from StringIO import StringIO
from collections import defaultdict

MAP = 'missiona'
scn = Scn(MAP + '.scn')
EXPORT_PATH = '/home/jsousa/dev/web/scn/'
TEXSIZE= 128

class Bunch:
    def __init__(self, **kwds):
        self.__dict__.update(kwds)


def load_switchable_lmaps(f,n):
    slmaps = []
    for _ in range(n):
        h = read_u32(f)
        buf.seek(-4,1)    #go back
#        if h==0:        #this is an extra lmap
        slmaps.append(read_struct(f,tSwitchableLMapHeader))
#        else:
#            break
    return slmaps

def load_lmapHeaders(f,n):
    lh = []
    for i in range(n):
        lh.append(read_struct(f,tLMapHeader))
    return lh

def load_lumps(f,n):
    lumps = []
    for i in range(n):
        size,n_unk = read_u32(f,2)
        print 'cell %2i: start: %i, size: %i, n_unk:%i' % (i,f.tell()-8, size,n_unk)
        if size == 0: break
        lump = Bunch(
            size=size,
            n_unk=n_unk,
            data=f.read(size)
        )        
       
        lumps.append(lump)
    return lumps
    

def load_lbitmaps(f,lmapHeaders,surfs,lbitmaps_offset):
    lmaps  =[]
    for lh,surf in zip(lmapHeaders,surfs):
        f.seek(lbitmaps_offset + lh.offset,0)
        w,h = surf.lmsize
        lmap = read_array(f,'B',h*w*3)
        lmap = array(lmap,dtype='uint8').reshape(h,w,3,order='C')
        lmaps.append(lmap)
    return lmaps

scn.file.seek(scn.header.lmaps_offset,0)
rest = scn.file.read()
buf = StringIO(rest)
end = scn.file.tell()
totalLmapsSize = end - scn.header.lmaps_offset
nsurfs = len(scn.solids[0].surfs)
slmaps = load_switchable_lmaps(buf,scn.header.n_extralmaps)
print "hlmaps start at ", scn.header.lmaps_offset + buf.tell()
hlmaps0 = load_lmapHeaders(buf,nsurfs)
lumpstart =buf.tell() + 8
print "light bitmaps start at ", scn.header.lmaps_offset + lumpstart

lumps = load_lumps(buf,scn.solids[0].header.n_cells)

hlmaps_all = [hlmaps0,]
for solid in scn.solids[1:]:
    hlmaps_all.append(
        load_lmapHeaders(buf,solid.header.n_surfs))
#hlmaps1 = load_lmapHeaders(buf,len(scn.solids[1].surfs))
#hlmaps2 = load_lmapHeaders(buf,len(scn.solids[2].surfs))

def show_lbitmaps(lbitmaps):
    for lmap in lbitmaps:
        clf()
        imshow(array(lmap)/255.0,interpolation='none')
        waitforbuttonpress()

lightMapTextures= []
def save_lbitmaps(folder,lbitmaps):
    os.system('mkdir -p %s' % folder)
    for i,lmap in enumerate(lbitmaps):
        fname = '%s/lmap_%i.png' % (folder,i)
        lightMapTextures.append(fname)
        imsave(fname,array(lmap)/255.0,format='png')

for i in range(len(scn.solids)):
    print i
#    lmaps = load_lbitmaps(buf,hlmaps_all[i],scn.solids[i].surfs,lumpstart)
#    show_lbitmaps(lmaps)
#    save_lbitmaps('lmaps/solid%02i/'%i,lmaps)
#    if i > 0:    
#        show_lbitmaps(lmaps)

def create_master_lbitmap(hlmaps,surfs,lumps,bitmapIdx):
    """build a 128x128 texture from hlmaps"""

def create_all_master_lbitmaps(hlmaps,surfs,lumps):
    
    textures = defaultdict(lambda : np.zeros((TEXSIZE,TEXSIZE,3),dtype='uint8'))
    textures_written = defaultdict(lambda : np.zeros((TEXSIZE,TEXSIZE),dtype='uint8'))
    surf2lmap = []
    
    for hlmap,surf in zip(hlmaps,surfs):
        if hlmap.unk == -1: #these are not lit
            surf2lmap.append((-1,-1))
            continue
        
        surf2lmap.append((hlmap.cellidx,hlmap.unk))
        texture = textures[hlmap.cellidx,hlmap.unk]
        texture_written = textures_written[hlmap.cellidx,hlmap.unk]

        x0,y0 = hlmap.pos%TEXSIZE, hlmap.pos//TEXSIZE
        sx,sy = surf.lmsize
#        print '(%i,%i) -> (%i,%i)' %(x0,y0,x0+sx,y0+sy)
        
       
        buf = StringIO(lumps[hlmap.cellidx].data)
        buf.seek(hlmap.offset)
        for y in range(y0,y0+sy):
            for x in range(x0,x0+sx):
                try:
                    texture[y,x,:] = struct.unpack('B'*3,buf.read(3))
                    if texture_written[y,x] != 0:
                        print "WARNING: already wrote to pos %i,%i! Tex: (%i,%i) SurfIdx:%i" % (y,x,hlmap.cellidx,hlmap.unk,hlmaps.index(hlmap)) 
                        
                    texture_written[y,x] += 1
                        
                except struct.error,e:
                    print 'hlmap idx = %i ' % hlmaps.index(hlmap)
                    raise e
                    
    #normalize to 0-1 range
#    texture = texture/255.0
    return textures,surf2lmap

def lmi2path(*lmapidxs):
    return 'lmaps/' + MAP + '_lmap%i_%i.png' % lmapidxs    

scn.file.seek(scn.header.lmaps_offset + lumpstart)
mlbmaps,surf2lmap = create_all_master_lbitmaps(hlmaps_all[0],scn.solids[0].surfs,lumps)
for i,m in mlbmaps.iteritems():
#    figure()
#    title(lmi2path(*i))
#    imshow(m,interpolation='none',vmin=0,vmax=255)    
#    imsave(EXPORT_PATH + lmi2path(*i), m, format='png')
    pass


#also create a fully lit -1 lmap
#imsave(EXPORT_PATH + lmi2path(-1,-1),np.ones((128,128,3)))


bb=[hl.b for hlmaps in hlmaps_all for hl in hlmaps]
print unique(bb)
