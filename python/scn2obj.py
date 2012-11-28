import struct
import collections
import os
import re
os.chdir('../bin/')

def read_u32(file):
    return struct.unpack('I',file.read(4))[0]

def read_s32(file):
    return struct.unpack('i',file.read(4))[0]
    

def read_u16(file):
    return struct.unpack('H',file.read(2))[0]

def read_cstring(file,size):
    """
    read size bytes from file and returns string.
    
    If string is null terminated, return only up to null character
    """
    return file.read(size).split('\0',1)[0]
    

class CScn(object):

    def __init__(self,fname):
        super(CScn, self).__init__()
        self.file = open(fname,'rb')
        self.loadHeader()
        self.loadEntities()
        

    def loadHeader(self):
        self.file.seek(0)
        header = collections.namedtuple('header',(
                    'magic',
                    'version',
                    'datalen',
                    'n_ents',
                    'n_solids',
                    'solid0_offset',  #should always be 0x98
                    'solid0_length',	#lengths of solid[0], ie, worldspawn
                    'solids_offset',  #offset to solids[1]
                    'solids_length',    #length of all other solids
                    'unk2',
                    'ents_offset',
                    'unk3',
                    'unk4',
                    'ents_offset2',
                    'ents_size',
                    'unk5',
                    'lmaps_offset',   #lightmaps start address
                    'n_lights',   #number of light enteties
                    ))
        fmt = '=4s'+'i'*(len(header._fields)-1)
                    
        self.header = header(*struct.unpack(fmt,self.file.read(struct.calcsize(fmt))))
        assert self.header.ents_offset == self.header.ents_offset2
        
    def loadEntities(self):
        self.file.seek(self.header.ents_offset2,0)
        ent_t = collections.namedtuple('ent',('n_fields','srefidx','items'))
        def loadEntity():
            n_fields = read_u32(self.file)
            srefidx  = read_s32(self.file)
            items = {}
            for i in range(n_fields):
                keylen = read_u16(self.file)
                vallen = read_u16(self.file)
                k = read_cstring(self.file,keylen)
                v = read_cstring(self.file,vallen)
                items.update({k:v})
            return ent_t(n_fields,srefidx,items)
        
        self.entities = [loadEntity() for _ in range(self.header.n_ents)]
        
    
    def searchEnts(self,classname,mode='exact'):
        """
        search entities and return those that match classname
        
        mode can be 're_search' or 're_match' for regular expression or 'exact' for exact match (case insensitive)
        """
        if mode=='exact':
            cond = lambda ent: ent.items['classname'].lower() == classname.lower()
        elif mode == 're_search':
            cond = lambda ent: re.search(classname,ent.items['classname'],flags=re.IGNORECASE) is not None
        elif mode == 're_match':
            cond = lambda ent: re.match(classname,ent.items['classname'],flags=re.IGNORECASE) is not None
            
        else:
            raise SystemError, "Invalid mode: " + str(mode)
            
        return [ent for ent in self.entities if cond(ent)]
            

    def getLights(self):
        return self.searchEnts('light')
                
scn = CScn('missiona.scn')
lights = scn.getLights()