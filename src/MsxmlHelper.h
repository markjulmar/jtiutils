/****************************************************************************/
//
// MsxmlHelper.h
//
// This file describes an XML helper class for reading/writing XML files
// using MSXML.DLL
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_MSXML_HELPER_INC__
#define __JTI_MSXML_HELPER_INC__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <wtypes.h>
#include <string>
#include <stdexcept>
#import <msxml3.dll>

#pragma warning(disable:4100)

namespace JTI_Util
{
/****************************************************************************/
// XmlDOM
//
// Class used to wrap MSXML in a more friendly and usable fashion.
//
/*****************************************************************************/
class XmlDOM
{
// Class data
public:
	bool hasSchema_;
	std::wstring nameSpace_;
	MSXML2::IXMLDOMDocumentPtr xmlDOM_;

// Constructor
public:
	XmlDOM() : hasSchema_(false) { loadMSXML(); }
	explicit XmlDOM(const wchar_t* pszXml) : hasSchema_(false) { loadMSXML(); xmlDOM_->loadXML(pszXml); }
	~XmlDOM() {}
	XmlDOM(const XmlDOM& rhs) : xmlDOM_(rhs.xmlDOM_) {/* */}
	XmlDOM& operator=(const XmlDOM& rhs) {
		if (this != &rhs)
			xmlDOM_ = rhs.xmlDOM_;
		return *this;
	}

// Properties
public:
	__declspec (property(get=get_WriteInlineSchema, put=put_WriteInlineSchema)) bool InlineSchema;

// Property helpers
public:
	bool get_WriteInlineSchema() const { return hasSchema_; }
	void put_WriteInlineSchema(bool f) { hasSchema_ = !f; }

// Access methods
public:
	MSXML2::IXMLDOMDocumentPtr& domPtr() { return xmlDOM_; }

	bool LoadXML(LPCWSTR pszXML, bool validateOnParse=false)
	{
		try {
			xmlDOM_->validateOnParse = validateOnParse;
			return (xmlDOM_->loadXML(pszXML) == VARIANT_TRUE);
		} catch(const _com_error&) { return false; }
		
	}

	bool Save(LPCWSTR pszFile)
	{
		try {
			return (xmlDOM_->save(pszFile) == VARIANT_TRUE);
		} catch(const _com_error&) { return false; }
	}

	bool Load(LPCWSTR pszFile, bool validateOnParse=false)
	{
		try {
			xmlDOM_->validateOnParse = validateOnParse;
			return (xmlDOM_->load(pszFile) == VARIANT_TRUE);
		} catch(const _com_error&) { return false; }
	}

	void AddXMLHeader()
	{
		MSXML2::IXMLDOMProcessingInstructionPtr piProc = xmlDOM_->createProcessingInstruction(L"xml", L"version=\"1.0\"");
		if (piProc)
		{
			if (xmlDOM_->firstChild != NULL)
			{
				xmlDOM_->insertBefore(piProc, _variant_t(xmlDOM_->firstChild));
			}
			else
			{
				xmlDOM_->appendChild(piProc);
			}
		}
	}

	MSXML2::IXMLDOMNodePtr CreateRoot(LPCWSTR pszName, LPCWSTR pszNamespace=NULL, LPCWSTR pszSchema=NULL) 
	{
		if (pszNamespace != NULL) nameSpace_ = pszNamespace;
		MSXML2::IXMLDOMNodePtr nodeRoot = xmlDOM_->appendChild(CreateNode(pszName,pszNamespace));
		if (nodeRoot != NULL && pszSchema != NULL)
			AddSchema(nodeRoot, pszSchema);
		return nodeRoot;
	}

	MSXML2::IXMLDOMNodePtr GetRoot() const
	{
		MSXML2::IXMLDOMNodeListPtr childNodes = xmlDOM_->childNodes;
		if (childNodes->length > 0)
		{
			for (long i = 0; i < childNodes->length; ++i)
			{
				MSXML2::IXMLDOMNodePtr nodeRoot = childNodes->item[i];
				if (nodeRoot->nodeType == NODE_ELEMENT)
					return nodeRoot;
			}
		}
		return NULL;
	}

	void AddSchema(MSXML2::IXMLDOMNodePtr& nodeRoot, LPCWSTR pszSchemaInfo)
	{
		AppendAttribute(nodeRoot, L"xmlns:xsi", L"http://www.w3.org/2001/XMLSchema-instance", VT_BSTR);
		if (!nameSpace_.empty())
			AppendAttribute(nodeRoot, L"xsi:schemaLocation", pszSchemaInfo, VT_BSTR);
		else
			AppendAttribute(nodeRoot, L"xsi:noNamespaceSchemaLocation", pszSchemaInfo, VT_BSTR);
		hasSchema_ = true;
	}

	MSXML2::IXMLDOMNodePtr CreateNode(LPCWSTR pszName, LPCWSTR pszNamespace=NULL)
	{
		_bstr_t bstrNamespace;
		if (pszNamespace != NULL) 
			bstrNamespace = pszNamespace;
		else if (!nameSpace_.empty())
			bstrNamespace = nameSpace_.c_str();
		return xmlDOM_->createNode(_variant_t(static_cast<long>(MSXML2::NODE_ELEMENT)), pszName, bstrNamespace);
	}

	MSXML2::IXMLDOMNodePtr GetNodeByName(LPCWSTR pszName) const
	{
		return GetNodeByName(xmlDOM_, pszName);
	}

	MSXML2::IXMLDOMNodePtr GetNodeByName(MSXML2::IXMLDOMNodePtr piRoot, LPCWSTR pszName) const
	{
		return piRoot->selectSingleNode(pszName);
	}

	MSXML2::IXMLDOMNodeListPtr GetNodeListByName(LPCWSTR pszName) const
	{
		return GetNodeListByName(xmlDOM_, pszName);
	}

	MSXML2::IXMLDOMNodeListPtr GetNodeListByName(MSXML2::IXMLDOMNodePtr piRoot, LPCWSTR pszName) const
	{
		return piRoot->selectNodes(pszName);
	}

	_variant_t GetAttributeByName(MSXML2::IXMLDOMNodePtr piRoot, LPCWSTR pszName) const
	{
		MSXML2::IXMLDOMNamedNodeMapPtr piMap = piRoot->attributes;
		if (piMap != NULL && piMap->length > 0)
		{
			MSXML2::IXMLDOMNodePtr piNode = piMap->getNamedItem(pszName);
			if (piNode != NULL)
				return piNode->nodeValue;
		}
		return vtMissing;
	}

	_variant_t GetValueByName(LPCWSTR pszName) const
	{
		return GetValueByName(xmlDOM_, pszName);
	}

	_variant_t GetValueByName(MSXML2::IXMLDOMNodePtr piRoot, LPCWSTR pszName) const
	{
		MSXML2::IXMLDOMNodePtr piNode = piRoot->selectSingleNode(pszName);
		if (piNode != NULL)
		{
			// Element?
			if (piNode->nodeType == MSXML2::NODE_ELEMENT)
			{
				// Search for child "#text" value
				MSXML2::IXMLDOMNodeListPtr piList = piNode->childNodes;
				if (piList != NULL)
				{
					MSXML2::IXMLDOMNodePtr piNode = piList->nextNode();
					while (piNode != NULL)
					{
						if (piNode->nodeType == MSXML2::NODE_TEXT)
							return piNode->nodeValue;
						piNode = piList->nextNode();
					}
				}
			}
		}

		return vtMissing;
	}

	MSXML2::IXMLDOMAttributePtr CreateAttribute(LPCWSTR pszName, _variant_t vt, VARTYPE varType=VT_ERROR)
	{
		MSXML2::IXMLDOMAttributePtr spAttrib = xmlDOM_->createAttribute(pszName);
		spAttrib->nodeValue = vt;
		return spAttrib;
	}

	MSXML2::IXMLDOMAttributePtr AppendAttribute(MSXML2::IXMLDOMElementPtr& nodeRoot, MSXML2::IXMLDOMAttributePtr spAttrib)
	{
		if (nodeRoot->getAttributeNode(spAttrib->name))
			nodeRoot->removeAttribute(spAttrib->name);
		return nodeRoot->setAttributeNode(spAttrib);
	}

	bool AppendAttribute(MSXML2::IXMLDOMElementPtr& nodeRoot, LPCWSTR pszName, _variant_t vt, VARTYPE varType=VT_ERROR)
	{
		return (nodeRoot->setAttribute(pszName, vt) == S_OK);
	}

	MSXML2::IXMLDOMAttributePtr AppendAttribute(MSXML2::IXMLDOMNodePtr& nodeRoot, MSXML2::IXMLDOMAttributePtr spAttrib)
	{
		MSXML2::IXMLDOMElementPtr nodeElem = nodeRoot;
		if (nodeElem->getAttributeNode(spAttrib->name))
			nodeElem->removeAttribute(spAttrib->name);
		return nodeElem->setAttributeNode(spAttrib);
	}

	bool AppendAttribute(MSXML2::IXMLDOMNodePtr& nodeRoot, LPCWSTR pszName, _variant_t vt, VARTYPE varType=VT_ERROR)
	{
		MSXML2::IXMLDOMElementPtr nodeElem = nodeRoot;
		return (nodeElem->setAttribute(pszName, vt) == S_OK);
	}

	MSXML2::IXMLDOMNodePtr AppendNode(MSXML2::IXMLDOMNodePtr& nodeRoot, LPCWSTR pszName, LPCWSTR pszNamespace=NULL)
	{
		return nodeRoot->appendChild(CreateNode(pszName, pszNamespace));
	}

	void AppendNode(MSXML2::IXMLDOMNodePtr& nodeRoot, MSXML2::IXMLDOMNodePtr& nodeChild)
	{
		nodeRoot->appendChild(nodeChild);
	}

	void AppendNode(MSXML2::IXMLDOMNodePtr& nodeRoot, LPCWSTR pszName, _variant_t vt, VARTYPE vType=VT_ERROR, LPCWSTR pszNamespace=NULL)
	{
		MSXML2::IXMLDOMNodePtr spNode = nodeRoot->appendChild(CreateNode(pszName, pszNamespace));
		SetNodeValueType(spNode,vt, vType);
	}

	void SetNodeValueType(MSXML2::IXMLDOMNodePtr& spNode, _variant_t vt, VARTYPE vType=VT_ERROR)
	{
		// Element?
		if (spNode->nodeType == MSXML2::NODE_ELEMENT && spNode->hasChildNodes() == VARIANT_TRUE)
		{
			// Remove for all child "#text" value
			MSXML2::IXMLDOMNodeListPtr piList = spNode->childNodes;
			if (piList != NULL)
			{
				MSXML2::IXMLDOMNodePtr piNode = piList->nextNode();
				while (piNode != NULL)
				{
					if (piNode->nodeType == MSXML2::NODE_TEXT)
						spNode->removeChild(piNode);
					piNode = piList->nextNode();
				}
			}
		}

		// Append a new child text node.
		MSXML2::IXMLDOMNodePtr spNodeText = xmlDOM_->createNode(_variant_t(static_cast<long>(MSXML2::NODE_TEXT)), "","");
		spNode->appendChild(spNodeText);

		if (!hasSchema_)
		{
			switch(vType)
			{
				case VT_BSTR: spNode->PutdataType(L"string"); break;
				case VT_UI4:  spNode->PutdataType(L"ui4"); break;
				case VT_I4:	  spNode->PutdataType(L"i4"); break;
				case VT_BOOL: spNode->PutdataType(L"boolean"); break;
				case VT_DATE: spNode->PutdataType(L"dateTime"); break;
				default: break;
			}
		}

		// Set the value of the text
		if (vType == VT_BOOL)
			spNodeText->nodeTypedValue = _variant_t( (static_cast<bool>(vt) == true) ? (short)1 : (short)0, VT_I2);
		else
			spNodeText->nodeTypedValue = vt;
	}

	std::wstring GetXMLText() const
	{
		return static_cast<LPCWSTR>(xmlDOM_->xml);
	}

	static std::wstring CvtDate(DATE dt)
	{
		SYSTEMTIME sTime;
		if (VariantTimeToSystemTime(dt, &sTime))
		{
			wchar_t chBuff[30]; wsprintfW(chBuff, L"%04d-%02d-%02dT%02d:%02d:%02d",
					sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
			return chBuff;
		}
		return L"";
	}

// Internal methods
private:
	void loadMSXML()
	{
		// Try various flavors of the parser to see which one is installed.
		if (FAILED(xmlDOM_.CreateInstance(L"MSXML2.DOMDocument.4.0")) &&
			FAILED(xmlDOM_.CreateInstance(__uuidof(MSXML2::FreeThreadedDOMDocument30))) &&
			FAILED(xmlDOM_.CreateInstance(__uuidof(MSXML2::FreeThreadedDOMDocument26))) &&
			FAILED(xmlDOM_.CreateInstance(__uuidof(MSXML2::FreeThreadedDOMDocument))) &&
			FAILED(xmlDOM_.CreateInstance(__uuidof(MSXML2::DOMDocument))))
			throw std::runtime_error(std::string("MSXML is not installed."));
	}
};

}// namespace JTI_Util

#pragma warning(default:4100)

#endif // __JTI_MSXML_HELPER_INC__
