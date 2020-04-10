/******************************************************************************/
//                                                                        
// memstream.h
//                                             
// This header implements a binary-based stream class comparable to IOStreams
// which stores an in-memory copy of the stream.
//
// Copyright (C) 1994-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//                                                                        
/******************************************************************************/

#ifndef _MEMSTREAM_INC_
#define _MEMSTREAM_INC_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <binstream.h>

namespace JTI_Util
{
/******************************************************************************/
// memstream
//
// Implementation class for the binary stream.
//
/******************************************************************************/
class memstream : public binstream
{
// Class data
private:
	static const unsigned int SIZE_INC = 4096;
	BYTE *dataBuff_, *currRead_, *currWrite_;
	unsigned int buffSize_;

// Constructor/Destructor
public:
	memstream(const void* pData, unsigned int nSize) : buffSize_(nSize), dataBuff_(0), currRead_(0), currWrite_(0) {
		currRead_ = currWrite_ = dataBuff_ = reinterpret_cast<BYTE*>(realloc(dataBuff_,nSize));
		memcpy(dataBuff_, pData, nSize);
	}
	memstream() : buffSize_(SIZE_INC), currRead_(0), currWrite_(0), dataBuff_(0) { 
		currRead_ = currWrite_ = dataBuff_ = reinterpret_cast<BYTE*>(realloc(dataBuff_,buffSize_));
		setbit(binstream::eofbit);
	}
	virtual ~memstream() { realloc(dataBuff_,0); }

// Overridable operations required in the derived classes
public:
	virtual bool skip(int sz) { 
		currRead_ += sz;
		clrbit(binstream::eofbit);
		if (currRead_ < dataBuff_)
			currRead_ = dataBuff_;
		else if (currRead_ >= (dataBuff_ + size()))
		{
			currRead_ = (dataBuff_ + size());
			setbit(binstream::eofbit);
		}
		return true;
	}

	virtual bool peek(void* pBuff, unsigned int sz) const {
		if (!currRead_) 
			return false;
		if ( (currRead_ + sz) > (dataBuff_ + size()) )
			return false;
		memcpy(pBuff,currRead_,sz);
		return true;
	}

	virtual bool read(void* pBuff, unsigned int sz) {
		if (peek(pBuff,sz)) {
			currRead_ += sz;
			if (currRead_ >= (dataBuff_+size()))
				setbit(binstream::eofbit);
			return true;
		}
		return false;
	}

	virtual bool write(const void* pBuff, unsigned int size) {
		if ( (currWrite_ + size) > (dataBuff_ + buffSize_) )
		{
			unsigned int nDiffR = static_cast<unsigned int>(currRead_ - dataBuff_);
			unsigned int nDiffW = static_cast<unsigned int>(currWrite_ - dataBuff_);
			dataBuff_ = reinterpret_cast<BYTE*>(realloc(dataBuff_, buffSize_+SIZE_INC));
			buffSize_ += SIZE_INC;
			currWrite_ = dataBuff_ + nDiffW;
			currRead_ = dataBuff_ + nDiffR;
		}
		memcpy(currWrite_, pBuff, size);
		currWrite_ += size;
		clrbit(binstream::eofbit);
		return true;
	}

// Access methods
public:
	void* get() const { return dataBuff_; }
	unsigned long size() const { 
		if (currWrite_ > dataBuff_)
			return static_cast<unsigned long>(currWrite_ - dataBuff_);
		return buffSize_;
	}
};

} // namespace JTI_Util

#endif // _MEMSTREAM_INC_
