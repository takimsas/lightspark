/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SWFTYPES_H
#define SWFTYPES_H

#include "compat.h"
#include <llvm/System/DataTypes.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>

#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>

namespace lightspark
{

#define ASFUNCTION(name) \
	static ASObject* name(ASObject* , ASObject* const* args, const unsigned int argslen)
#define ASFUNCTIONBODY(c,name) \
	ASObject* c::name(ASObject* obj, ASObject* const* args, const unsigned int argslen)

enum SWFOBJECT_TYPE { T_OBJECT=0, T_INTEGER=1, T_NUMBER=2, T_FUNCTION=3, T_UNDEFINED=4, T_NULL=5, T_STRING=6, 
	T_DEFINABLE=7, T_BOOLEAN=8, T_ARRAY=9, T_CLASS=10, T_QNAME=11, T_NAMESPACE=12, T_UINTEGER=13, T_PROXY=14};

enum STACK_TYPE{STACK_NONE=0,STACK_OBJECT,STACK_INT,STACK_UINT,STACK_NUMBER,STACK_BOOLEAN};

typedef double number_t;

class ASObject;
class ABCContext;
class Class_base;
class IFunction;
struct arrayElem;

class tiny_string
{
friend std::ostream& operator<<(std::ostream& s, const tiny_string& r);
private:
	#define TS_SIZE 256
	char _buf_static[TS_SIZE];
	char* buf;
	bool isStatic;
	//TODO: use static buffer again if reassigning to short string
public:
	tiny_string():buf(_buf_static),isStatic(true){buf[0]=0;}
	tiny_string(const char* s):buf(_buf_static),isStatic(true)
	{
		if(strlen(s)>(TS_SIZE-1))
		{
			isStatic=false;
			buf=new char[4096];
		}
		assert(strlen(s)<=4096);
		strcpy(buf,s);
	}
	tiny_string(const tiny_string& r):buf(_buf_static),isStatic(true)
	{
		if(strlen(r.buf)>(TS_SIZE-1))
		{
			isStatic=false;
			buf=new char[4096];
		}
		assert(strlen(r.buf)<=4096);
		strcpy(buf,r.buf);
	}
	~tiny_string()
	{
		if(isStatic==false)
			delete[] buf;
	}
	explicit tiny_string(int i):buf(_buf_static),isStatic(true)
	{
		sprintf(buf,"%i",i);
	}
	explicit tiny_string(number_t d):buf(_buf_static),isStatic(true)
	{
		sprintf(buf,"%g",d);
	}
	tiny_string& operator=(const tiny_string& s)
	{
		if(s.len()>(TS_SIZE-1))
		{
			isStatic=false;
			assert(s.len()<=4096);
			buf=new char[4096];
		}
		//Lenght is already checked
		strcpy(buf,s.buf);
		return *this;
	}
	tiny_string& operator=(const std::string& s)
	{
		if(s.size()>(TS_SIZE-1))
		{
			isStatic=false;
			assert(s.size()<=4096);
			buf=new char[4096];
		}
		//Lenght is already checked
		strcpy(buf,s.c_str());
		return *this;
	}
	tiny_string& operator=(const char* s)
	{
		if(strlen(s)>(TS_SIZE-1))
		{
			isStatic=false;
			assert(strlen(s)<=4096);
			buf=new char[4096];
		}
		//Lenght is already checked
		strcpy(buf,s);
		return *this;
	}
	tiny_string& operator+=(const char* s)
	{
		assert((strlen(buf)+strlen(s)+1)<TS_SIZE);
		strcat(buf,s);
		return *this;
	}
	tiny_string& operator+=(const tiny_string& r)
	{
		assert((strlen(buf)+strlen(r.buf)+1)<TS_SIZE);
		strcat(buf,r.buf);
		return *this;
	}
	const tiny_string operator+(const tiny_string& r)
	{
		tiny_string ret(buf);
		ret+=r;
		return ret;
	}
	bool operator<(const tiny_string& r) const
	{
		return strcmp(buf,r.buf)<0;
	}
	bool operator==(const tiny_string& r) const
	{
		return strcmp(buf,r.buf)==0;
	}
	bool operator!=(const tiny_string& r) const
	{
		return strcmp(buf,r.buf)!=0;
	}
	bool operator==(const char* r) const
	{
		return strcmp(buf,r)==0;
	}
	bool operator!=(const char* r) const
	{
		return strcmp(buf,r)!=0;
	}
	const char* raw_buf() const
	{
		return buf;
	}
	char operator[](int i) const
	{
		return *(buf+i);
	}
	int len() const
	{
		return strlen(buf);
	}
	tiny_string substr(int start, int end) const
	{
		tiny_string ret;
		assert((end-start+1)<TS_SIZE);
		strncpy(ret.buf,buf+start,end-start);
		ret.buf[end-start]=0;
		return ret;
	}
};

class UI32
{
friend std::istream& operator>>(std::istream& s, UI32& v);
private:
	uint32_t val;
public:
	UI32():val(0){}
	UI32(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
	void bswap() { val=ntohl(val); }
};

class UI24
{
friend std::istream& operator>>(std::istream& s, UI24& v);
private:
	uint32_t val;
public:
	UI24():val(0){}
	operator uint32_t() const { return val; }
	void bswap()
	{
		val=ntohl(val)>>8; //The most significant byte is discarted
	}
};

class SI24
{
friend std::istream& operator>>(std::istream& s, SI24& v);
private:
	int32_t val;
public:
	SI24():val(0){}
	operator int32_t() const { return val; }
	void bswap() 
	{
		val=ntohl(val)>>8; //The most significant byte is discarted
		//Check for sign extension
		if(val&0x800000)
			val|=(0xff000000);
	}
};

class UI16
{
friend std::istream& operator>>(std::istream& s, UI16& v);
private:
	uint16_t val;
public:
	UI16():val(0){}
	UI16(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
	operator UI32() const { return val; }
	void bswap() { val=ntohs(val); }
};

class UI8 
{
friend std::istream& operator>>(std::istream& s, UI8& v);
private:
	uint8_t val;
public:
	UI8():val(0){}
	UI8(uint8_t v):val(v){}
	operator uint8_t() const { return val; }
	operator UI16(){ return val; }
};

class STRING
{
friend std::ostream& operator<<(std::ostream& s, const STRING& r);
friend std::istream& operator>>(std::istream& stream, STRING& v);
friend class ASString;
private:
	std::string String;
public:
	STRING():String(){};
	STRING(const char* s):String(s)
	{
	}
	bool operator==(const STRING& s)
	{
		if(String.size()!=s.String.size())
			return false;
		for(uint32_t i=0;i<String.size();i++)
		{
			if(String[i]!=s.String[i])
				return false;
		}
		return true;
	}
	STRING operator+(const STRING& s)
	{
		STRING ret(*this);
		for(unsigned int i=0;i<s.String.size();i++)
			ret.String.push_back(s.String[i]);
		return ret;
	}
	bool isNull() const
	{
		return !String.size();
	}
	operator const std::string&() const
	{
		return String;
	}
	operator const char*() const
	{
		return String.c_str();
	}
	int size()
	{
		return String.size();
	}
};

struct nsNameAndKind
{
	tiny_string name;
	int kind;
	nsNameAndKind(const tiny_string& _name, int _kind):name(_name),kind(_kind){}
	nsNameAndKind(const char* _name, int _kind):name(_name),kind(_kind){}
	bool operator<(const nsNameAndKind& r) const
	{
		return name < r.name;
	}
	bool operator<(const tiny_string& r) const
	{
		return name < r;
	}

	//Automatic conversion to tiny_string is useful is multiname lookup
	operator const tiny_string&() const
	{
		return name;
	}
};

struct multiname
{
	enum NAME_TYPE {NAME_STRING,NAME_INT,NAME_NUMBER,NAME_OBJECT};
	NAME_TYPE name_type;
	tiny_string name_s;
	union
	{
		int32_t name_i;
		number_t name_d;
		ASObject* name_o;
	};
	std::vector<nsNameAndKind> ns;
	tiny_string qualifiedString() const;
};

struct obj_var
{
	ASObject* var;
	IFunction* setter;
	IFunction* getter;
	obj_var():var(NULL),setter(NULL),getter(NULL){}
	explicit obj_var(ASObject* v):var(v),setter(NULL),getter(NULL){}
	explicit obj_var(ASObject* v,IFunction* g,IFunction* s):var(v),setter(s),getter(g){}
};

class Manager
{
friend class ASObject;
private:
	std::vector<ASObject*> available;
public:
template<class T>
	T* get();
	void put(ASObject* o);
};

class objAndLevel
{
public:
	ASObject* obj;
	int level;
	objAndLevel(ASObject* o, int l):obj(o),level(l){}
};

class nameAndLevel
{
public:
	tiny_string name;
	int level;
	nameAndLevel(const char* s, int l):name(s),level(l){}
	nameAndLevel(const tiny_string& n, int l):name(n),level(l){}
	bool operator<(const nameAndLevel& r) const
	{
		if(name==r.name)
			return level>r.level; //This forces the ordering in descending order
		else
			return name<r.name;
	}
};

class variables_map
{
//ASObject knows how to use its variable_map
friend class ASObject;
//ABCContext uses findObjVar when building and linking traits
friend class ABCContext;
private:
	std::multimap<nameAndLevel,std::pair<tiny_string, obj_var> > Variables;
	typedef std::multimap<nameAndLevel,std::pair<tiny_string, obj_var> >::iterator var_iterator;
	typedef std::multimap<nameAndLevel,std::pair<tiny_string, obj_var> >::const_iterator const_var_iterator;
	std::vector<var_iterator> slots_vars;
	//When findObjVar is invoked with create=true the pointer returned is garanteed to be valid
	//Level will be modified with the actual level where the object is found
	obj_var* findObjVar(const tiny_string& name, const tiny_string& ns, int& level, bool create, bool searchPreviusLevels);
	obj_var* findObjVar(const multiname& mname, int& level, bool create, bool searchPreviusLevels);
	void killObjVar(const multiname& mname, int level);
	ASObject* getSlot(unsigned int n)
	{
		return slots_vars[n-1]->second.second.var;
	}
	void setSlot(unsigned int n,ASObject* o);
	void initSlot(unsigned int n,int level, const tiny_string& name, const tiny_string& ns);
	ASObject* getVariableByString(const std::string& name);
	int size() const
	{
		return Variables.size();
	}
	tiny_string getNameAt(unsigned int i);
	obj_var* getValueAt(unsigned int i, int& level);
	~variables_map();
public:
	void dumpVariables();
};

//Atomic operations: placeholder until C++0x is supported in GCCl
inline void atomic_increment(int* operand)
{
    __asm__ volatile ("lock xaddl %1, (%0)" : : "r" (operand), "r"(1));
}
inline void atomic_decrement(int* operand)
{
    __asm__ volatile ("lock xaddl %1, (%0)" : : "r" (operand), "r"(-1));
}

class ASObject
{
friend class Manager;
friend class ABCVm;
friend class ABCContext;
friend class SystemState;
protected:
	//ASObject* asprototype; //HUMM.. ok the prototype, actually class, should be renamed
	//maps variable name to namespace and var
	variables_map Variables;
	ASObject(const ASObject& o);
	SWFOBJECT_TYPE type;
private:
	int ref_count;
	Manager* manager;
	int cur_level;
	virtual int _maxlevel();

public:
#ifndef NDEBUG
	//Stuff onyl used in debugging
	bool initialized;
	int getRefCount(){ return ref_count; }
#endif
	bool implEnable;
	Class_base* prototype;
	ASObject(Manager* m=NULL);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getPrototype);
	ASFUNCTION(_setPrototype);
	ASFUNCTION(_toString);
	virtual ~ASObject();
	void check() const;
	void incRef()
	{
		//std::cout << "incref " << this << std::endl;
		atomic_increment(&ref_count);
		assert(ref_count>0);
	}
	void decRef()
	{
		//std::cout << "decref " << this << std::endl;
		assert(ref_count>0);
		atomic_decrement(&ref_count);
		if(ref_count==0)
		{
			if(manager)
			{
				manager->put(this);
			}
			else
			{
				//Let's make refcount very invalid
				ref_count=-1024;
				//std::cout << "delete " << this << std::endl;
				delete this;
			}
		}
	}
	void fake_decRef()
	{
		atomic_decrement(&ref_count);
	}
	static void s_incRef(ASObject* o)
	{
		o->incRef();
	}
	static void s_decRef(ASObject* o)
	{
		if(o)
			o->decRef();
	}
	static void s_decRef_safe(ASObject* o,ASObject* o2)
	{
		if(o && o!=o2)
			o->decRef();
	}
	virtual ASObject* getVariableByString(const std::string& name)
	{
		ASObject* ret=Variables.getVariableByString(name);
		return ret;
	}
	//The enableOverride parameter is set to false in setSuper, getSuper and callSuper
	virtual objAndLevel getVariableByMultiname(const multiname& name, bool skip_impl=false, bool enableOverride=true );
	virtual intptr_t getVariableByMultiname_i(const multiname& name);
	virtual objAndLevel getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false);
	virtual void setVariableByMultiname_i(const multiname& name, intptr_t value);
	virtual void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride=true);
	virtual void deleteVariableByMultiname(const multiname& name);
	virtual void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool find_back=true, bool skip_impl=false);
	void setGetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o);
	void setSetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o);
	bool hasPropertyByMultiname(const multiname& name);
	bool hasPropertyByQName(const tiny_string& name, const tiny_string& ns);
	ASObject* getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	virtual void setSlot(unsigned int n,ASObject* o)
	{
		Variables.setSlot(n,o);
	}
	virtual void initSlot(unsigned int n,const tiny_string& name, const tiny_string& ns);
	virtual unsigned int numVariables();
	tiny_string getNameAt(int i)
	{
		return Variables.getNameAt(i);
	}
	ASObject* getValueAt(int i);
	SWFOBJECT_TYPE getObjectType() const
	{
		return type;
	}
	virtual tiny_string toString(bool debugMsg=false);
	virtual int32_t toInt();
	virtual uint32_t toUInt();
	virtual double toNumber();

	//Comparison operators
	virtual bool isEqual(ASObject* r);
	virtual bool isLess(ASObject* r);

	//Level handling
	int getLevel() const
	{
		return cur_level;
	}
	void decLevel()
	{
		assert(cur_level>0);
		cur_level--;
	}
	void setLevel(int l)
	{
		cur_level=l;
	}
	void resetLevel();

	//Prototype handling
	Class_base* getActualPrototype() const;
	
	static void sinit(Class_base*){}
	static void buildTraits(ASObject* o);
	
	//TODO: Rework this stuff
	virtual bool hasNext(unsigned int& index, bool& out);
	virtual bool nextName(unsigned int index, ASObject*& out);
	virtual bool nextValue(unsigned int index, ASObject*& out);
};

inline void Manager::put(ASObject* o)
{
	if(available.size()>15)
		delete o;
	else
		available.push_back(o);
}

template<class T>
T* Manager::get()
{
	if(available.size())
	{
		T* ret=static_cast<T*>(available.back());
		available.pop_back();
		ret->incRef();
		//std::cout << "getting[" << name << "] " << ret << std::endl;
		return ret;
	}
	else
	{
		T* ret=new T(this);
		//std::cout << "newing" << ret << std::endl;
		return ret;
	}
}

class FLOAT 
{
friend std::istream& operator>>(std::istream& s, FLOAT& v);
private:
	float val;
public:
	FLOAT():val(0){}
	FLOAT(float v):val(v){}
	operator float(){ return val; }
};

class DOUBLE 
{
friend std::istream& operator>>(std::istream& s, DOUBLE& v);
private:
	double val;
public:
	DOUBLE():val(0){}
	DOUBLE(double v):val(v){}
	operator double(){ return val; }
};

class SI16
{
friend std::istream& operator>>(std::istream& s, SI16& v);
private:
	int16_t val;
public:
	SI16():val(0){}
	SI16(int16_t v):val(v){}
	operator int16_t(){ return val; }
};

//TODO: Really implement or suppress
typedef UI32 SI32;

class RECORDHEADER
{
friend std::istream& operator>>(std::istream& s, RECORDHEADER& v);
private:
	UI16 CodeAndLen;
	SI32 Length;
public:
	unsigned int getLength() const
	{
		if((CodeAndLen&0x3f)==0x3f)
			return Length;
		else
			return CodeAndLen&0x3f;
	}
	unsigned int getTagType() const
	{
		return CodeAndLen>>6;
	}
};

class RGB
{
public:
	RGB(){};
	RGB(int r,int g, int b):Red(r),Green(g),Blue(b){};
	UI8 Red;
	UI8 Green;
	UI8 Blue;
};

class RGBA
{
public:
	RGBA():Red(0),Green(0),Blue(0),Alpha(255){}
	RGBA(int r, int g, int b, int a):Red(r),Green(g),Blue(b),Alpha(a){}
	UI8 Red;
	UI8 Green;
	UI8 Blue;
	UI8 Alpha;
	RGBA& operator=(const RGB& r)
	{
		Red=r.Red;
		Green=r.Green;
		Blue=r.Blue;
		Alpha=255;
		return *this;
	}
};

typedef UI8 LANGCODE;

std::istream& operator>>(std::istream& s, RGB& v);

inline std::istream& operator>>(std::istream& s, UI8& v)
{
	s.read((char*)&v.val,1);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI16& v)
{
	s.read((char*)&v.val,2);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16& v)
{
	s.read((char*)&v.val,2);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	//Check for sign extesion
	if(v.val&0x800000)
		v.val|=0xff000000;
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32& v)
{
	s.read((char*)&v.val,4);
	return s;
}

inline std::istream& operator>>(std::istream& s, FLOAT& v)
{
	s.read((char*)&v.val,4);
	return s;
}

inline std::istream& operator>>(std::istream& s, DOUBLE& v)
{
	// "Wacky format" is 45670123. Thanks to Ghash for reversing :-)
	s.read(((char*)&v.val)+4,4);
	s.read(((char*)&v.val),4);
	return s;
}

inline std::istream& operator>>(std::istream& s, RECORDHEADER& v)
{
	s >> v.CodeAndLen;
	if((v.CodeAndLen&0x3f)==0x3f)
		s >> v.Length;
	return s;
}

inline int min(int a,int b)
{
	return (a<b)?a:b;
}

inline int max(int a,int b)
{
	return (a>b)?a:b;
}

class BitStream
{
public:
	std::istream& f;
	unsigned char buffer;
	unsigned char pos;
public:
	BitStream(std::istream& in):f(in),pos(0){};
	unsigned int readBits(unsigned int num)
	{
		unsigned int ret=0;
		while(num)
		{
			if(!pos)
			{
				pos=8;
				f.read((char*)&buffer,1);
			}
			ret<<=1;
			ret|=(buffer>>(pos-1))&1;
			pos--;
			num--;
		}
		return ret;
	}
};

class FB
{
	int32_t buf;
	int size;
public:
	FB() { buf=0; }
	FB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(LOG_ERROR,"Fixed point bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator float() const
	{
		if(buf>=0)
		{
			int32_t b=buf;
			return b/65536.0f;
		}
		else
		{
			int32_t b=-buf;
			return -(b/65536.0f);
		}
		//return (buf>>16)+(buf&0xffff)/65536.0f;
	}
};

class UB
{
	uint32_t buf;
	int size;
public:
	UB() { buf=0; }
	UB(int s,BitStream& stream):size(s)
	{
/*		if(s%8)
			buf=new uint8_t[s/8+1];
		else
			buf=new uint8_t[s/8];
		int i=0;
		while(!s)
		{
			buf[i]=stream.readBits(min(s,8));
			s-=min(s,8);
			i++;
		}*/
		if(s>32)
			LOG(LOG_ERROR,"Unsigned bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
	}
	operator int() const
	{
		return buf;
	}
};

class SB
{
	int32_t buf;
	int size;
public:
	SB() { buf=0; }
	SB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(LOG_ERROR,"Signed bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator int() const
	{
		return buf;
	}
};

class RECT
{
	friend std::ostream& operator<<(std::ostream& s, const RECT& r);
	friend std::istream& operator>>(std::istream& stream, RECT& v);
public:
	int Xmin;
	int Xmax;
	int Ymin;
	int Ymax;
public:
	RECT();
	RECT(int xmin, int xmax, int ymin, int ymax);
};

class MATRIX
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
	friend std::ostream& operator<<(std::ostream& s, const MATRIX& r);
public:
	float ScaleX;
	float ScaleY;
	float RotateSkew0;
	float RotateSkew1;
	int TranslateX;
	int TranslateY;
public:
	MATRIX():ScaleX(1),ScaleY(1),RotateSkew0(0),RotateSkew1(0),TranslateX(0),TranslateY(0){}
	void get4DMatrix(float matrix[16]);
	void getTranslation(int& x, int& y);
	void multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
};

class GRADRECORD
{
	friend std::istream& operator>>(std::istream& s, GRADRECORD& v);
public:
	int version;
	UI8 Ratio;
	RGBA Color;
	bool operator<(const GRADRECORD& g) const
	{
		return Ratio<g.Ratio;
	}
};

class GRADIENT
{
	friend std::istream& operator>>(std::istream& s, GRADIENT& v);
public:
	int version;
	int SpreadMode;
	int InterpolationMode;
	int NumGradient;
	std::vector<GRADRECORD> GradientRecords;
};

class FOCALGRADIENT
{
	friend std::istream& operator>>(std::istream& s, FOCALGRADIENT& v);
public:
	int version;
	int SpreadMode;
	int InterpolationMode;
	int NumGradient;
	std::vector<GRADRECORD> GradientRecords;
	float FocalPoint;
};

class FILLSTYLEARRAY;
class MORPHFILLSTYLE;

class FILLSTYLE
{
	friend std::istream& operator>>(std::istream& s, FILLSTYLEARRAY& v);
	friend std::istream& operator>>(std::istream& s, FILLSTYLE& v);
	friend std::istream& operator>>(std::istream& s, MORPHFILLSTYLE& v);
	friend class DefineTextTag;
	friend class DefineShape2Tag;
	friend class DefineShape3Tag;
	friend class GeomShape;
	friend class Graphics;
private:
	int version;
	UI8 FillStyleType;
	RGBA Color;
	MATRIX GradientMatrix;
	GRADIENT Gradient;
	FOCALGRADIENT FocalGradient;
	UI16 BitmapId;
	MATRIX BitmapMatrix;

public:
	virtual void setFragmentProgram() const;
	static void fixedColor(float r, float g, float b);
	virtual void setVertexData(arrayElem* elem);
	virtual ~FILLSTYLE(){}
};

class MORPHFILLSTYLE:public FILLSTYLE
{
public:
	RGBA StartColor;
	RGBA EndColor;
	MATRIX StartGradientMatrix;
	MATRIX EndGradientMatrix;
	UI8 NumGradients;
	std::vector<UI8> StartRatios;
	std::vector<UI8> EndRatios;
	std::vector<RGBA> StartColors;
	std::vector<RGBA> EndColors;
	virtual ~MORPHFILLSTYLE(){}
};

class LINESTYLE
{
public:
	int version;
	UI16 Width;
	RGBA Color;
};

class LINESTYLE2
{
public:
	UI16 Width;
	UB StartCapStyle;
	UB JointStyle;
	UB HasFillFlag;
	UB NoHScaleFlag;
	UB NoVScaleFlag;
	UB PixelHintingFlag;
	UB NoClose;
	UB EndCapStyle;
	UI16 MiterLimitFactor;
	RGBA Color;
	FILLSTYLE FillType;
};

class MORPHLINESTYLE
{
public:
	UI16 StartWidth;
	UI16 EndWidth;
	RGBA StartColor;
	RGBA EndColor;
};

class LINESTYLEARRAY
{
public:
	LINESTYLEARRAY():version(-1){}
	void appendStyles(const LINESTYLEARRAY& r);
	int version;
	UI8 LineStyleCount;
	std::list<LINESTYLE> LineStyles;
	std::list<LINESTYLE2> LineStyles2;
};

class MORPHLINESTYLEARRAY
{
public:
	UI8 LineStyleCount;
	MORPHLINESTYLE* LineStyles;
};

class FILLSTYLEARRAY
{
public:
	FILLSTYLEARRAY():version(-1){}
	void appendStyles(const FILLSTYLEARRAY& r);
	int version;
	UI8 FillStyleCount;
	std::list<FILLSTYLE> FillStyles;
};

class MORPHFILLSTYLEARRAY
{
public:
	UI8 FillStyleCount;
	MORPHFILLSTYLE* FillStyles;
};

class SHAPE;
class SHAPEWITHSTYLE;

class SHAPERECORD
{
public:
	SHAPE* parent;
	UB TypeFlag;
	UB StateNewStyles;
	UB StateLineStyle;
	UB StateFillStyle1;
	UB StateFillStyle0;
	UB StateMoveTo;

	UB MoveBits;
	SB MoveDeltaX;
	SB MoveDeltaY;

	unsigned int FillStyle1;
	unsigned int FillStyle0;
	unsigned int LineStyle;

	//Edge record
	UB StraightFlag;
	UB NumBits;
	UB GeneralLineFlag;
	UB VertLineFlag;
	SB DeltaX;
	SB DeltaY;

	SB ControlDeltaX;
	SB ControlDeltaY;
	SB AnchorDeltaX;
	SB AnchorDeltaY;

	SHAPERECORD* next;

	SHAPERECORD():next(0){};
	SHAPERECORD(SHAPE* p,BitStream& bs);
};

class TEXTRECORD;

class GLYPHENTRY
{
public:
	UB GlyphIndex;
	SB GlyphAdvance;
	TEXTRECORD* parent;
	GLYPHENTRY(TEXTRECORD* p,BitStream& bs);
};

class DefineTextTag;

class TEXTRECORD
{
public:
	UB TextRecordType;
	UB StyleFlagsReserved;
	UB StyleFlagsHasFont;
	UB StyleFlagsHasColor;
	UB StyleFlagsHasYOffset;
	UB StyleFlagsHasXOffset;
	UI16 FontID;
	RGBA TextColor;
	SI16 XOffset;
	SI16 YOffset;
	UI16 TextHeight;
	UI8 GlyphCount;
	std::vector <GLYPHENTRY> GlyphEntries;
	DefineTextTag* parent;
	TEXTRECORD(DefineTextTag* p):parent(p){}
};

class GeomShape;

class SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPE& v);
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPE():fillOffset(0),lineOffset(0){}
	virtual ~SHAPE(){}
	UB NumFillBits;
	UB NumLineBits;
	unsigned int fillOffset;
	unsigned int lineOffset;
	SHAPERECORD ShapeRecords;
};

class SHAPEWITHSTYLE : public SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	int version;
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
};

class CXFORMWITHALPHA
{
	friend std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
private:
	UB HasAddTerms;
	UB HasMultTerms;
	UB NBits;
	SB RedMultTerm;
	SB GreenMultTerm;
	SB BlueMultTerm;
	SB AlphaMultTerm;
	SB RedAddTerm;
	SB GreenAddTerm;
	SB BlueAddTerm;
	SB AlphaAddTerm;
};

class CXFORM
{
};

class DROPSHADOWFILTER
{
};

class GLOWFILTER
{
};

class FILTER
{
public:
	UI8 FilterID;
	GLOWFILTER GlowFilter;
	DROPSHADOWFILTER DropShadowFilter;
};

class FILTERLIST
{
public:
	UI8 NumberOfFilters;
	std::vector<FILTER> Filters;
};

class BUTTONRECORD
{
public:
	BUTTONRECORD(int v):buttonVersion(v){}
	int buttonVersion;
	UB ButtonReserved;
	UB ButtonHasBlendMode;
	UB ButtonHasFilterList;
	UB ButtonStateHitTest;
	UB ButtonStateDown;
	UB ButtonStateOver;
	UB ButtonStateUp;
	UI16 CharacterID;
	UI16 PlaceDepth;
	MATRIX PlaceMatrix;
	CXFORMWITHALPHA	ColorTransform;
	FILTERLIST FilterList;
	UI8 BlendMode;

	bool isNull() const
	{
		return !(ButtonReserved | ButtonHasBlendMode | ButtonHasFilterList | ButtonStateHitTest | ButtonStateDown | ButtonStateOver | ButtonStateUp);
	}
};

class CLIPEVENTFLAGS
{
public:
	UI32 toParse;
	bool isNull();
};

class CLIPACTIONRECORD
{
public:
	CLIPEVENTFLAGS EventFlags;
	UI32 ActionRecordSize;
	bool isLast();
};

class CLIPACTIONS
{
public:
	UI16 Reserved;
	CLIPEVENTFLAGS AllEventFlags;
	std::vector<CLIPACTIONRECORD> ClipActionRecords;
};

class RunState
{
public:
	unsigned int FP;
	unsigned int next_FP;
	unsigned int max_FP;
	bool stop_FP;
	bool explicit_FP;
	RunState();
	void prepareNextFP();
};

ASObject* abstract_i(intptr_t i);
ASObject* abstract_b(bool i);
ASObject* abstract_d(number_t i);

void stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns);

std::ostream& operator<<(std::ostream& s, const RECT& r);
std::ostream& operator<<(std::ostream& s, const RGB& r);
std::ostream& operator<<(std::ostream& s, const RGBA& r);
std::ostream& operator<<(std::ostream& s, const STRING& r);
std::ostream& operator<<(std::ostream& s, const multiname& r);
std::ostream& operator<<(std::ostream& s, const tiny_string& r);

std::istream& operator>>(std::istream& s, RECT& v);
std::istream& operator>>(std::istream& s, CLIPEVENTFLAGS& v);
std::istream& operator>>(std::istream& s, CLIPACTIONRECORD& v);
std::istream& operator>>(std::istream& s, CLIPACTIONS& v);
std::istream& operator>>(std::istream& s, RGB& v);
std::istream& operator>>(std::istream& s, RGBA& v);
std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
std::istream& operator>>(std::istream& stream, LINESTYLE2& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLE& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPERECORD& v);
std::istream& operator>>(std::istream& stream, TEXTRECORD& v);
std::istream& operator>>(std::istream& stream, MATRIX& v);
std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
std::istream& operator>>(std::istream& stream, GLYPHENTRY& v);
std::istream& operator>>(std::istream& stream, STRING& v);
std::istream& operator>>(std::istream& stream, BUTTONRECORD& v);
std::istream& operator>>(std::istream& stream, RECORDHEADER& v);
std::istream& operator>>(std::istream& stream, FILTERLIST& v);
std::istream& operator>>(std::istream& stream, FILTER& v);
std::istream& operator>>(std::istream& stream, DROPSHADOWFILTER& v);
std::istream& operator>>(std::istream& stream, GLOWFILTER& v);


};
#endif