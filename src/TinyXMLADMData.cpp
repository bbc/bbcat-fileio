
#include <math.h>

#include <tinyxml.h>

#define BBCDEBUG_LEVEL 1
#include "TinyXMLADMData.h"

BBC_AUDIOTOOLBOX_START

BBC_AUDIOTOOLBOX_KEEP(TinyXMLADMData);

const bool TinyXMLADMData::__registered = TinyXMLADMData::Register();

TinyXMLADMData::TinyXMLADMData(const std::string& standarddefinitionsfile) : XMLADMData()
{
  LoadStandardDefinitions(standarddefinitionsfile);
}

TinyXMLADMData::~TinyXMLADMData()
{
  // no special destruction required
}

/*--------------------------------------------------------------------------------*/
/** Register function - this is called automatically
 */
/*--------------------------------------------------------------------------------*/
bool TinyXMLADMData::Register()
{
  static bool registered = false;
  if (!registered) {
    RegisterProvider(&__Creator);
    registered = true;
  }

  return registered;
}

/*--------------------------------------------------------------------------------*/
/** Decode XML string as ADM
 *
 * @param data ptr to string containing ADM XML (MUST be terminated)
 *
 * @return true if XML decoded correctly
 */
/*--------------------------------------------------------------------------------*/
bool TinyXMLADMData::TranslateXML(const char *data)
{
  TiXmlDocument doc;
  const TiXmlNode *node, *formatNode = NULL;
  bool success = false;

  BBCDEBUG3(("XML: %s", data));

  doc.Parse(data);

  // dig to correct location of audioFormatExtended section
  if (((node = FindElement(&doc, "ebuCoreMain")) != NULL) ||
      ((node = FindElement(&doc, "ituADM")) != NULL))
  {
    // collect non-ADM objects from parent node
    CollectNonADMObjects(node);

    if ((formatNode = FindElement(node, "audioFormatExtended")) == NULL)
    {
      // if format node not a top level, dig deeper
      if ((node = FindElement(node, "coreMetadata")) != NULL)
      {
        // collect non-ADM objects from parent node
        CollectNonADMObjects(node);

        if ((node = FindElement(node, "format")) != NULL)
        {
          // collect non-ADM objects from parent node
          CollectNonADMObjects(node);

          formatNode = FindElement(node, "audioFormatExtended");
        }
        else BBCERROR("Failed to find format element");
      }
      else BBCERROR("Failed to find coreMetadata element");
    }

    // if format node found, decode it
    if (formatNode)
    {
      CollectObjects(formatNode);
      
      success = true;
    }
  }
  else BBCERROR("Failed to find ebuCoreMain or ituADM nodes");

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Parse the XML section header
 *
 * @param header header object ot be populated
 * @param type XML type of object
 * @param userdata user suppled data
 *
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseHeader(ADMHEADER& header, const std::string& type, void *userdata)
{
  const TiXmlNode      *node = (const TiXmlNode *)userdata;
  const TiXmlAttribute *attr; 

  header.type = type;
  header.id   = StringFrom(GetNanosecondTicks(), "016x");
  
  for (attr = node->ToElement()->FirstAttribute(); attr; attr = attr->Next())
  {
    std::string attr_name = attr->Name();

    // find '<type>Name', '<type>ID' or 'UID' (special for AudioTrack)
    if (attr_name == (type + "Name"))
    {
      header.name = attr->Value();
    }
    else if (attr_name == (type + "ID"))
    {
      header.id   = attr->Value();
    }
    else if (attr_name == "UID")
    {
      header.id   = attr->Value();
    }
  }

  BBCDEBUG2(("Parse header (type='%s', id='%s', name='%s')", header.type.c_str(), header.id.c_str(), header.name.c_str()));
}

/*--------------------------------------------------------------------------------*/
/** Parse value (and its attributes) into a list of XML values
 *
 * @param obj ADM object
 * @param userdata implementation specific object data
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseValue(ADMObject *obj, void *userdata)
{
  ParseValue(obj->ToString(), obj->GetValues(), userdata);
}

/*--------------------------------------------------------------------------------*/
/** Parse value (and its attributes) into a list of XML values
 *
 * @param name name of object (for BBCDEBUGGING only)
 * @param values list of XML values to be added to
 * @param userdata implementation specific object data
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseValue(const std::string& name, XMLValues& values, void *userdata)
{
  const TiXmlNode      *node    = (const TiXmlNode *)userdata;
  const TiXmlNode      *subnode = node->FirstChild();
  const TiXmlAttribute *attr; 
  XMLValue value;

  UNUSED_PARAMETER(name);

  value.attr = false;
  value.name = node->Value();
  if (subnode && (subnode->Type() != TiXmlNode::TINYXML_ELEMENT)) value.value = subnode->Value();

  BBCDEBUG3(("%s: %s='%s', attrs:",
          name.c_str(),
          value.name.c_str(), value.value.c_str()));
    
  for (attr = node->ToElement()->FirstAttribute(); attr; attr = attr->Next())
  {
    value.attrs[attr->Name()] = attr->Value();

    BBCDEBUG3(("\t%s='%s'", attr->Name(), attr->Value()));
  }

  XMLValues subvalues;
  for (; subnode; subnode = subnode->NextSibling())
  {
    if (subnode->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      ParseValue(name + ":subvalue", subvalues, (void *)subnode);
    }
  }
  // add subvalues to value
  value.AddSubValues(subvalues);
  
  values.AddValue(value);
}

/*--------------------------------------------------------------------------------*/
/** Parse attributes into a list of XML values
 *
 * @param obj ADM object
 * @param userdata implementation specific object data
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseAttributes(ADMObject *obj, void *userdata)
{
  ParseAttributes(obj->ToString(), obj->GetType(), obj->GetValues(), userdata);
}

/*--------------------------------------------------------------------------------*/
/** Parse attributes into a list of XML values
 *
 * @param name name of object (for BBCDEBUGGING only)
 * @param type object type - necessary to prevent object name and ID being added
 * @param values list of XML values to be populated
 * @param userdata implementation specific object data
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseAttributes(const std::string& name, const std::string& type, XMLValues& values, void *userdata)
{
  const TiXmlNode      *node = (const TiXmlNode *)userdata;
  const TiXmlAttribute *attr; 

  UNUSED_PARAMETER(name);
  
  for (attr = node->ToElement()->FirstAttribute(); attr; attr = attr->Next())
  {
    std::string attr_name = attr->Name();

    // ignore header values previously found
    if ((attr_name != (type + "Name")) &&
        (attr_name != (type + "ID")) &&
        (attr_name != "UID"))
    {
      XMLValue value;
            
      value.attr  = true;
      value.name  = attr_name;
      value.value = attr->Value();

      values.AddValue(value);

      BBCDEBUG3(("%s: %s='%s'",
              name.c_str(),
              attr_name.c_str(), attr->Value()));
    }
  }
}

/*--------------------------------------------------------------------------------*/
/** Parse audioBlockFormat XML object
 *
 * @param name parent name (for BBCDEBUGGING only)
 * @param obj audioBlockFormat object
 * @param userdata user suppled data
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseValues(const std::string& name, ADMAudioBlockFormat *obj, void *userdata)
{
  const TiXmlNode *node = (const TiXmlNode *)userdata;
  const TiXmlNode *subnode;
  XMLValues       values;
            
  // parse attributes
  ParseAttributes(name, obj->GetType(), values, userdata);

  // parse subnode elements and create values from them
  for (subnode = node->FirstChild(); subnode; subnode = subnode->NextSibling())
  {
    if (subnode->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      ParseValue(name, values, (void *)subnode);
    }
  }
  
  // set values in object
  obj->SetValues(values);
}

/*--------------------------------------------------------------------------------*/
/** Parse attributes and subnodes as values
 *
 * @param obj object to read values from
 * @param userdata user suppled data
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::ParseValues(ADMObject *obj, void *userdata)
{
  // if the supplied ADM object is an AudioChannelFormat, AudioBlockFormat sub-sections will be parsed
  ADMAudioChannelFormat *channel = dynamic_cast<ADMAudioChannelFormat *>(obj);
  const TiXmlNode       *node    = (const TiXmlNode *)userdata;
  const TiXmlNode       *subnode;

  // parse attributes
  ParseAttributes(obj, userdata);

  // parse subnode elements and create values from them
  for (subnode = node->FirstChild(); subnode; subnode = subnode->NextSibling())
  {
    if (subnode->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      std::string name = subnode->Value();

      if (name == ADMAudioBlockFormat::Type)
      {
        // AudioBlockFormat sections are handled differently...

        if (channel)
        {
          // if the supplied ADM object is an AudioChannelFormat, parse this subnode as an AudioBlockFormat section
          ADMAudioBlockFormat *block;

          // parse this subnode as a section
          if ((block = new ADMAudioBlockFormat) != NULL)
          {
            ParseValues(obj->ToString() + ":BlockFormat", block, (void *)subnode);
            channel->Add(block);
          }
          else BBCERROR("Parsed object was not an AudioBlockFormat object");
        }
        else BBCERROR("No AudioChannelFormat for found AudioBlockFormat");
      }
      else ParseValue(obj, (void *)subnode);
    }
  }
}

/*--------------------------------------------------------------------------------*/
/** Find named element in subnode list
 *
 * @param node parent node
 * @param name name of subnode to find
 */
/*--------------------------------------------------------------------------------*/
const TiXmlNode *TinyXMLADMData::FindElement(const TiXmlNode *node, const std::string& name)
{
  const TiXmlNode *subnode; 

  // iterate through subnodes to find named element
  for (subnode = node->FirstChild(); subnode; subnode = subnode->NextSibling())
  {
    if ((subnode->Type() == TiXmlNode::TINYXML_ELEMENT) &&
        (name == subnode->Value()))
    {
      return subnode;
    }
  }

  return NULL;
}

/*--------------------------------------------------------------------------------*/
/** Recursively collect all objects in tree and parse them
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::CollectObjects(const TiXmlNode *node)
{
  // first collect ADM objects
  CollectADMObjects(node);
  // then collect non-ADM objects
  CollectNonADMObjects(node);
}

/*--------------------------------------------------------------------------------*/
/** Recursively collect all ADM objects in tree and parse them
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::CollectADMObjects(const TiXmlNode *node)
{
  const TiXmlNode *subnode; 

  for (subnode = node->FirstChild(); subnode; subnode = subnode->NextSibling())
  {
    if (subnode->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      if (ValidType(subnode->Value()))
      {
        Parse(subnode->Value(), (void *)subnode);

        // ONLY collect ADM objects below this point
        CollectADMObjects(subnode);
      }
    }
  }
}

/*--------------------------------------------------------------------------------*/
/** Recursively collect all non-ADM objects in tree and parse them
 */
/*--------------------------------------------------------------------------------*/
void TinyXMLADMData::CollectNonADMObjects(const TiXmlNode *node)
{
  const TiXmlNode *subnode; 
  std::string parentname = node->Value();

  // for root node, use empty name
  if ((parentname == "ebuCoreMain") ||
      (parentname == "ituADM")) parentname = "";
  
  for (subnode = node->FirstChild(); subnode; subnode = subnode->NextSibling())
  {
    if (subnode->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      const std::string name = subnode->Value();
      
      if (!ValidType(name) &&
          (name != "audioFormatExtended") &&
          (name != "coreMetadata") &&
          (name != "format")) // not an ADM type
      {
        BBCDEBUG4(("Parsing non-ADM type '%s'", name.c_str()));
        
        ParseValue(name, nonadmxml[parentname], (void *)subnode);
      }
    }
  }
}

BBC_AUDIOTOOLBOX_END
