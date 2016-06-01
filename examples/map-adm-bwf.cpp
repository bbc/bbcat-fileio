
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-fileio/ADMRIFFFile.h>

using namespace bbcat;

BBC_AUDIOTOOLBOX_START
extern bool bbcat_register_bbcat_fileio();
BBC_AUDIOTOOLBOX_END

static std::map<std::string,std::string> colours;

void GenerateDisplay(std::vector<std::string>& lines, const ADMObject *obj)
{
  const ADMAudioTrack *tr;
  const ADMAudioChannelFormat *cf;
  std::string line;

  Printf(line, "1:\"%s\" [label=\"", obj->GetID().c_str());

  if ((tr = dynamic_cast<const ADMAudioTrack *>(obj)) != NULL)
  {
    Printf(line, "Track %u", tr->GetTrackNum() + 1);
  }
  else Printf(line, "%s", obj->GetName().c_str());

  Printf(line, "\\n(%s,\\n%s", obj->GetType().c_str(), obj->GetID().c_str());

  if (obj->GetTypeLabel() != ADMObject::TypeLabel_Unknown)
  {
    Printf(line, ",\\nType %04x:%s", obj->GetTypeLabel(), obj->GetTypeDefinition().c_str());
  }

  if ((cf = dynamic_cast<const ADMAudioChannelFormat *>(obj)) != NULL)
  {
    Printf(line, ",\\n%u blocks", (uint_t)cf->GetBlockFormatRefs().size());
  }

  Printf(line, ")\",");
  Printf(line, "shape=box,style=filled");

  std::map<std::string,std::string>::iterator it;
  if ((it = colours.find(obj->GetType())) != colours.end()) Printf(line, ",fillcolor=\"%s\"", it->second.c_str());

  Printf(line, "];");

  lines.push_back(line);
}

bool IncludeID(const std::map<std::string,bool>& ids, const std::string id)
{
  std::map<std::string,bool>::const_iterator it;

  for (it = ids.begin(); it != ids.end(); ++it)
  {
    if (matchstring(it->first.c_str(), id.c_str())) return true;
  }

  return false;
}

int main(int argc, char *argv[])
{
  // ensure libraries are set up
  bbcat_register_bbcat_fileio();

  if (argc < 2)
  {
    fprintf(stderr, "Usage: map-adm-bwf [-pure] <bwf-file> [<bmf-file>]\n");
    exit(1);
  }

  colours[ADMAudioProgramme::Type]     = "#ff8080";
  colours[ADMAudioContent::Type]       = "#80ff80";
  colours[ADMAudioObject::Type]        = "#8080ff";
  colours[ADMAudioTrack::Type]         = "#e0ffff";
  colours[ADMAudioPackFormat::Type]    = "#ffe0e0";
  colours[ADMAudioTrackFormat::Type]   = "#e0ffe0";
  colours[ADMAudioStreamFormat::Type]  = "#e0e0ff";
  colours[ADMAudioChannelFormat::Type] = "#ffe0ff";

  // ADM aware WAV file
  ADMRIFFFile file;

  int  i;
  bool incstddef = false;
  std::map<std::string,bool> includedids;
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-pure") == 0)
    {
      // enable pure mode
      ADMData::SetDefaultPureMode(true);
    }
    else if (strcmp(argv[i], "-stddefs") == 0) incstddef = true;
    else if (strcmp(argv[i], "-include") == 0) includedids[argv[++i]] = true;
    else if (file.Open(argv[i]))
    {
      // get access to ADM data object
      const ADMData *adm;
      std::map<const ADMObject *,bool> objects;
      std::vector<std::string>         lines;

      printf("Opened '%s' okay\n", argv[i]);

      if ((adm = file.GetADM()) != NULL)
      {
        EnhancedFile fp;
        std::string filename = std::string(argv[i]) + ".dot";

        if (fp.fopen(filename.c_str(), "w"))
        {
          std::vector<std::pair<const ADMObject *,const ADMObject *> > links;
          std::string str;
          bool explicitobjs = false;

          adm->GenerateReferenceList(str);

          fp.fprintf("digraph \"%s\" {\n", argv[i]);
          fp.fprintf("\toverlap=false;\n");
          fp.fprintf("\tsplines=true;\n");
          fp.fprintf("\tranksep=.75;\n");

          size_t p = 0;
          do
          {
            size_t p1 = str.find("\n", p);
            if (p1 > str.length()) p1 = str.length();

            std::string line  = str.substr(p, p1 - p);
            size_t      sep   = line.find("->");

            if (sep < std::string::npos)
            {
              std::string id1 = line.substr(0, sep);
              std::string id2 = line.substr(sep + 2);
              const ADMObject *obj1, *obj2;

              if (((obj1 = adm->GetObjectByID(id1)) != NULL) &&
                  ((obj2 = adm->GetObjectByID(id2)) != NULL))
              {
                explicitobjs |= !obj1->IsStandardDefinition();
                explicitobjs |= !obj2->IsStandardDefinition();
                links.push_back(std::pair<const ADMObject *,const ADMObject *>(obj1, obj2));
              }
            }

            p = p1 + 1;
          }
          while (p < str.length());

          incstddef |= !explicitobjs;

          {
            std::vector<std::pair<const ADMObject *,const ADMObject *> >::iterator it;
            bool done = false;

            while (!done)
            {
              done = true;

              for (it = links.begin(); it != links.end();)
              {
                const ADMObject *obj1 = it->first;
                const ADMObject *obj2 = it->second;

                if (incstddef ||
                    !obj1->IsStandardDefinition() ||
                    !obj2->IsStandardDefinition() ||
                    (objects.find(obj1) != objects.end()))
                {
                  if (objects.find(obj1) == objects.end())
                  {
                    GenerateDisplay(lines, obj1);
                    objects[obj1] = true;
                  }
                  if (objects.find(obj2) == objects.end())
                  {
                    GenerateDisplay(lines, obj2);
                    objects[obj2] = true;
                  }

                  std::string line;
                  Printf(line, "0:\"%s\" -> \"%s\";", obj1->GetID().c_str(), obj2->GetID().c_str());
                  lines.push_back(line);

                  it = links.erase(it);

                  done = false;
                }
                else ++it;
              }
            }
          }

          std::sort(lines.begin(), lines.end());

          {
            std::vector<std::string>::iterator it;
            for (it = lines.begin(); it != lines.end(); ++it) fp.fprintf("\t%s\n", it->substr(2).c_str());
          }

          fp.fprintf("}\n");
          fp.fclose();

          printf("Creating SVG graph...\n");
          std::string cmd;
          Printf(cmd, "dot -Tsvg -o \"%s.svg\" -Gepsilon=1.0 -Gmaxiter=1000 \"%s.dot\"", argv[i], argv[i]);
          if (system(cmd.c_str()) != 0) fprintf(stderr, "Command '%s' failed\n", cmd.c_str());
        }
        else fprintf(stderr, "Failed to open file '%s' for writing\n", filename.c_str());
      }
      else fprintf(stderr, "File '%s' is not an ADM-enabled BWF file!\n", argv[i]);

      file.Close();
    }
    else fprintf(stderr, "Failed to open file '%s' for reading\n", argv[i]);
  }

  return 0;
}
