/*
 *  FakeSMCKey.h
 *  FakeSMC
 *
 *  Created by kozlek on 03/10/10.
 *  Copyright 2010 Natan Zalkin <natan.zalkin@me.com>. All rights reserved.
 *
 */

#ifndef _FAKESMCKEY_H
#define _FAKESMCKEY_H

#include <IOKit/IOService.h>

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

inline void copySymbol(const char *from, char* to)
{
    // Made the key name valid (4 char long): add trailing spaces if needed
    snprintf(to, 5, "%-4s", from);
}

class EXPORT FakeSMCKey : public OSObject
{
    OSDeclareDefaultStructors(FakeSMCKey)
    
private:
    UInt64              lastUpdated;
	
protected:
    char *              key;
    char *              type;
	UInt8               size;
	void *              value;
	IOService *         handler;
	
public:
	static FakeSMCKey   *withValue(const char *aKey, const char *aType, const unsigned char aSize, const void *aValue);
	static FakeSMCKey   *withHandler(const char *aKey, const char *aType, const unsigned char aSize, IOService *aHandler);
    
    // Not for general use. Use withHandler or withValue instance creation method
	virtual bool        init(const char * aKey, const char * aType, const unsigned char aSize, const void *aValue, IOService *aHandler = 0);
	
	virtual void        free();
	
    const char          *getName(); // this is used by logging functions
    
	const char          *getKey();
	const char          *getType();
	UInt8               getSize() const;
	void                *getValue();
    IOService           *getHandler();
	
    bool                setType(const char *aType);
    bool                setSize(UInt8 aSize);
	bool                setValueFromBuffer(const void *aBuffer, UInt8 aSize);
	bool                setHandler(IOService *aHandler);
	
	bool                isEqualTo(const char *aKey);
	bool                isEqualTo(FakeSMCKey *aKey);
	bool                isEqualTo(const OSMetaClassBase *anObject);
};


#endif