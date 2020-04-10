/****************************************************************************/
//
// XmlParser.h
//
// This header defines the simple Xml parser used by the configuration class.
// This is not designed or intended to be used as a full XML implementation.
// It does not treat comments or PIs as node types; but instead is designed to
// be element-oriented for configuration purporses.
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

#ifndef __JTI_XML_PARSER_H_INCL__
#define __JTI_XML_PARSER_H_INCL__

/*----------------------------------------------------------------------------
	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <Lock.h>
#include <RefCount.h>

using namespace std;

namespace JTI_Util
{
/*----------------------------------------------------------------------------
	FORWARD DEFINITIONS
-----------------------------------------------------------------------------*/
class XmlCommentArray;
class XmlNodeArray;
class XmlAttributeMap;

/******************************************************************************/
// XmlNodeImpl
//
// This internal class provides support for a single parser node; this
// class is then wrapped in a wrapper class for ref counting.
//
/******************************************************************************/
class XmlNodeImpl : 
	public RefCountedObject<>,
	public LockableObject<>
{
// Class data
private:
	std::string name_;			// Name of the node
	std::string value_;			// Value of the node
	std::map<std::string, std::string> attribs_;// Attributes
	std::vector<XmlNodeImpl*> children_;		// Children
	std::vector<std::string> comments_;			// Comments

// Constructor
private:
	friend class XmlNode;
	friend class XmlNodeArray;
	friend class XmlCommentArray;
	XmlNodeImpl(const char* pszName="", const char* pszValue = "");
public:
	~XmlNodeImpl();

// Methods
private:
	std::string RenderXml(int level) const;

// Unavailable methods
private:
	XmlNodeImpl(const XmlNodeImpl&);
	XmlNodeImpl& operator=(const XmlNodeImpl&);
};

/******************************************************************************/
// XmlNode
//
// This class provides support for a single parser node.
//
/******************************************************************************/
class XmlNode
{
// Class data
private:
	XmlNodeImpl* pImpl_;

// Constructor
public:
	XmlNode() : pImpl_(JTI_NEW XmlNodeImpl) {/* */}
	XmlNode(const char* pszName, const char* pszValue = "") : pImpl_(JTI_NEW XmlNodeImpl(pszName, pszValue)) {/* */}
	XmlNode(const XmlNode& rhs) : pImpl_(rhs.pImpl_) { pImpl_->AddRef(); }
	~XmlNode() { pImpl_->Release(); }

	// Operators
public:
	XmlNode& operator=(const XmlNode& rhs) {
		if (this != &rhs) {
			pImpl_->Release();
			pImpl_ = rhs.pImpl_;
			pImpl_->AddRef();
		}
		return *this;
	}

	bool operator==(const XmlNode& rhs) const { return pImpl_ == rhs.pImpl_; }
	bool operator!=(const XmlNode& rhs) const { return !operator==(rhs); }
	bool operator!() { return !get_IsValid(); }

	// Properties
public:
	__declspec(property(get=get_Name, put=set_Name)) const std::string Name;
	__declspec(property(get=get_Value, put=set_Value)) const std::string Value;
	__declspec(property(get=get_hasValue)) bool HasValue;
	__declspec(property(get=get_hasAttributes)) bool HasAttributes;
	__declspec(property(get=get_Attributes)) XmlAttributeMap& Attributes;
	__declspec(property(get=get_hasChildren)) bool HasChildren;
	__declspec(property(get=get_Children)) XmlNodeArray& Children;
	__declspec(property(get=get_IsValid)) bool IsValid;
	__declspec(property(get=get_hasComments)) bool HasComments;
	__declspec(property(get=get_Comments)) XmlCommentArray& Comments;

// Accessors
public:
	bool get_IsValid() const { return !pImpl_->name_.empty(); }
	const std::string& get_Name() const { return pImpl_->name_; }
	void set_Name(const char* pszName) { pImpl_->name_ = pszName; }
	void set_Value(const char* pszValue) { pImpl_->value_ = pszValue; }
	const std::string& get_Value() const { return pImpl_->value_; }
	bool get_hasValue() const { return !pImpl_->value_.empty(); }
	bool get_hasChildren() const { return !pImpl_->children_.empty(); }
	bool get_hasComments() const { return !pImpl_->comments_.empty(); }
	bool get_hasAttributes() const { return !pImpl_->attribs_.empty(); }
	XmlCommentArray get_Comments() const;
	XmlAttributeMap get_Attributes() const;
	XmlNodeArray get_Children() const;

// Methods
public:
	XmlNode find(const char* pszPath) const;
	std::string RenderXml(int level=0) const { return pImpl_->RenderXml(level); }

// Private methods
private:
	friend class XmlNodeArray;
	friend class XmlAttributeMap;
	friend class XmlNodeArrayIterator;
	friend class XmlCommentArray;
	XmlNode(XmlNodeImpl* pNode) : pImpl_(pNode) { pImpl_->AddRef(); }
};

/******************************************************************************/
// XmlAttributeMapIterator
//
// This class provides iterator support for the attribute map
//
/******************************************************************************/
class XmlAttributeMapIterator
{
	// Class data
private:
	typedef std::map<std::string, std::string> AttributeMap;
	AttributeMap map_;
public:
	typedef AttributeMap::iterator iterator ;
	typedef AttributeMap::const_iterator const_iterator;

// Constructor
private:
	friend class XmlAttributeMap;
	XmlAttributeMapIterator(const AttributeMap& mapIn) : map_(mapIn) {/* */}

// Methods
public:
	bool empty() const { return map_.empty(); }
	int size() const { return static_cast<int>(map_.size()); }
	std::string operator[](const char* pszKey) const { 
		const_iterator it = map_.find(pszKey);
		if (it == map_.end())
			return "";
		return it->second;
	}

	iterator begin() { return map_.begin(); }
	iterator end() { return map_.end(); }
	const_iterator begin() const { return map_.begin(); }
	const_iterator end() const { return map_.end(); }
};

/******************************************************************************/
// XmlAttributeMap
//
// This class provides support for attributes
//
/******************************************************************************/
class XmlAttributeMap
{
// Class data
private:
	typedef std::map<std::string, std::string> AttributeMap;
	XmlNode node_;
	AttributeMap& mapRef_;

// Constructor
private:
	friend class XmlNode;
	XmlAttributeMap(const XmlNode& node, AttributeMap& mapIn) : node_(node), mapRef_(mapIn) {/* */}
public:
	XmlAttributeMap(const XmlAttributeMap& rhs) : node_(rhs.node_), mapRef_(rhs.mapRef_) {/* */}
	~XmlAttributeMap() {/* */}

// Operators
public:
	XmlAttributeMap& operator=(const XmlAttributeMap& rhs) {
		if (this != &rhs) {
			node_ = rhs.node_;
			mapRef_ = rhs.mapRef_;
		}
		return *this;
	}

// Properties
public:
	__declspec(property(get=empty)) bool IsEmpty;
	__declspec(property(get=size)) int Count;
	__declspec(property(get=get_Iterator)) XmlAttributeMapIterator Iterator;

	// Accessors
public:
	bool empty() const { return mapRef_.empty(); }
	int size() const { return  static_cast<int>(mapRef_.size()); }
	XmlAttributeMapIterator get_Iterator() const { return XmlAttributeMapIterator(mapRef_); }

	std::string operator[](const char* pszKey) const { 
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		return mapRef_[pszKey]; 
	}

	void add(const char* pszKey, const char* pszValue)
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		mapRef_.insert(std::make_pair(pszKey, pszValue));
	}

	void add(const char* pszKey, int value)
	{
		char chBuff[50];
		itoa(value, chBuff, 10);
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		mapRef_.insert(std::make_pair(pszKey, chBuff));
	}

	void add(const char* pszKey, long value)
	{
		char chBuff[50];
		ltoa(value, chBuff, 10);
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		mapRef_.insert(std::make_pair(pszKey, chBuff));
	}

	void remove(const char* pszKey)
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		mapRef_.erase(pszKey);
	}

	std::string find(const char* pszKey) const
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		AttributeMap::const_iterator it = mapRef_.find(pszKey);
		if (it == mapRef_.end())
			return std::string("");
		return it->second;
	}
};

/******************************************************************************/
// XmlCommentArrayIterator
//
// This class provides a safe way to iterate the collection of comments
//
/******************************************************************************/
class XmlCommentArrayIterator
{
	// Class data
private:
	typedef std::vector<std::string> CommentArray;
	CommentArray arrComments_;
public:
	typedef CommentArray::iterator iterator;
	typedef CommentArray::const_iterator const_iterator;

	// Constructor
private:
	friend class XmlCommentArray;
	XmlCommentArrayIterator(const CommentArray& arrIn) { arrComments_ = arrIn; }

	// Methods
public:
	bool empty() const { return arrComments_.empty(); }
	int size() const { return static_cast<int>(arrComments_.size()); }
	std::string operator[](int index) const { return arrComments_[index]; }

	iterator begin() { return arrComments_.begin(); }
	iterator end() { return arrComments_.end(); }
	const_iterator begin() const { return arrComments_.begin(); }
	const_iterator end() const { return arrComments_.end(); }
};

/******************************************************************************/
// XmlCommentArray
//
// This class provides support for comments
//
/******************************************************************************/
class XmlCommentArray
{
	// Class data
private:
	typedef std::vector<std::string> CommentArray;
	XmlNode node_;
	CommentArray& arrRef_;

	// Constructor
private:
	friend class XmlNode;
	XmlCommentArray(const XmlNode& node, CommentArray& arrIn) : node_(node), arrRef_(arrIn) {/* */}
public:
	XmlCommentArray(const XmlCommentArray& rhs) : node_(rhs.node_), arrRef_(rhs.arrRef_) {/* */}
	~XmlCommentArray() {/* */}

	// Operators
public:
	XmlCommentArray& operator=(const XmlCommentArray& rhs) {
		if (this != &rhs) {
			node_ = rhs.node_;
			arrRef_ = rhs.arrRef_;
		}
		return *this;
	}

	// Properties
public:
	__declspec(property(get=empty)) bool IsEmpty;
	__declspec(property(get=size)) int Count;
	__declspec(property(get=get_Iterator)) XmlCommentArrayIterator Iterator;

	// Accessors
public:
	bool empty() const { return arrRef_.empty(); }
	int size() const { return  static_cast<int>(arrRef_.size()); }
	XmlCommentArrayIterator get_Iterator() const { return XmlCommentArrayIterator(arrRef_); }

	std::string operator[](int key) const { 
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		return arrRef_[key]; 
	}

	void add(const char* pszComment)
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		arrRef_.push_back(pszComment);
	}

	void remove(int key)
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		arrRef_.erase(arrRef_.begin()+key);
	}
};

/******************************************************************************/
// XmlNodeArrayIterator
//
// This class provides a safe way to iterate the collection
//
/******************************************************************************/
class XmlNodeArrayIterator
{
// Class data
private:
	typedef std::vector<XmlNode> NodeArray;
	NodeArray arrNodes_;
public:
	typedef NodeArray::iterator iterator;
	typedef NodeArray::const_iterator const_iterator;

// Constructor
private:
	friend class XmlNodeArray;
	XmlNodeArrayIterator(XmlNode node, std::vector<XmlNodeImpl*>& arrNodes)
	{
		arrNodes_.reserve(arrNodes.size());
		for(std::vector<XmlNodeImpl*>::iterator it = arrNodes.begin(); 
			it != arrNodes.end(); ++it)
			arrNodes_.push_back(XmlNode(*it));
	}

// Methods
public:
	bool empty() const { return arrNodes_.empty(); }
	int size() const { return static_cast<int>(arrNodes_.size()); }
	XmlNode operator[](int index) const { return arrNodes_[index]; }

	iterator begin() { return arrNodes_.begin(); }
	iterator end() { return arrNodes_.end(); }
	const_iterator begin() const { return arrNodes_.begin(); }
	const_iterator end() const { return arrNodes_.end(); }
};

/******************************************************************************/
// XmlNodeArray
//
// This class provides a node array
//
/******************************************************************************/
class XmlNodeArray
{
// Class data
private:
	typedef std::vector<XmlNodeImpl*> NodeArray;
	XmlNode node_;
	NodeArray& arrNodes_;

// Constructor
private:
	friend class XmlNode;
	XmlNodeArray(const XmlNode& node, NodeArray& arrNodes) : node_(node), arrNodes_(arrNodes) {/* */}
public:
	XmlNodeArray(const XmlNodeArray& rhs) : node_(rhs.node_), arrNodes_(rhs.arrNodes_) {/* */}
	~XmlNodeArray() {/* */}

// Operators
public:
	XmlNodeArray& operator=(XmlNodeArray& rhs) {
		if (this != &rhs) {
			node_ = rhs.node_;
			arrNodes_ = rhs.arrNodes_;
		}
		return *this;
	}

// Properties
public:
	__declspec(property(get=empty)) bool IsEmpty;
	__declspec(property(get=size)) int Count;
	__declspec(property(get=get_Iterator)) XmlNodeArrayIterator Iterator;

// Accessors
public:
	bool empty() const { return arrNodes_.empty(); }
	int size() const { return  static_cast<int>(arrNodes_.size()); }

	void add(const XmlNode& xmlNode)
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		if (xmlNode == node_)
			throw(std::runtime_error("Cannot insert self into child array!"));
		XmlNodeImpl* pImpl = xmlNode.pImpl_;
		NodeArray::iterator it = std::find(arrNodes_.begin(), arrNodes_.end(), pImpl);
		if (it != arrNodes_.end())
			throw(std::runtime_error("Multiple insertion of same node within array not allowed!"));
		pImpl->AddRef();
		arrNodes_.push_back(pImpl);
	}

	bool remove(const XmlNode& xmlNode)
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		XmlNodeImpl* pImpl = xmlNode.pImpl_;
		NodeArray::iterator it = std::find(arrNodes_.begin(), arrNodes_.end(), pImpl);
		if (it != arrNodes_.end())
		{
			(*it)->Release();
			arrNodes_.erase(it);
			return true;
		}
		return false;
	}

	XmlNode find(const char* pszName) const
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		for (NodeArray::iterator it = arrNodes_.begin(); it != arrNodes_.end(); ++it)
		{
			if ((*it)->name_ == pszName)
				return XmlNode(*it);
		}
		return XmlNode();
	}

	XmlNode Index(int index) const
	{
		CCSLock<XmlNodeImpl> _lockGuard(node_.pImpl_);
		if (index < 0 || index >= static_cast<int>(arrNodes_.size()))
			throw std::out_of_range("index out of range");
		return XmlNode(arrNodes_[index]);
	}

	XmlNode operator[](int index) const
	{
		return Index(index);
	}

	XmlNodeArrayIterator get_Iterator() const { return XmlNodeArrayIterator(node_, arrNodes_); }
};

/******************************************************************************/
// XmlDocument
//
// This class provides the document owner which holds the root node.
//
/******************************************************************************/
class XmlDocument
{
// Class data
private:
	XmlNode root_;	// Root node of the document.

// Constructor
public:
	XmlDocument(const char* pszRootName=NULL);
	~XmlDocument() {/* */}

// Properties
public:
	__declspec(property(get=get_Root)) XmlNode& RootNode;
	__declspec(property(get=get_Xml)) std::string XmlText;

// Accessors
public:
	XmlNode get_Root() { return root_; }
	std::string get_Xml() const { return root_.RenderXml(); }

// Methods
public:
	bool load(const char* pszFile);
	bool save(const char* pszFile);
	void parse(const std::string& xmlBuffer);
	XmlNode find(const char* pszPath) const;
	XmlNode create(const char* pszPath, bool* pfCreated=NULL);
};

}// namespace JTI_Util

#endif // __JTI_XML_PARSER_H_INCL__