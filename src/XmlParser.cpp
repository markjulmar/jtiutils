/****************************************************************************/
//
// XmlParser.cpp
//
// This class provides a simple XML parser which has no dependancies on any
// other code base other than STL.  This removes our original dependancy on
// MSXML which is not always available on every platform.
//
// Copyright (C) 2002-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
// Revision History
// --------------------------------------------------------------
// 09/11/2002  MCS   Initial revision
//
/****************************************************************************/

/*----------------------------------------------------------------------------
	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include "stlx.h"
#include "XmlParser.h"

using namespace JTI_Util;

/*----------------------------------------------------------------------------
	CONSTANTS
-----------------------------------------------------------------------------*/
enum XmlTagType
{
	TT_ANY,
	TT_INVALID,
	TT_OPEN_TAG,
	TT_OPEN_ENDTAG,
	TT_CLOSE_TAG,
	TT_CLOSE_ENDTAG,
	TT_PI_TAG,
	TT_PI_ENDTAG,
	TT_COMMENT_TAG,
	TT_COMMENT_ENDTAG,
	TT_MARKUP_TAG,
	TT_EQUAL_SIGN,
	TT_QUOTE,
	TT_SINGLEQUOTE,
	TT_TEXT
};

/*----------------------------------------------------------------------------
	GLOBALS
-----------------------------------------------------------------------------*/
static struct {
	const char* pszTag;
	int size;
	XmlTagType id;
} gTags[] = 
{
	{ "</",	2, TT_CLOSE_TAG },
	{ "/>", 2, TT_CLOSE_ENDTAG },
	{ "<?", 2, TT_PI_TAG },
	{ "?>", 2, TT_PI_ENDTAG },
	{ "-->", 3, TT_COMMENT_ENDTAG },
	{ "<!--", 3, TT_COMMENT_TAG },
	{ "<!", 2, TT_MARKUP_TAG },
	{ "<",	1, TT_OPEN_TAG },
	{ ">",	1, TT_OPEN_ENDTAG },
	{ "=",	1, TT_EQUAL_SIGN },
	{ "\"",	1, TT_QUOTE },
	{ "'",	1, TT_SINGLEQUOTE }
};
const int TAG_COUNT = sizeofarray(gTags);

/******************************************************************************/
// InternalParser
//
// This class provides the parser support to read an XML document.
//
/******************************************************************************/
class InternalParser
{
// Class data
private:
	LPCSTR xmlData_;

// Constructor
public:
	InternalParser(LPCSTR pszData) : xmlData_(pszData) {/* */}

// Access methods
public:
	void Parse(XmlNode& node) { 
		LPCSTR pszCurrent = xmlData_;
		node = ReadElement(pszCurrent, 0);
	}

// Internal methods
private:
	XmlNode ReadElement(LPCSTR& pszCurrent, int level) const;
	bool ParseElementData(XmlNode& node, LPCSTR& pszCurrent) const;
	bool ParseAttribute(XmlNode& node, LPCSTR& pszCurrent) const;
	bool ParseText(XmlNode& node, LPCSTR& pszCurrent, XmlTagType lastTag) const;
	XmlTagType NextToken(LPCSTR& pszCurr, int& size) const;
	XmlTagType IdentifyToken(LPCSTR pszCurr, int* psize=NULL) const;
	bool SkipWhitespace(LPCSTR& pszCurrent) const;
	std::string CollectToTag(LPCSTR& pszCurrent, XmlTagType tagToEnd) const;
	std::string CollectToWhitespaceOrTag(LPCSTR& pszCurrent, XmlTagType tagToEnd = TT_ANY) const;
	void Throw(const char* pszFmt, ...) const;

public:
	static LPCSTR InternalParser::GetTagText(XmlTagType tag);
};

/*****************************************************************************
** Procedure:  InternalParser::Throw
** 
** Arguments:  'pszFormat' - Format string
** 
** Returns: void
** 
** Description: Throws an exception
**
/****************************************************************************/
void InternalParser::Throw(const char* pszFormat, ...) const
{
	va_list args;
	va_start(args, pszFormat);
	char chBuff[255];
	if (_vsnprintf(chBuff, sizeof(chBuff), pszFormat, args) == -1)
		chBuff[sizeof(chBuff)-1] = '\0';
	va_end(args);
	
	throw std::runtime_error(chBuff);

}// InternalParser::Throw

/*****************************************************************************
** Procedure:  InternalParser::GetTagText
** 
** Arguments:  'tag' - Tag to search for
** 
** Returns: Text for tag
** 
** Description: This looks up a tag and returns the appropriate text
**
/****************************************************************************/
LPCSTR InternalParser::GetTagText(XmlTagType tag)
{
	// Walk all the tags looking for a match.
	for (int i = 0; i < TAG_COUNT; ++i)
	{
		if (gTags[i].id == tag)
			return gTags[i].pszTag;
	}
	return "";

}// InternalParser::GetTagText

/*****************************************************************************
** Procedure:  InternalParser::NextToken
** 
** Arguments:  'pszCurrent' - Current position in the buffer.
**             'size' - Reference to returning size
** 
** Returns: Tag type found in buffer.
** 
** Description: This reads the next tag and returns
**
/****************************************************************************/
XmlTagType InternalParser::NextToken(LPCSTR& pszCurr, int& size) const
{
	// Skip any whitespace
	if (!SkipWhitespace(pszCurr))
		return TT_INVALID;

	// Identify the next token and adjust our buffer position.
	XmlTagType tag = IdentifyToken(pszCurr, &size);
	pszCurr += size;
	return tag;

}// InternalParser::NextToken

/*****************************************************************************
** Procedure:  InternalParser::IdentifyToken
** 
** Arguments: 'pszCurr' - Current position in buffer to identify
** 
** Returns: Token type
** 
** Description: This checks the current position for a valid token
**
/****************************************************************************/
XmlTagType InternalParser::IdentifyToken(LPCSTR pszCurr, int* psize) const
{
	// Walk all the tags looking for a match.
	for (int i = 0; i < TAG_COUNT; ++i)
	{
		if (!strncmp(gTags[i].pszTag, pszCurr, gTags[i].size))
		{
			if (psize) *psize = gTags[i].size;
			return gTags[i].id;
		}
	}

	if (psize) *psize = 0;
	return TT_TEXT;

}// InternalParser::IdentifyToken

/*****************************************************************************
** Procedure:  InternalParser::SkipWhitespace
** 
** Arguments: 'pszCurrent' - Current position in buffer (adjusted)
** 
** Returns: String read
** 
** Description: This skips any whitespace characters
**
/****************************************************************************/
bool InternalParser::SkipWhitespace(LPCSTR& pszCurrent) const
{
	// Skip whitespace
	while (*pszCurrent && isspace(*pszCurrent))
		++pszCurrent;

	return (*pszCurrent != '\0');

}// InternalParser::SkipWhitespace

/*****************************************************************************
** Procedure:  InternalParser CollectToWhitespace
** 
** Arguments: 'pszData' - Current position in buffer (adjusted)
** 
** Returns: String read
** 
** Description: This reads up to the next whitespace character
**
/****************************************************************************/
std::string InternalParser::CollectToWhitespaceOrTag(LPCSTR& pszCurrent, XmlTagType tagToEnd) const
{
	std::string text;
	while (*pszCurrent && !isspace(*pszCurrent))
	{
		XmlTagType tagThis = IdentifyToken(pszCurrent);
		if (tagThis == tagToEnd || (tagToEnd == TT_ANY && tagThis != TT_TEXT))
			break;
		text += *pszCurrent++;
	}

	if (*pszCurrent == '\0' && tagToEnd != TT_ANY)
		Throw("Hit end of stream while searching for tag %s", GetTagText(tagToEnd));

	return text;

}// InternalParser CollectToWhitespace

/*****************************************************************************
** Procedure:  InternalParser::CollectToTag
** 
** Arguments: 'pszData' - Current position in buffer (adjusted)
** 
** Returns: String read
** 
** Description: This reads up to the given tag.  This is similar to the
**              above function but ignores whitespace.
**
/****************************************************************************/
std::string InternalParser::CollectToTag(LPCSTR& pszCurrent, XmlTagType tagToEnd) const
{
	std::string text;
	while (*pszCurrent)
	{
		XmlTagType tagThis = IdentifyToken(pszCurrent);
		if (tagThis == tagToEnd)
			break;
		text += *pszCurrent++;
	}

	if (*pszCurrent == '\0')
		Throw("Hit end of stream while searching for tag %s", GetTagText(tagToEnd));

	return text;

}// InternalParser::CollectToTag

/*****************************************************************************
** Procedure:  InternalParser::ParseAttribute
** 
** Arguments: 'node' - Node to fill in 
**            'pszCurrent' - Current position in the buffer.
** 
** Returns: True/False success
** 
** Description: This parses out a single attribute from the buffer and
**              adds it to the given node.
**
/****************************************************************************/
bool InternalParser::ParseAttribute(XmlNode& node, LPCSTR& pszCurrent) const
{
	// Current position is at beginning of text.  Read until we hit a space
	// or an equal sign indicating the start of the value.
	std::string attValue, attName = CollectToWhitespaceOrTag(pszCurrent, TT_EQUAL_SIGN);

	// Make sure the next token is an equal sign!
	int size = 0;
	if (NextToken(pszCurrent, size) != TT_EQUAL_SIGN)
		Throw("Missing '=' on attribute for element %s.", node.Name.c_str());

	// Now, move until we hit a text tag.  Note whether we hit a quote/doublequote
	// tag so we can search for the end marker.
	XmlTagType tagNext = NextToken(pszCurrent, size);
	if (tagNext == TT_QUOTE || tagNext == TT_SINGLEQUOTE)
	{
		attValue = CollectToTag(pszCurrent, tagNext);
		if (NextToken(pszCurrent, size) != tagNext)
			Throw("Missing closing quote on attribute %s for element %s.", attName.c_str(), node.Name.c_str());
	}
	else if (tagNext == TT_TEXT)
		attValue = CollectToWhitespaceOrTag(pszCurrent, TT_ANY);
	else
	{
		// Invalid tag hit
		return false;
	}

	node.Attributes.add(attName.c_str(), attValue.c_str());
	return true;

}// InternalParser::ParseAttribute

/*****************************************************************************
** Procedure:  InternalParser::ParseElementData
** 
** Arguments: 'node' - Node to fill in 
**            'pszCurrent' - Current position in the buffer.
** 
** Returns: Node which was read in.
** 
** Description: This parses out a single element from the XML file.
**
/****************************************************************************/
bool InternalParser::ParseElementData(XmlNode& node, LPCSTR& pszCurrent) const
{
	// Current position is at beginning of text.  Read until we hit a space
	// or any known tag.  Element names are not allowed to contain spaces or
	// reserved keyword (tag) characters.
	node.Name = CollectToWhitespaceOrTag(pszCurrent).c_str();

	// Now parse the remaining tokens until we hit an end marker.
	while (*pszCurrent)
	{
		int size = 0;
		XmlTagType tag = NextToken(pszCurrent, size);
		switch (tag)
		{
			// Hit more text?  must be an attribute.
			case TT_TEXT:
				ParseAttribute(node, pszCurrent);
				break;

			// Got the end-tag or empty tag, backup and end.
			case TT_OPEN_ENDTAG:
			case TT_CLOSE_ENDTAG:
				pszCurrent -= size;
				return true;

			// Some unknown tag.
			default:
				return false;
		}
	}
	return false;

}// InternalParser::ParseElementData

/*****************************************************************************
** Procedure:  InternalParser::ParseText
** 
** Arguments: 'node' - Node to fill in 
**            'pszCurrent' - Current position in the buffer.
**            'lastTag' - Last tag we hit in the stream before the text.
** 
** Returns: void
** 
** Description: This parses out text from the XML stream.
**
/****************************************************************************/
bool InternalParser::ParseText(XmlNode& node, LPCSTR& pszCurrent, XmlTagType lastTag) const
{
	if (lastTag == TT_OPEN_ENDTAG)
	{
		// Collect to the next close tag.
		node.Value = CollectToTag(pszCurrent, TT_CLOSE_TAG).c_str();
	}
	else if (lastTag == TT_CLOSE_TAG)
	{
		std::string name = CollectToWhitespaceOrTag(pszCurrent, TT_OPEN_ENDTAG);
		if (name != node.Name)
			Throw("Name on end tag (%s) does not match start tag (%s).", name.c_str(), node.Name.c_str());
		return true;
	}
	else
	{
		Throw("Invalid text found (%s...), malformed document.", std::string(pszCurrent, 20).c_str());
	}

	return false;

}// InternalParser::ParseText

/*****************************************************************************
** Procedure:  InternalParser::ReadElement
** 
** Arguments:  'pszCurrent' - Current position in the buffer.
**             'level' - Current element level
** 
** Returns: Node which was read in.
** 
** Description: This parses out a single element from the XML file.
**
/****************************************************************************/
XmlNode InternalParser::ReadElement(LPCSTR& pszCurrent, int level) const
{
	XmlNode node;
	XmlTagType lastTag = TT_ANY;

	// If not a valid buffer..
	if (pszCurrent == NULL || *pszCurrent == '\0')
		Throw("Hit end of stream reading an incomplete element.");

	// Keep walking until we hit the end.
	while (*pszCurrent)
	{
		int size = 0;
		XmlTagType tag = NextToken(pszCurrent, size);
		switch (tag)
		{
			case TT_OPEN_TAG:
				if (!node.IsValid)
				{
					if (!ParseElementData(node, pszCurrent))
						Throw("Error parsing element %s", std::string(pszCurrent,20).c_str());
				}
				else
				{
					pszCurrent -= size;
					node.Children.add(ReadElement(pszCurrent, level+1));
				}
				break;

			case TT_OPEN_ENDTAG:
				if (!node.IsValid)
					Throw("Hit end-tag without fully formed element (%s...)", std::string(pszCurrent, 20).c_str());
				break;

			case TT_CLOSE_TAG:
				if (!node.IsValid)
					Throw("Hit end-tag without fully formed element (%s...)", std::string(pszCurrent, 20).c_str());
				break;

			
			case TT_QUOTE:
			case TT_SINGLEQUOTE:
				if (!node.IsValid) {
					Throw("Hit end-tag without fully formed element (%s...)", std::string(pszCurrent, 20).c_str());
					break;
				}
				pszCurrent -= size;

				// Fall through intentional.

			case TT_TEXT:
				if (ParseText(node, pszCurrent, lastTag))
				{
					// Parse indicated end tag found for this element; make sure
					// and then exit.
					if (NextToken(pszCurrent, size) != TT_OPEN_ENDTAG)
						Throw("Missing end-tag on element %s", node.Name.c_str());
					return node;
				}
				break;

			case TT_CLOSE_ENDTAG:
				return node;

			case TT_PI_TAG:
				CollectToTag(pszCurrent, TT_PI_ENDTAG);
				break;
			case TT_COMMENT_TAG:
				node.Comments.add(CollectToTag(pszCurrent, TT_COMMENT_ENDTAG).c_str());
				break;
			case TT_MARKUP_TAG:
				CollectToTag(pszCurrent, TT_OPEN_ENDTAG);
				break;

			// Ignore
			case TT_PI_ENDTAG:
			case TT_COMMENT_ENDTAG:
				break;

			default:
				return node;
		}

		// Save off the last tag we hit.
		lastTag = tag;
	}

	return node;

}// InternalParser::ReadElement

/*****************************************************************************
** Procedure:  XmlNodeImpl::XmlNodeImpl
** 
** Arguments:  'name' - Name of the node
**             'value' - Value of the node
** 
** Returns: void
** 
** Description: This creates a new XML node
**
/****************************************************************************/
XmlNodeImpl::XmlNodeImpl(const char* pszName, const char* pszValue) : 
	name_((pszName) ? pszName : ""), value_((pszValue) ? pszValue : "") 
{
	// Strip any spaces from the name.
	if (name_.find(' ') != string::npos)
	{
		std::string::size_type pos; 
		while ((pos = name_.find_first_of(' ')) != std::string::npos)
			name_.replace(pos, 1, "_");		
	}

}// XmlNodeImpl::XmlNodeImpl

/*****************************************************************************
** Procedure:  XmlNodeImpl::~XmlNodeImpl
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Destructor for the node
**
/****************************************************************************/
XmlNodeImpl::~XmlNodeImpl()
{
	std::for_each(children_.begin(), children_.end(), stdx::delptr<XmlNodeImpl*>());

}// XmlNodeImpl::~XmlNodeImpl

/*****************************************************************************
** Procedure:  XmlNodeImpl::RenderXml
** 
** Arguments:  void
** 
** Returns: string representation of the node.
** 
** Description: This renders the XML for a given node.
**
/****************************************************************************/
std::string XmlNodeImpl::RenderXml(int level) const
{
	std::stringstream stm;

	// Spit out the default PI header
	if (level == 0)
		stm << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;

	// Add the comments above the tag itself.
	if (!comments_.empty())
	{
		for (std::vector<std::string>::const_iterator it = comments_.begin(); 
				it != comments_.end(); ++it)
		{
			stm << std::string(level,'\t')
				<< InternalParser::GetTagText(TT_COMMENT_TAG)
				<< *it
				<< InternalParser::GetTagText(TT_COMMENT_ENDTAG)
				<< endl;
		}
	}

	// Add the opening tag
	stm << std::string(level,'\t') << InternalParser::GetTagText(TT_OPEN_TAG) << name_;
	if (!attribs_.empty())
	{
		CCSLock<XmlNodeImpl> aLock(this);
		for (std::map<std::string, std::string>::const_iterator it = attribs_.begin(); 
			 it != attribs_.end(); ++it)
		{
			stm << " " << it->first << InternalParser::GetTagText(TT_EQUAL_SIGN);
			std::string sQuote = (it->second.find("\"") == std::string::npos) ?
				InternalParser::GetTagText(TT_QUOTE) : 
				InternalParser::GetTagText(TT_SINGLEQUOTE);
			stm << sQuote << it->second << sQuote;
		}
	}
	
	if (!value_.empty())
	{
		stm << InternalParser::GetTagText(TT_OPEN_ENDTAG) << value_ 
			<< InternalParser::GetTagText(TT_CLOSE_TAG) << name_ 
			<< InternalParser::GetTagText(TT_OPEN_ENDTAG) << std::endl;
	}
	else
	{
		if (!children_.empty())
		{
			stm << InternalParser::GetTagText(TT_OPEN_ENDTAG) << endl;
			CCSLock<XmlNodeImpl> aLock(this);
			for (std::vector<XmlNodeImpl*>::const_iterator it = children_.begin(); 
				 it != children_.end(); ++it)
				stm << (*it)->RenderXml(level+1);
			stm << std::string(level,'\t') << InternalParser::GetTagText(TT_CLOSE_TAG) 
				<< name_ << InternalParser::GetTagText(TT_OPEN_ENDTAG) << std::endl;
		}
		else
			stm << InternalParser::GetTagText(TT_CLOSE_ENDTAG) << std::endl;
	}

	return stm.str();

}// XmlNodeImpl::RenderXml

/*****************************************************************************
** Procedure:  XmlNode::get_Attributes
** 
** Arguments:  void
** 
** Returns: Wrapped attribute map
** 
** Description: Returns the attribute map for the XmlNode list.
**
/****************************************************************************/
XmlAttributeMap XmlNode::get_Attributes() const 
{ 
	return XmlAttributeMap(*this, pImpl_->attribs_); 

}// XmlNode::get_Attributes

/*****************************************************************************
** Procedure:  XmlNode::get_Children
** 
** Arguments:  void
** 
** Returns: Wrapped child node array
** 
** Description: Returns the node array for the XmlNode list.
**
/****************************************************************************/
XmlNodeArray XmlNode::get_Children() const 
{ 
	return XmlNodeArray(*this, pImpl_->children_); 

}// XmlNode::get_Children

/*****************************************************************************
** Procedure:  XmlNode::get_Comments
** 
** Arguments:  void
** 
** Returns: Returns the array of comments.
** 
** Description: Returns the string array for the comment list
**
/****************************************************************************/
XmlCommentArray XmlNode::get_Comments() const
{
	return XmlCommentArray(*this, pImpl_->comments_);

}// XmlNode::get_Comments

/*****************************************************************************
** Procedure:  XmlNode::Find
** 
** Arguments:  'pszPath' - Path of the node to locate
** 
** Returns: Located node
** 
** Description: Locates a subnode from this point
**
/****************************************************************************/
XmlNode XmlNode::find(const char* pszPath) const
{
	if (pszPath == NULL || *pszPath == '\0' || !HasChildren)
		return XmlNode();

	std::vector<std::string> arrPath;
	stdx::split(pszPath, "/\\", std::back_inserter(arrPath));
	if (arrPath.empty())
		return XmlNode();

	XmlNodeImpl* pcurrNode = pImpl_;
	for (std::vector<std::string>::iterator it = arrPath.begin(); it != arrPath.end(); ++it)
	{
		// If the current node has no children then we are done.
		if (pcurrNode->children_.empty())
			return XmlNode();

		// Search the array.
		std::vector<XmlNodeImpl*>::iterator itNode = pcurrNode->children_.begin();
		for (; itNode != pcurrNode->children_.end(); ++itNode)
		{
			if ((*itNode)->name_ == (*it)) {
				pcurrNode = (*itNode);
				break;
			}
		}

		if (itNode == pcurrNode->children_.end())
			return XmlNode();
	}

	// Return the found node
	return XmlNode(pcurrNode);

}// XmlNode::find

/*****************************************************************************
** Procedure:  XmlDocument::XmlDocument
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Constructor for the Xml parser object
**
/****************************************************************************/
XmlDocument::XmlDocument(const char* pszRootName) : root_(pszRootName)
{ 
}// XmlDocument::XmlDocument

/*****************************************************************************
** Procedure:  XmlDocument::find
** 
** Arguments:  'pszPath' - Path of the node to locate
** 
** Returns: Located node
** 
** Description: Constructor for the Xml parser object
**
/****************************************************************************/
XmlNode XmlDocument::find(const char* pszPath) const
{
	if (pszPath == NULL || *pszPath == '\0')
		return XmlNode();

	// Skip any leading seperator.  Standard XML syntax allows for
	// root node to be identified this way.
	if (*pszPath == '/' || *pszPath == '\\')
		++pszPath;

	std::vector<std::string> arrPath;
	stdx::split(pszPath, "/\\", std::back_inserter(arrPath));

	if (arrPath.empty() || arrPath[0] != root_.Name)
		return XmlNode();

	XmlNode currNode = root_;
	if (arrPath.size() > 1)
	{
		for (std::vector<std::string>::iterator it = arrPath.begin()+1; it != arrPath.end(); ++it)
		{
			if (it->empty())
				continue;
			if (!currNode.IsValid)
				break;
			if (!currNode.HasChildren)
				return XmlNode();
			currNode = currNode.Children.find(it->c_str());
		}
	}
	return currNode;

}// XmlDocument::find

/*****************************************************************************
** Procedure:  XmlDocument::load
** 
** Arguments:  'pszFile' - Filename to load
** 
** Returns: true/false whether load was successful.
** 
** Description: This loads an XML file into memory.
**
/****************************************************************************/
bool XmlDocument::load(const char* pszFile)
{
	const int PATTERN_COUNT = 4;
	static unsigned char gszPatterns[PATTERN_COUNT][5] = {
		{ 3, 0xEF, 0xBB, 0xBF },			// UTF-8
		{ 2, 0xFE, 0xFF },					// UTF-16 UCS-2 little endian
		{ 4, 0xFF, 0xFE, 0x00, 0x00 },		// UTF-16/32 UCS-2/4 
		{ 4, 0x00, 0x00, 0xFE, 0xFF }		// UTF-32 UCS-4 big endian
	};

	// Read the file into a string.
	std::string xmlBuffer;
	std::ifstream ifs(pszFile);
	std::istreambuf_iterator<char> itBegin(ifs), itEnd;

	// Skip any XML byte ordering marker
	if (itBegin != itEnd)
	{
		unsigned char ch = static_cast<unsigned char>(*itBegin);
		for (int i = 0; i < PATTERN_COUNT; ++i)
		{
			if (ch == gszPatterns[i][1])
			{
				for (int x = 1; x <= gszPatterns[i][0]; ++x && itBegin != itEnd, ++itBegin)
				{
					ch = static_cast<unsigned char>(*itBegin);
					if (ch != gszPatterns[i][x])
						break;
				}
				break;
			}
		}
	}

	// Load the full XML text.
	xmlBuffer = std::string(itBegin, itEnd);

	// Now parse out the buffer
	parse(xmlBuffer);
	return true;

}// XmlDocument::load

/*****************************************************************************
** Procedure:  XmlDocument::save
** 
** Arguments:  'pszFile' - Filename to save to
** 
** Returns: true/false whether save was successful.
** 
** Description: This saves an XML file onto disk.
**
/****************************************************************************/
bool XmlDocument::save(const char* pszFile)
{
	std::ofstream ofs(pszFile);
	ofs << 0xEF << 0xBB << 0xBF;	 // UTF-8 byte order marker
	ofs << root_.RenderXml();

	return true;

}// XmlDocument::save

/*****************************************************************************
** Procedure:  XmlDocument::parse
** 
** Arguments:  'xmlBuffer' - XML data to load
** 
** Returns: true/false whether parse was successful.
** 
** Description: This parses out an XML file
**
/****************************************************************************/
void XmlDocument::parse(const std::string& xmlBuffer)
{
	// No data? ignore.
	if (xmlBuffer.empty())
		return;

	InternalParser parser(xmlBuffer.c_str());
	parser.Parse(root_);

}// XmlDocument::parse

/*****************************************************************************
** Procedure:  XmlDocument::create
** 
** Arguments:  'pszPath' - Path of the node to create
** 
** Returns: Located or created node
** 
** Description: Constructor for the Xml parser object
**
/****************************************************************************/
XmlNode XmlDocument::create(const char* pszPath, bool* pfCreated)
{
	// Init entry
	if (pfCreated)
		*pfCreated = false;

	if (pszPath == NULL || *pszPath == '\0')
		throw std::runtime_error("Invalid path passed into XmlDocument::Create");

	// Skip any leading seperator.  Standard XML syntax allows for
	// root node to be identified this way.
	if (*pszPath == '/' || *pszPath == '\\')
		++pszPath;

	std::vector<std::string> arrPath;
	stdx::split(pszPath, "/\\", std::back_inserter(arrPath));

	if (arrPath.empty() || arrPath[0] != root_.Name)
		return XmlNode();

	XmlNode currNode = root_;
	if (arrPath.size() > 1)
	{
		for (std::vector<std::string>::iterator it = arrPath.begin()+1; it != arrPath.end(); ++it)
		{
			XmlNode foundNode = XmlNode();
			if (currNode.HasChildren)
				foundNode = currNode.Children.find(it->c_str());
			if (foundNode == XmlNode())
			{
				foundNode = XmlNode(it->c_str());
				currNode.Children.add(foundNode);
				if (pfCreated) *pfCreated = true;
			}
			currNode = foundNode;
		}
	}
	return currNode;

}// XmlDocument::create
