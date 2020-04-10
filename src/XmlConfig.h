/****************************************************************************/
//
// XmlConfig.h
//
// This file describes a XML-based configuration class.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//
// Revision History
// --------------------------------------------------------------
// 09/12/2002  MCS   Updated to use new XmlParser class to
//                   remove the dependancy on MSXML.
// 
/****************************************************************************/

#ifndef __JTI_XML_CONFIG_INC__
#define __JTI_XML_CONFIG_INC__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <wtypes.h>
#include <string>
#include <sstream>
#include <JTIUtils.h>
#include <xmlParser.h>

namespace JTI_Util
{
/****************************************************************************/
// XmlConfig
//
// Class used to read and write XML configuration files (ala .INI).
//
/*****************************************************************************/
class XmlConfig
{
// Constructor
public:
	XmlConfig(const char* pszSection="") : 
		isDirty_(false), hasData_(false), xmlSection_(pszSection) {/* */}
	XmlConfig(const XmlConfig& rhs) : xmlFile_(rhs.xmlFile_), isDirty_(rhs.isDirty_), hasData_(rhs.hasData_), xmlSection_(rhs.xmlSection_), xmlDoc_(rhs.xmlDoc_) {/* */}
	XmlConfig& operator=(const XmlConfig& rhs) {
		if (this != &rhs) {
			xmlFile_ = rhs.xmlFile_;
			xmlSection_ = rhs.xmlSection_;
			isDirty_ = rhs.isDirty_;
			hasData_ = rhs.hasData_;
			xmlDoc_ = rhs.xmlDoc_;
		}
		return *this;
	}

// Properties
public:
	__declspec(property(get=get_Root,put=set_Root)) std::string RootName;
	__declspec(property(get=get_AppName,put=set_AppName)) std::string AppName;
	__declspec(property(get=get_hasData)) bool HasData;
	__declspec(property(get=get_Text,put=set_Text)) std::string XmlText;
	__declspec(property(get=get_Filename)) const char* FileName;

// Access methods
public:
	const std::string& get_Root() const { return xmlDoc_.RootNode.Name; }
	void set_Root(const char* pszRoot) { xmlDoc_.RootNode.Name = pszRoot; }

	const std::string& get_AppName() const { return xmlSection_; }
	void set_AppName(const char* pszRoot) { xmlSection_ = pszRoot; }

	bool get_hasData() const { return hasData_; }
	std::string get_Text() const { return (hasData_) ? xmlDoc_.XmlText : ""; }
	bool set_Text(const char* pszXML) 
	{ 
		try
		{
			xmlDoc_.parse(pszXML);
		}
		catch(const std::runtime_error&)
		{
			return false;
		}
		return true;
	}

	bool KeyExists(const char* pszSection, const char* pszKey) const
	{
		try
		{
			return (xmlDoc_.find(BuildNodeName(pszSection, pszKey).c_str()).IsValid);
		}
		catch(const std::runtime_error&)
		{
			return false;
		}
	}

	std::string get_String(const char* pszSection, const char* pszKey, const char* pszDefault) const
	{
		bool fFound;
		std::string sValue = GetValue(pszSection, pszKey, fFound);
		if (!fFound)
			return pszDefault;
		return sValue;
	}

	std::wstring get_String(const char* pszSection, const char* pszKey, const wchar_t* pszDefault) const
	{
		bool fFound;
		std::string sValue = GetValue(pszSection, pszKey, fFound);
		if (!fFound)
			return pszDefault;
		USES_CONVERSION;
		return A2W(sValue.c_str());
	}

	unsigned long get_DWord(const char* pszSection, const char* pszKey, unsigned long nDefault=0L) const
	{
		bool fFound;
		std::string sValue = GetValue(pszSection, pszKey, fFound);
		if (!fFound || sValue.empty())
			return nDefault;
		return static_cast<unsigned long>(atol(sValue.c_str()));
	}

	long get_Int(const char* pszSection, const char* pszKey, long nDefault=0L) const
	{
		bool fFound;
		std::string sValue = GetValue(pszSection, pszKey, fFound);
		if (!fFound || sValue.empty())
			return nDefault;
		return atoi(sValue.c_str());
	}

	bool get_Bool(const char* pszSection, const char* pszKey, bool fDefault=false) const
	{
		bool fFound;
		std::string sValue = GetValue(pszSection, pszKey, fFound);
		if (!fFound || sValue.empty())
			return fDefault;
		return (!stricmp(sValue.c_str(), "true") || atoi(sValue.c_str()) == 1);
	}

	DATE get_Date(const char* pszSection, const char* pszKey, DATE dtDefault=0.0) const
	{
		bool fFound;
		std::string sValue = GetValue(pszSection, pszKey, fFound);
		if (fFound && !sValue.empty())
		{
			SYSTEMTIME sTime;
			if (sscanf(sValue.c_str(), "%04d-%02d-%02dT%02d:%02d:%02d", &sTime.wYear, &sTime.wMonth, &sTime.wDay, &sTime.wHour, &sTime.wMinute, &sTime.wSecond) == 6)
			{
				double dtNew;
				if (SystemTimeToVariantTime(&sTime, &dtNew))
					return dtNew;
			}
		}
		return dtDefault;
	}

	bool put_String(const char* pszSection, const char* pszKey, const char* pszValue)
	{
		return SetValue(pszSection, pszKey, pszValue);
	}

	bool put_String(const char* pszSection, const char* pszKey, const wchar_t* pszValue)
	{
		USES_CONVERSION;
		return SetValue(pszSection, pszKey, W2A(pszValue));
	}

	bool put_DWord(const char* pszSection, const char* pszKey, unsigned long dwValue)
	{
		char chBuff[50]; ultoa(dwValue, chBuff, 10);
		return SetValue(pszSection, pszKey, chBuff);
	}

	bool put_Int(const char* pszSection, const char* pszKey, long nValue)
	{
		char chBuff[50]; itoa(nValue, chBuff, 10);
		return SetValue(pszSection, pszKey, chBuff);
	}

	bool put_Bool(const char* pszSection, const char* pszKey, bool fValue)
	{
		char chBuff[10];
		strcpy(chBuff, (fValue) ? "true" : "false");
		return SetValue(pszSection, pszKey, chBuff);
	}

	bool put_Date(const char* pszSection, const char* pszKey, DATE dtValue)
	{
		SYSTEMTIME sTime;
		if (VariantTimeToSystemTime(dtValue, &sTime))
		{
			char chBuff[30]; wsprintfA(chBuff, "%04d-%02d-%02dT%02d:%02d:%02d",
					sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
			return SetValue(pszSection, pszKey, chBuff);
		}
		return false;
	}

	const char* get_Filename() const { return xmlFile_.c_str(); } 

	bool save(const char* pszXmlFile = NULL)  
	{ 
		if (pszXmlFile)
			xmlFile_ = pszXmlFile;
		return (isDirty_) ? xmlDoc_.save(xmlFile_.c_str()) : false; 
	}

	bool load(const char* pszXmlFile) 
	{ 
		xmlFile_ = pszXmlFile; 
		hasData_ = (xmlDoc_.load(xmlFile_.c_str()) && xmlDoc_.RootNode.IsValid);
        return hasData_;
	}

// Internal methods
private:
	std::string BuildNodeName(const char* pszSection, const char* pszKey) const
	{
		if (!xmlDoc_.RootNode.IsValid)
		{
			std::string s = "Configuration Root node not assigned (";
			s += + pszSection; s += "/"; s += pszKey; s += ")";
			throw std::runtime_error(s);
		}

		std::ostringstream ostm;
		ostm << "/" << xmlDoc_.RootNode.Name << "/";
		if (!xmlSection_.empty())
			ostm << xmlSection_ << "/";
		if (pszSection != NULL)
			ostm << pszSection << "/";
		ostm << pszKey;
		return ostm.str();
	}

	std::string GetValue(const char* pszSection, const char* pszKey, bool& fFound) const
	{
		XmlNode node = xmlDoc_.find(BuildNodeName(pszSection, pszKey).c_str());
		fFound = (node.IsValid);
		return node.Value;
	}

	bool SetValue(const char* pszSection, const char* pszKey, const char* pszValue)
	{
		XmlNode node = xmlDoc_.create(BuildNodeName(pszSection, pszKey).c_str());
		node.Value = pszValue;
		isDirty_ = true;
		return true;
	}

// Class data
private:
	mutable XmlDocument xmlDoc_;
	std::string xmlFile_;
	std::string xmlSection_;
	bool isDirty_, hasData_;
};

}// namespace JTI_Util

#endif // __JTI_XML_CONFIG_INC__
