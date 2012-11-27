#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>

#include "CScn.h"
#include "util.h"
#include "export.h"

using namespace std;
void USAGE_ERROR()
{
   ERROR("Usage: scn2obj [-s <scale>] <filename>\n\n"
         "\tfilename - scn file name with extension\n\n"
         "\tOPTIONS:\n"
         "\t-s <scale> - scale geometry by factor <scale>\n");
}

int main(int argc, char ** argv )
{
   char missionname[256];
   char filename[256];
   float scaling = 1.0;

   SAY("scn2obj " VERSION "\n\n");

   if (argc < 2)
      USAGE_ERROR();

   for (int i = 1; i < argc; i++)
   {
      if (str_equals(argv[i],"-s"))
      {
         i++;
         if (i == argc)
         {
            SAY("Exptected scaling parameter\n");
            USAGE_ERROR();
         } else {
            scaling = atof(argv[i]);
            if (scaling == 0) {
               SAY("Error reading scaling parameter\n");
               USAGE_ERROR();
            }
         }
      }
      else
      {
         //filename
         strncpy(filename,argv[i],sizeof(filename));
         //mission name
         int dot_pos;
         for (dot_pos=0; dot_pos < sizeof(filename); dot_pos++)
            if (filename[dot_pos]=='.' || filename[dot_pos]=='\0')
               break;
         strncpy(missionname,filename,dot_pos);
         missionname[dot_pos]='\0';

      }
    }

    ifstream scnFile(filename,ios::binary);

    if (!scnFile.is_open())
        ERROR("Can't open %s",filename);

    CScn * scn = new CScn(&scnFile);

    scnExportObj(scn,missionname,scaling);

    delete scn;
    scnFile.close();

    return 0;
}
