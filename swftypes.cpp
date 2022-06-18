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

#define GL_GLEXT_PROTOTYPES

#include "swftypes.h"
#include "abc.h"
#include "tags.h"
#include "logger.h"
#include "actions.h"
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "swf.h"
#include "geometry.h"
#include "class.h"
#include "exceptions.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;
extern TLSDATA Manager* iManager;
extern TLSDATA Manager* dManager;

tiny_string ASObject::toString(bool debugMsg)
{
	assert(ref_count>0);
	if(debugMsg==false && hasPropertyByQName("toString",""))
	{
		objAndLevel obj_toString=getVariableByQName("toString","");
		if(obj_toString.obj->getObjectType()==T_FUNCTION)
		{
			IFunction* f_toString=static_cast<IFunction*>(obj_toString.obj);
			ASObject* ret=f_toString->call(this,NULL,0,obj_toString.level);
			assert(ret->getObjectType()==T_STRING);
			return ret->toString();
		}
	}
	return "[object Object]";
}

bool ASObject::isLess(ASObject* r)
{
	assert(ref_count>0);

	if(hasPropertyByQName("valueOf",""))
	{
		if(r->hasPropertyByQName("valueOf","")==false)
			throw RunTimeException("Missing valueof for second operand",sys->getOrigin().raw_buf());

		objAndLevel obj1=getVariableByQName("valueOf","");
		objAndLevel obj2=r->getVariableByQName("valueOf","");

		assert(obj1.obj!=NULL && obj2.obj!=NULL);

		assert(obj1.obj->getObjectType()==T_FUNCTION && obj2.obj->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1.obj);
		IFunction* f2=static_cast<IFunction*>(obj2.obj);

		ASObject* ret1=f1->call(this,NULL,0,obj1.level);
		ASObject* ret2=f2->call(r,NULL,0,obj2.level);

		LOG(LOG_CALLS,"Overloaded isLess");
		return ret1->isLess(ret2);
	}

	LOG(LOG_NOT_IMPLEMENTED,"Less than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	if(prototype)
		LOG(LOG_NOT_IMPLEMENTED,"Type " << prototype->class_name);
	throw RunTimeException("Not handled less comparison for objects",sys->getOrigin().raw_buf());
	return false;
}

bool ASObject::hasNext(unsigned int& index, bool& out)
{
	assert(implEnable);
	return false;
}

bool ASObject::nextName(unsigned int index, ASObject*& out)
{
	assert(implEnable);
	return false;
}

bool ASObject::nextValue(unsigned int index, ASObject*& out)
{
	assert(implEnable);
	return false;
}

void ASObject::buildTraits(ASObject* o)
{
	if(o->getActualPrototype()->class_name!="IInterface")
		LOG(LOG_NOT_IMPLEMENTED,"Add buildTraits for class " << o->getActualPrototype()->class_name);
}

tiny_string multiname::qualifiedString() const
{
	assert(ns.size()==1);
	assert(name_type==NAME_STRING);
	//TODO: what if the ns is empty
	if(false && ns[0].name=="")
		return name_s;
	else
	{
		tiny_string ret=ns[0].name;
		ret+="::";
		ret+=name_s;
		return ret;
	}
}

bool ASObject::isEqual(ASObject* r)
{
	assert(ref_count>0);

	//if we are comparing the same object the answer is true
	if(this==r)
		return true;

	if(r->getObjectType()==T_NULL || r->getObjectType()==T_UNDEFINED)
		return false;

	if(hasPropertyByQName("equals",""))
	{
		objAndLevel func_equals=getVariableByQName("equals","");

		assert(func_equals.obj!=NULL);

		assert(func_equals.obj->getObjectType()==T_FUNCTION);
		IFunction* func=static_cast<IFunction*>(func_equals.obj);

		ASObject* ret=func->call(this,&r,1,func_equals.level);
		assert(ret->getObjectType()==T_BOOLEAN);

		LOG(LOG_CALLS,"Overloaded isEqual");
		return Boolean_concrete(ret);
	}

	//We can try to call valueOf (maybe equals) and compare that
	if(hasPropertyByQName("valueOf",""))
	{
		if(r->hasPropertyByQName("valueOf","")==false)
			throw RunTimeException("Not handled less comparison for objects",sys->getOrigin().raw_buf());

		objAndLevel obj1=getVariableByQName("valueOf","");
		objAndLevel obj2=r->getVariableByQName("valueOf","");

		assert(obj1.obj!=NULL && obj2.obj!=NULL);

		assert(obj1.obj->getObjectType()==T_FUNCTION && obj2.obj->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1.obj);
		IFunction* f2=static_cast<IFunction*>(obj2.obj);

		ASObject* ret1=f1->call(this,NULL,0,obj1.level);
		ASObject* ret2=f2->call(r,NULL,0,obj2.level);

		LOG(LOG_CALLS,"Overloaded isEqual");
		return ret1->isEqual(ret2);
	}

	LOG(LOG_CALLS,"Equal comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	if(prototype)
		LOG(LOG_CALLS,"Type " << prototype->class_name);
	return false;
}

unsigned int ASObject::toUInt()
{
	return toInt();
}

int ASObject::toInt()
{
	LOG(LOG_ERROR,"Cannot convert object of type " << getObjectType() << " to int");
	throw RunTimeException("Cannot converto object to int",sys->getOrigin().raw_buf());
	return 0;
}

double ASObject::toNumber()
{
	LOG(LOG_ERROR,"Cannot convert object of type " << getObjectType() << " to float");
	throw RunTimeException("Cannot converto object to float",sys->getOrigin().raw_buf());
	return 0;
}

obj_var* variables_map::findObjVar(const tiny_string& n, const tiny_string& ns, int& level, bool create, bool searchPreviusLevels)
{
	nameAndLevel name(n,level);
	const var_iterator ret_begin=Variables.lower_bound(name);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	if(searchPreviusLevels)
		name.level=0;
	const var_iterator ret_end=Variables.upper_bound(name);
	name.level=level;

	var_iterator ret=ret_begin;
	for(;ret!=ret_end;ret++)
	{
		if(ret->second.first==ns)
		{
			level=ret->first.level;
			return &ret->second.second;
		}
	}

	//Name not present, insert it if we have to create it
	if(create)
	{
		var_iterator inserted=Variables.insert(ret_begin,make_pair(nameAndLevel(n,level), make_pair(ns, obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

bool ASObject::hasPropertyByQName(const tiny_string& name, const tiny_string& ns)
{
	assert(ref_count>0);

	//We look in all the object's levels
	int level=(prototype)?(prototype->max_level):0;
	return (Variables.findObjVar(name, ns, level, false, true)!=NULL);
}

bool ASObject::hasPropertyByMultiname(const multiname& name)
{
	assert(ref_count>0);

	//We look in all the object's levels
	int level=(prototype)?(prototype->max_level):0;
	return (Variables.findObjVar(name, level, false, true)!=NULL);
}

void ASObject::setGetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	assert(ref_count>0);
	assert(!initialized);
	//Getters are inserted with the current level of the prototype chain
	int level=cur_level;
	obj_var* obj=Variables.findObjVar(name,ns,level,true,false);
	if(obj->getter!=NULL)
	{
		//This happens when interfaces are declared multiple times
		assert(o==obj->getter);
		return;
	}
	obj->getter=o;
}

void ASObject::setSetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	assert(ref_count>0);
	assert(!initialized);
	//Setters are inserted with the current level of the prototype chain
	int level=cur_level;
	obj_var* obj=Variables.findObjVar(name,ns,level,true,false);
	if(obj->setter!=NULL)
	{
		//This happens when interfaces are declared multiple times
		assert(o==obj->setter);
		return;
	}
	obj->setter=o;
}

void ASObject::deleteVariableByMultiname(const multiname& name)
{
	assert(ref_count>0);

	//Find out if the variable is declared more than once
	obj_var* obj=NULL;
	int level;
	unsigned int count=0;
	//We search in every level
	int max_level=(prototype)?prototype->max_level:0;
	for(int i=max_level;i>=0;i--)
	{
		//We stick to the old iteration mode, as we need to count
		obj=Variables.findObjVar(name,max_level,false,false);
		if(obj)
		{
			count++;
			level=i;
		}
	}
	//if it's not present it's ok
	if(count==0)
		return;

	assert(count==1);

	//Now dereference the values
	//TODO: maybe we can look on the previous levels
	obj=Variables.findObjVar(name,level,false,false);
	if(obj->var)
		obj->var->decRef();
	if(obj->getter)
		obj->getter->decRef();
	if(obj->setter)
		obj->setter->decRef();

	//Now kill the variable
	Variables.killObjVar(name,level);
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	check();
	setVariableByMultiname(name,abstract_i(value));
}

void ASObject::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride)
{
	check();

	obj_var* obj=NULL;
	//It's always correct to use the current level for the object
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level
	int level=cur_level;
	obj=Variables.findObjVar(name,level,false,true);

	if(obj==NULL)
	{
		assert(level==cur_level);
		obj=Variables.findObjVar(name,level,true,false);
	}

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,"Calling the setter");
		//Overriding function is automatically done by using cur_level
		IFunction* setter=obj->setter;
		if(enableOverride)
			setter=setter->getOverride();

		//One argument can be passed without creating an array
		ASObject* ret=setter->call(this,&o,1,level);
		assert(ret==NULL);
		LOG(LOG_CALLS,"End of setter");
	}
	else
	{
		assert(!obj->getter);
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
	check();
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool find_back, bool skip_impl)
{
	obj_var* obj=NULL;
	//It's always correct to use the current level for the object
	//NOTE: we assume that [gs]etSuper and setProperty correctly manipulate the cur_level
	int level=cur_level;
	obj=Variables.findObjVar(name,ns,level,false,find_back);

	if(obj==NULL)
	{
		//When the var is not found level should not be modified
		assert(cur_level==level);
		obj=Variables.findObjVar(name,ns,level,true,false);
	}

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,"Calling the setter");

		IFunction* setter=obj->setter->getOverride();
		//One argument can be passed without creating an array
		ASObject* ret=setter->call(this,&o,1,level);
		assert(ret==NULL);
		LOG(LOG_CALLS,"End of setter");
	}
	else
	{
		assert(!obj->getter);
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
	check();
}

void variables_map::killObjVar(const multiname& mname, int level)
{
	nameAndLevel name("",level);
	switch(mname.name_type)
	{
		case multiname::NAME_INT:
			name.name=tiny_string(mname.name_i);
			break;
		case multiname::NAME_NUMBER:
			name.name=tiny_string(mname.name_d);
			break;
		case multiname::NAME_STRING:
			name.name=mname.name_s;
			break;
		default:
			assert("Unexpected name kind" && false);
	}


	const pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	assert(ret.first!=ret.second);

	//Find the namespace
	assert(!mname.ns.empty());
	for(unsigned int i=0;i<mname.ns.size();i++)
	{
		const tiny_string& ns=mname.ns[i].name;
		var_iterator start=ret.first;
		for(;start!=ret.second;start++)
		{
			if(start->second.first==ns)
			{
				Variables.erase(start);
				return;
			}
		}
	}

	throw RunTimeException("Variable to kill not found",sys->getOrigin().raw_buf());
}

obj_var* variables_map::findObjVar(const multiname& mname, int& level, bool create, bool searchPreviusLevels)
{
	nameAndLevel name("",level);
	switch(mname.name_type)
	{
		case multiname::NAME_INT:
			name.name=tiny_string(mname.name_i);
			break;
		case multiname::NAME_NUMBER:
			name.name=tiny_string(mname.name_d);
			break;
		case multiname::NAME_STRING:
			name.name=mname.name_s;
			break;
		default:
			assert("Unexpected name kind" && false);
	}

	const var_iterator ret_begin=Variables.lower_bound(name);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	if(searchPreviusLevels)
		name.level=0;
	const var_iterator ret_end=Variables.upper_bound(name);
	name.level=level;

	var_iterator ret=ret_begin;
	for(;ret!=ret_end;ret++)
	{
		//Check if one the namespace is already present
		assert(!mname.ns.empty());
		//We can use binary search, as the namespace are ordered
		if(binary_search(mname.ns.begin(),mname.ns.end(),ret->second.first))
		{
			level=ret->first.level;
			return &ret->second.second;
		}
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	//TODO: HACK: this is needed if the property should be present but it's not
	if(create)
	{
		if(mname.ns.size()>1)
		{
			//Hack, insert with empty name
			//Here the object MUST exist
			var_iterator inserted=Variables.insert(ret,make_pair(name, make_pair("", obj_var() ) ) );
			return &inserted->second.second;
		}
		var_iterator inserted=Variables.insert(ret,make_pair(name, make_pair(mname.ns[0].name, obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

ASFUNCTIONBODY(ASObject,_toString)
{
	return Class<ASString>::getInstanceS(obj->toString());
}

ASFUNCTIONBODY(ASObject,_constructor)
{
	return NULL;
}

/*ASFUNCTIONBODY(ASObject,_getPrototype)
{
	if(prototype==NULL)
		return new Undefined;

	prototype->incRef();
	return prototype;
}

ASFUNCTIONBODY(ASObject,_setPrototype)
{
	if(prototype)
		prototype->decRef();

	prototype=args->at(0);
	prototype->incRef();
	return NULL;
}*/


void ASObject::initSlot(unsigned int n,const tiny_string& name, const tiny_string& ns)
{
	//Should be correct to use the level on the prototype chain
	assert(!initialized);
	Variables.initSlot(n,cur_level,name,ns);
}

//In all the getter function we first ask the interface, so that special handling (e.g. Array)
//can be done
intptr_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	ASObject* ret=getVariableByMultiname(name).obj;
	assert(ret);
	return ret->toInt();
}

objAndLevel ASObject::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride)
{
	check();

	obj_var* obj=NULL;
	int level=-1;
	int max_level=cur_level;
	for(int i=max_level;i>=0;i--)
	{
		//The variable i is automatically moved to the right level
		obj=Variables.findObjVar(name,i,false,true);
		if(obj)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, just go to the previous level
			if(obj->getter || obj->var)
			{
				level=i;
				break;
			}
		}
	}

	if(obj!=NULL)
	{
		assert(level!=-1);
		if(obj->getter)
		{
			//Call the getter
			if(prototype)
			{
				LOG(LOG_CALLS,"Calling the getter on type " << prototype->class_name);
			}
			else
			{
				LOG(LOG_CALLS,"Calling the getter");
			}
			IFunction* getter=obj->getter;
			if(enableOverride)
				getter=getter->getOverride();
			ASObject* ret=getter->call(this,NULL,0,level);
			LOG(LOG_CALLS,"End of getter");
			assert(ret);
			//The returned value is already owned by the caller
			ret->fake_decRef();
			//TODO: check
			return objAndLevel(ret,level);
		}
		else
		{
			assert(!obj->setter);
			assert(obj->var);
			return objAndLevel(obj->var,level);
		}
	}
	else
	{
		//Check if we should do lazy definition
		if(name.name_s=="toString")
		{
			ASObject* ret=new Function(ASObject::_toString);
			setVariableByQName("toString","",ret);
			//Added at level 0, as Object is always the base
			return objAndLevel(ret,0);
		}
		else if(getObjectType()==T_FUNCTION && name.name_s=="call")
		{
			//Fake returning the function itself
			return objAndLevel(this,0);
		}
		else if(getObjectType()==T_FUNCTION && name.name_s=="apply")
		{
			//Create on the fly a Function
			//HACK: both call and apply should be included in the Function object
			return objAndLevel(new Function(IFunction::apply),0);
		}

		//It has not been found yet, ask the prototype
		if(prototype)
			return prototype->getVariableByMultiname(name,skip_impl);
	}

	//If it has not been found
	return objAndLevel(NULL,0);
}

objAndLevel ASObject::getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl)
{
	check();

	obj_var* obj=NULL;
	int level=cur_level;
	obj=Variables.findObjVar(name,ns,level,false,true);

	if(obj!=NULL)
	{
		if(obj->getter)
		{
			//Call the getter
			LOG(LOG_CALLS,"Calling the getter");
			IFunction* getter=obj->getter->getOverride();
			ASObject* ret=getter->call(this,NULL,0,level);
			LOG(LOG_CALLS,"End of getter");
			//The variable is already owned by the caller
			ret->fake_decRef();
			return objAndLevel(ret,level);
		}
		else
			return objAndLevel(obj->var,level);
	}
	else if(prototype)
	{
		objAndLevel ret=prototype->getVariableByQName(name,ns);
		if(ret.obj)
			return ret;
	}

	return objAndLevel(NULL,0);
}

ASObject* variables_map::getVariableByString(const std::string& name)
{
	//Slow linear lookup, should be avoided
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		string cur(it->second.first.raw_buf());
		if(!cur.empty())
			cur+='.';
		cur+=it->first.name.raw_buf();
		if(cur==name)
		{
			if(it->second.second.getter)
				throw UnsupportedException("Getters are not supported in getVariableByString",sys->getOrigin().raw_buf()); 
			return it->second.second.var;
		}
	}
	
	return NULL;
}

std::ostream& lightspark::operator<<(std::ostream& s, const tiny_string& r)
{
	s << r.buf;
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const multiname& r)
{
	for(unsigned int i=0;i<r.ns.size();i++)
	{
		string prefix;
		switch(r.ns[i].kind)
		{
			case 0x08:
				prefix="ns:";
				break;
			case 0x16:
				prefix="pakns:";
				break;
			case 0x17:
				prefix="pakintns:";
				break;
			case 0x18:
				prefix="protns:";
				break;
			case 0x19:
				prefix="explns:";
				break;
			case 0x1a:
				prefix="staticprotns:";
				break;
			case 0x05:
				prefix="privns:";
				break;
		}
		s << '[' << prefix << r.ns[i].name << "] ";
	}
	if(r.name_type==multiname::NAME_INT)
		s << r.name_i;
	else if(r.name_type==multiname::NAME_NUMBER)
		s << r.name_d;
	else if(r.name_type==multiname::NAME_STRING)
		s << r.name_s;
	else
		s << r.name_o; //We print the hexadecimal value
	return s;
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
	assert(ref_count>0);
	//Heavyweight stuff
#ifndef NDEBUG
	variables_map::const_var_iterator it=Variables.Variables.begin();
	for(;it!=Variables.Variables.end();it++)
	{
		variables_map::const_var_iterator next=it;
		next++;
		if(next==Variables.Variables.end())
			break;

		//No double definition of a single variable should exist
		if(it->first.name==next->first.name && it->second.first==next->second.first)
		{
			if(it->second.second.var==NULL && next->second.second.var==NULL)
				continue;

			if(it->second.second.var==NULL || next->second.second.var==NULL)
			{
				cout << it->first.name << endl;
				cout << it->second.second.var << ' ' << it->second.second.setter << ' ' << it->second.second.getter << endl;
				cout << next->second.second.var << ' ' << next->second.second.setter << ' ' << next->second.second.getter << endl;
				abort();
			}

			if(it->second.second.var->getObjectType()!=T_FUNCTION || next->second.second.var->getObjectType()!=T_FUNCTION)
			{
				cout << it->first.name << endl;
				abort();
			}
		}
	}

#endif
}

void variables_map::dumpVariables()
{
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();it++)
		cerr << it->first.level << ": [" << it->second.first << "] "<< it->first.name << " " << 
			it->second.second.var << ' ' << it->second.second.setter << ' ' << it->second.second.getter << endl;
}

lightspark::RECT::RECT()
{
}

lightspark::RECT::RECT(int a, int b, int c, int d):Xmin(a),Xmax(b),Ymin(c),Ymax(d)
{
}

std::ostream& lightspark::operator<<(std::ostream& s, const RECT& r)
{
	s << '{' << (int)r.Xmin << ',' << r.Xmax << ',' << r.Ymin << ',' << r.Ymax << '}';
	return s;
}

ostream& lightspark::operator<<(ostream& s, const STRING& t)
{
	for(unsigned int i=0;i<t.String.size();i++)
		s << t.String[i];
	return s;
}

std::ostream& operator<<(std::ostream& s, const RGBA& r)
{
	s << "RGBA <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << ',' << (int)r.Alpha << '>';
	return s;
}

std::ostream& operator<<(std::ostream& s, const RGB& r)
{
	s << "RGB <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << '>';
	return s;
}

void MATRIX::get4DMatrix(float matrix[16])
{
	memset(matrix,0,sizeof(float)*16);
	matrix[0]=ScaleX;
	matrix[1]=RotateSkew0;

	matrix[4]=RotateSkew1;
	matrix[5]=ScaleY;

	matrix[10]=1;

	matrix[12]=TranslateX;
	matrix[13]=TranslateY;
	matrix[15]=1;
}

void MATRIX::multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	xout=xin*ScaleX + yin*RotateSkew1 + TranslateX;
	yout=xin*RotateSkew0 + yin*ScaleY + TranslateY;
}

void MATRIX::getTranslation(int& x, int& y)
{
	x=TranslateX;
	y=TranslateY;
}

std::ostream& operator<<(std::ostream& s, const MATRIX& r)
{
	s << "| " << r.ScaleX << ' ' << r.RotateSkew0 << " |" << std::endl;
	s << "| " << r.RotateSkew1 << ' ' << r.ScaleY << " |" << std::endl;
	s << "| " << (int)r.TranslateX << ' ' << (int)r.TranslateY << " |" << std::endl;
	return s;
}

std::istream& lightspark::operator>>(std::istream& stream, STRING& v)
{
	v.String.clear();
	UI8 c;
	do
	{
		stream >> c;
		if(c==0)
			break;
		v.String.push_back(c);
	}
	while(c!=0);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, RECT& v)
{
	BitStream s(stream);
	int nbits=UB(5,s);
	v.Xmin=SB(nbits,s);
	v.Xmax=SB(nbits,s);
	v.Ymin=SB(nbits,s);
	v.Ymax=SB(nbits,s);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& s, RGB& v)
{
	s >> v.Red >> v.Green >> v.Blue;
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, RGBA& v)
{
	s >> v.Red >> v.Green >> v.Blue >> v.Alpha;
	return s;
}

void LINESTYLEARRAY::appendStyles(const LINESTYLEARRAY& r)
{
	unsigned int count = LineStyleCount + r.LineStyleCount;
	assert(version!=-1);

	assert(r.version==version);
	if(version<4)
		LineStyles.insert(LineStyles.end(),r.LineStyles.begin(),r.LineStyles.end());
	else
		LineStyles2.insert(LineStyles2.end(),r.LineStyles2.begin(),r.LineStyles2.end());
	LineStyleCount = count;
}

std::istream& lightspark::operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	assert(v.version!=-1);
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,"Line array extended not supported");
	if(v.version<4)
	{
		for(int i=0;i<v.LineStyleCount;i++)
		{
			LINESTYLE tmp;
			tmp.version=v.version;
			s >> tmp;
			v.LineStyles.push_back(tmp);
		}
	}
	else
	{
		for(int i=0;i<v.LineStyleCount;i++)
		{
			LINESTYLE2 tmp;
			s >> tmp;
			v.LineStyles2.push_back(tmp);
		}
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHLINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,"Line array extended not supported");
	v.LineStyles=new MORPHLINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

void FILLSTYLEARRAY::appendStyles(const FILLSTYLEARRAY& r)
{
	assert(version!=-1);
	unsigned int count = FillStyleCount + r.FillStyleCount;

	FillStyles.insert(FillStyles.end(),r.FillStyles.begin(),r.FillStyles.end());
	FillStyleCount = count;
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	assert(v.version!=-1);
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,"Fill array extended not supported");

	v.FillStyles.resize(v.FillStyleCount);
	list<FILLSTYLE>::iterator it=v.FillStyles.begin();
	for(;it!=v.FillStyles.end();it++)
	{
		it->version=v.version;
		s >> *it;
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,"Fill array extended not supported");
	v.FillStyles=new MORPHFILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, SHAPE& v)
{
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.TypeFlag + v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+
			v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, SHAPEWITHSTYLE& v)
{
	v.FillStyles.version=v.version;
	v.LineStyles.version=v.version;
	s >> v.FillStyles >> v.LineStyles;
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.TypeFlag+v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+
			v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}
	return s;
}

istream& lightspark::operator>>(istream& s, LINESTYLE2& v)
{
	s >> v.Width;
	BitStream bs(s);
	v.StartCapStyle=UB(2,bs);
	v.JointStyle=UB(2,bs);
	v.HasFillFlag=UB(1,bs);
	v.NoHScaleFlag=UB(1,bs);
	v.NoVScaleFlag=UB(1,bs);
	v.PixelHintingFlag=UB(1,bs);
	UB(5,bs);
	v.NoClose=UB(1,bs);
	v.EndCapStyle=UB(2,bs);
	if(v.JointStyle==2)
		s >> v.MiterLimitFactor;
	if(v.HasFillFlag)
		s >> v.FillType;
	else
		s >> v.Color;

	return s;
}

istream& lightspark::operator>>(istream& s, LINESTYLE& v)
{
	s >> v.Width;
	if(v.version==2 || v.version==1)
	{
		RGB tmp;
		s >> tmp;
		v.Color=tmp;
	}
	else
		s >> v.Color;
	return s;
}

istream& lightspark::operator>>(istream& s, MORPHLINESTYLE& v)
{
	s >> v.StartWidth >> v.EndWidth >> v.StartColor >> v.EndColor;
	return s;
}

std::istream& lightspark::operator>>(std::istream& in, TEXTRECORD& v)
{
	BitStream bs(in);
	v.TextRecordType=UB(1,bs);
	v.StyleFlagsReserved=UB(3,bs);
	if(v.StyleFlagsReserved)
		LOG(LOG_ERROR,"Reserved bits not so reserved");
	v.StyleFlagsHasFont=UB(1,bs);
	v.StyleFlagsHasColor=UB(1,bs);
	v.StyleFlagsHasYOffset=UB(1,bs);
	v.StyleFlagsHasXOffset=UB(1,bs);
	if(!v.TextRecordType)
		return in;
	if(v.StyleFlagsHasFont)
		in >> v.FontID;
	if(v.StyleFlagsHasColor)
	{
		RGB t;
		in >> t;
		v.TextColor=t;
	}
	if(v.StyleFlagsHasXOffset)
		in >> v.XOffset;
	if(v.StyleFlagsHasYOffset)
		in >> v.YOffset;
	if(v.StyleFlagsHasFont)
		in >> v.TextHeight;
	in >> v.GlyphCount;
	v.GlyphEntries.clear();
	for(int i=0;i<v.GlyphCount;i++)
	{
		v.GlyphEntries.push_back(GLYPHENTRY(&v,bs));
	}

	return in;
}

std::istream& lightspark::operator>>(std::istream& s, GRADRECORD& v)
{
	s >> v.Ratio;
	if(v.version==1 || v.version==2)
	{
		RGB tmp;
		s >> tmp;
		v.Color=tmp;
	}
	else
		s >> v.Color;

	return s;
}

std::istream& lightspark::operator>>(std::istream& s, GRADIENT& v)
{
	BitStream bs(s);
	v.SpreadMode=UB(2,bs);
	v.InterpolationMode=UB(2,bs);
	v.NumGradient=UB(4,bs);
	GRADRECORD gr;
	gr.version=v.version;
	for(int i=0;i<v.NumGradient;i++)
	{
		s >> gr;
		v.GradientRecords.push_back(gr);
	}
	sort(v.GradientRecords.begin(),v.GradientRecords.end());
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, FOCALGRADIENT& v)
{
	BitStream bs(s);
	v.SpreadMode=UB(2,bs);
	v.InterpolationMode=UB(2,bs);
	v.NumGradient=UB(4,bs);
	GRADRECORD gr;
	gr.version=v.version;
	for(int i=0;i<v.NumGradient;i++)
	{
		s >> gr;
		v.GradientRecords.push_back(gr);
	}
	sort(v.GradientRecords.begin(),v.GradientRecords.end());
	//TODO: support FocalPoint
	s.read((char*)&v.FocalPoint,2);
	return s;
}

inline RGBA medianColor(const RGBA& a, const RGBA& b, float factor)
{
	return RGBA(a.Red+(b.Red-a.Red)*factor,
		a.Green+(b.Green-a.Green)*factor,
		a.Blue+(b.Blue-a.Blue)*factor,
		a.Alpha+(b.Alpha-a.Alpha)*factor);
}

void FILLSTYLE::setVertexData(arrayElem* elem)
{
	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		elem->colors[0]=1;
		elem->colors[1]=0;
		elem->colors[2]=0;

		elem->texcoord[0]=float(Color.Red)/256.0f;
		elem->texcoord[1]=float(Color.Green)/256.0f;
		elem->texcoord[2]=float(Color.Blue)/256.0f;
		elem->texcoord[3]=float(Color.Alpha)/256.0f;
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");
		elem->colors[0]=0;
		elem->colors[1]=1;
		elem->colors[2]=0;

		/*color_entry buffer[256];
		int grad_index=0;
		RGBA color_l(0,0,0,1);
		int index_l=0;
		RGBA color_r(Gradient.GradientRecords[0].Color);
		int index_r=Gradient.GradientRecords[0].Ratio;

		for(int i=0;i<256;i++)
		{
			float dist=i-index_l;
			dist/=(index_r-index_l);
			RGBA c=medianColor(color_l,color_r,dist);
			buffer[i].r=float(c.Red)/256.0f;
			buffer[i].g=float(c.Green)/256.0f;
			buffer[i].b=float(c.Blue)/256.0f;
			buffer[i].a=1;

			if(Gradient.GradientRecords[grad_index].Ratio==i)
			{
				grad_index++;
				color_l=color_r;
				index_l=index_r;
				color_r=Gradient.GradientRecords[grad_index].Color;
				index_r=Gradient.GradientRecords[grad_index].Ratio;
			}
		}

		glBindTexture(GL_TEXTURE_2D,rt->data_tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);*/
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Reverting to fixed function");
		elem->colors[0]=1;
		elem->colors[1]=0;
		elem->colors[2]=0;

		elem->texcoord[0]=0.5;
		elem->texcoord[1]=0.5;
		elem->texcoord[2]=0;
		elem->texcoord[3]=1;
	}
}

void FILLSTYLE::setFragmentProgram() const
{
	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	struct color_entry
	{
		float r,g,b,a;
	};

	glBindTexture(GL_TEXTURE_2D,rt->data_tex);

	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		glColor4f(1,0,0,0);
		glTexCoord4f(float(Color.Red)/256.0f,
			float(Color.Green)/256.0f,
			float(Color.Blue)/256.0f,
			float(Color.Alpha)/256.0f);
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");

		color_entry buffer[256];
		unsigned int grad_index=0;
		RGBA color_l(0,0,0,1);
		int index_l=0;
		RGBA color_r(Gradient.GradientRecords[0].Color);
		int index_r=Gradient.GradientRecords[0].Ratio;
		/*glColor4f(0,1,0,0);

		for(int i=0;i<256;i++)
		{
			float dist=i-index_l;
			dist/=(index_r-index_l);
			RGBA c=medianColor(color_l,color_r,dist);
			buffer[i].r=float(c.Red)/256.0f;
			buffer[i].g=float(c.Green)/256.0f;
			buffer[i].b=float(c.Blue)/256.0f;
			buffer[i].a=1;

			assert(grad_index<Gradient.GradientRecords.size());
			if(Gradient.GradientRecords[grad_index].Ratio==i)
			{
				color_l=color_r;
				index_l=index_r;
				color_r=Gradient.GradientRecords[grad_index].Color;
				index_r=Gradient.GradientRecords[grad_index].Ratio;
				grad_index++;
			}
		}

		glBindTexture(GL_TEXTURE_2D,rt->data_tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);*/

		//HACK: TODO: revamp gradient support
		glColor4f(1,0,0,0);
		glTexCoord4f(float(color_r.Red)/256.0f,
			float(color_r.Green)/256.0f,
			float(color_r.Blue)/256.0f,
			float(color_r.Alpha)/256.0f);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Style not implmented");
		FILLSTYLE::fixedColor(0.5,0.5,0);
	}
}

void FILLSTYLE::fixedColor(float r, float g, float b)
{
	glBindTexture(GL_TEXTURE_2D,rt->data_tex);

	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	glColor4f(1,0,0,0);
	glTexCoord4f(r,g,b,1);
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType==0x00)
	{
		if(v.version==1 || v.version==2)
		{
			RGB tmp;
			s >> tmp;
			v.Color=tmp;
		}
		else
			s >> v.Color;
	}
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12 || v.FillStyleType==0x13)
	{
		s >> v.GradientMatrix;
		v.Gradient.version=v.version;
		v.FocalGradient.version=v.version;
		if(v.FillStyleType==0x13)
			s >> v.FocalGradient;
		else
			s >> v.Gradient;
	}
	else if(v.FillStyleType==0x41 || v.FillStyleType==0x42 || v.FillStyleType==0x43)
	{
		s >> v.BitmapId >> v.BitmapMatrix;
	}
	else
	{
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
	}
	return s;
}


std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType==0x00)
	{
		s >> v.StartColor >> v.EndColor;
	}
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12)
	{
		s >> v.StartGradientMatrix >> v.EndGradientMatrix;
		s >> v.NumGradients;
		UI8 t;
		RGBA t2;
		for(int i=0;i<v.NumGradients;i++)
		{
			s >> t >> t2;
			v.StartRatios.push_back(t);
			v.StartColors.push_back(t2);
			s >> t >> t2;
			v.EndRatios.push_back(t);
			v.EndColors.push_back(t2);
		}
	}
	else
	{
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
	}
	return s;
}

GLYPHENTRY::GLYPHENTRY(TEXTRECORD* p,BitStream& bs):parent(p)
{
	GlyphIndex = UB(parent->parent->GlyphBits,bs);
	GlyphAdvance = SB(parent->parent->AdvanceBits,bs);
}

SHAPERECORD::SHAPERECORD(SHAPE* p,BitStream& bs):parent(p),next(0)
{
	TypeFlag = UB(1,bs);
	if(TypeFlag)
	{
		StraightFlag=UB(1,bs);
		NumBits=UB(4,bs);
		if(StraightFlag)
		{

			GeneralLineFlag=UB(1,bs);
			if(!GeneralLineFlag)
				VertLineFlag=UB(1,bs);

			if(GeneralLineFlag || !VertLineFlag)
			{
				DeltaX=SB(NumBits+2,bs);
			}
			if(GeneralLineFlag || VertLineFlag)
			{
				DeltaY=SB(NumBits+2,bs);
			}
		}
		else
		{
			
			ControlDeltaX=SB(NumBits+2,bs);
			ControlDeltaY=SB(NumBits+2,bs);
			AnchorDeltaX=SB(NumBits+2,bs);
			AnchorDeltaY=SB(NumBits+2,bs);
			
		}
	}
	else
	{
		StateNewStyles = UB(1,bs);
		StateLineStyle = UB(1,bs);
		StateFillStyle1 = UB(1,bs);
		StateFillStyle0 = UB(1,bs);
		StateMoveTo = UB(1,bs);
		if(StateMoveTo)
		{
			MoveBits = UB(5,bs);
			MoveDeltaX = SB(MoveBits,bs);
			MoveDeltaY = SB(MoveBits,bs);
		}
		if(StateFillStyle0)
		{
			FillStyle0=UB(parent->NumFillBits,bs)+p->fillOffset;
		}
		if(StateFillStyle1)
		{
			FillStyle1=UB(parent->NumFillBits,bs)+p->fillOffset;
		}
		if(StateLineStyle)
		{
			LineStyle=UB(parent->NumLineBits,bs)+p->lineOffset;
		}
		if(StateNewStyles)
		{
			SHAPEWITHSTYLE* ps=dynamic_cast<SHAPEWITHSTYLE*>(parent);
			if(ps==NULL)
				throw ParseException("Malformed SWF file",sys->getOrigin().raw_buf());
			bs.pos=0;
			FILLSTYLEARRAY a;
			a.version=ps->FillStyles.version;
			bs.f >> a;
			p->fillOffset=ps->FillStyles.FillStyleCount;
			ps->FillStyles.appendStyles(a);

			LINESTYLEARRAY b;
			b.version=ps->LineStyles.version;
			bs.f >> b;
			p->lineOffset=ps->LineStyles.LineStyleCount;
			ps->LineStyles.appendStyles(b);

			parent->NumFillBits=UB(4,bs);
			parent->NumLineBits=UB(4,bs);
		}
	}
}

std::istream& lightspark::operator>>(std::istream& stream, CXFORMWITHALPHA& v)
{
	BitStream bs(stream);
	v.HasAddTerms=UB(1,bs);
	v.HasMultTerms=UB(1,bs);
	v.NBits=UB(4,bs);
	if(v.HasMultTerms)
	{
		v.RedMultTerm=SB(v.NBits,bs);
		v.GreenMultTerm=SB(v.NBits,bs);
		v.BlueMultTerm=SB(v.NBits,bs);
		v.AlphaMultTerm=SB(v.NBits,bs);
	}
	if(v.HasAddTerms)
	{
		v.RedAddTerm=SB(v.NBits,bs);
		v.GreenAddTerm=SB(v.NBits,bs);
		v.BlueAddTerm=SB(v.NBits,bs);
		v.AlphaAddTerm=SB(v.NBits,bs);
	}
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, MATRIX& v)
{
	BitStream bs(stream);
	int HasScale=UB(1,bs);
	if(HasScale)
	{
		int NScaleBits=UB(5,bs);
		v.ScaleX=FB(NScaleBits,bs);
		v.ScaleY=FB(NScaleBits,bs);
	}
	int HasRotate=UB(1,bs);
	if(HasRotate)
	{
		int NRotateBits=UB(5,bs);
		v.RotateSkew0=FB(NRotateBits,bs);
		v.RotateSkew1=FB(NRotateBits,bs);
	}
	int NTranslateBits=UB(5,bs);
	v.TranslateX=SB(NTranslateBits,bs)/20;
	v.TranslateY=SB(NTranslateBits,bs)/20;
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BUTTONRECORD& v)
{
	assert(v.buttonVersion==2);
	BitStream bs(stream);

	UB(2,bs);
	v.ButtonHasBlendMode=UB(1,bs);
	v.ButtonHasFilterList=UB(1,bs);
	v.ButtonStateHitTest=UB(1,bs);
	v.ButtonStateDown=UB(1,bs);
	v.ButtonStateOver=UB(1,bs);
	v.ButtonStateUp=UB(1,bs);

	if(v.isNull())
		return stream;

	stream >> v.CharacterID >> v.PlaceDepth >> v.PlaceMatrix >> v.ColorTransform;

	if(v.ButtonHasFilterList)
		stream >> v.FilterList;

	if(v.ButtonHasBlendMode)
	{
		LOG(LOG_ERROR,"Button record not yet totally supported");
	}

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, FILTERLIST& v)
{
	stream >> v.NumberOfFilters;
	v.Filters.resize(v.NumberOfFilters);

	for(int i=0;i<v.NumberOfFilters;i++)
		stream >> v.Filters[i];
	
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, FILTER& v)
{
	stream >> v.FilterID;
	switch(v.FilterID)
	{
		case 0:
			stream >> v.DropShadowFilter;
			break;
		case 2:
			stream >> v.GlowFilter;
			break;
		default:
			LOG(LOG_ERROR,"Unsupported Filter Id " << (int)v.FilterID);
			::abort();
	}
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, GLOWFILTER& v)
{
	//TODO: implement GLOWFILTER parsing
	ignore(stream,4+4+4+2+1);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, DROPSHADOWFILTER& v)
{
	//TODO: implement DROPSHADOWFILTER parsing
	ignore(stream,4+4+4+4+4+2+1);
	return stream;
}

void DictionaryDefinable::define(ASObject* g)
{
	abort();
/*	DictionaryTag* t=p->root->dictionaryLookup(dict_id);
	ASObject* o=dynamic_cast<ASObject*>(t);
	if(o==NULL)
	{
		//Should not happen in real live
		ASObject* ret=new ASObject;
		g->setVariableByName(p->Name,new ASObject);
	}
	else
	{
		ASObject* ret=o->clone();
		if(ret->constructor)
			ret->constructor->call(ret,NULL);
		p->setWrapped(ret);
		g->setVariableByName(p->Name,ret);
	}*/
}

variables_map::~variables_map()
{
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		if(it->second.second.var)
			it->second.second.var->decRef();
		if(it->second.second.setter)
			it->second.second.setter->decRef();
		if(it->second.second.getter)
			it->second.second.getter->decRef();
	}
}

ASObject::ASObject(Manager* m):type(T_OBJECT),ref_count(1),manager(m),cur_level(0),implEnable(true),prototype(NULL)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):type(o.type),ref_count(1),manager(NULL),cur_level(0),implEnable(true),prototype(o.prototype)
{
	if(prototype)
		prototype->incRef();

	#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
	#endif

	assert(o.Variables.size()==0);
}

ASObject::~ASObject()
{
	if(prototype)
		prototype->decRef();
}

int ASObject::_maxlevel()
{
	return (prototype)?(prototype->max_level):0;
}

void ASObject::resetLevel()
{
	cur_level=_maxlevel();
}

Class_base* ASObject::getActualPrototype() const
{
	Class_base* ret=prototype;
	if(ret==NULL)
	{
		assert(type==T_CLASS);
		return NULL;
	}

	for(int i=prototype->max_level;i>cur_level;i--)
		ret=ret->super;

	assert(ret->max_level==cur_level);
	assert(ret);
	return ret;
}

void variables_map::initSlot(unsigned int n, int level, const tiny_string& name, const tiny_string& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n,Variables.end());

	pair<var_iterator, var_iterator> ret=Variables.equal_range(nameAndLevel(name,level));
	if(ret.first!=ret.second)
	{
		//Check if this namespace is already present
		var_iterator start=ret.first;
		for(;start!=ret.second;start++)
		{
			if(start->second.first==ns)
			{
				slots_vars[n-1]=start;
				return;
			}
		}
	}

	//Name not present, no good
	throw RunTimeException("initSlot on missing variable",sys->getOrigin().raw_buf());
}

void variables_map::setSlot(unsigned int n,ASObject* o)
{
	if(n-1<slots_vars.size())
	{
		assert(slots_vars[n-1]!=Variables.end());
		if(slots_vars[n-1]->second.second.setter)
			throw UnsupportedException("setSlot has setters",sys->getOrigin().raw_buf());
		slots_vars[n-1]->second.second.var->decRef();
		slots_vars[n-1]->second.second.var=o;
	}
	else
		throw RunTimeException("setSlot out of bounds",sys->getOrigin().raw_buf());
}

obj_var* variables_map::getValueAt(unsigned int index, int& level)
{
	//TODO: CHECK behavious on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			it++;

		level=it->first.level;
		return &it->second.second;
	}
	else
		throw RunTimeException("getValueAt out of bounds",sys->getOrigin().raw_buf());
}

ASObject* ASObject::getValueAt(int index)
{
	int level;
	obj_var* obj=Variables.getValueAt(index,level);
	assert(obj);
	ASObject* ret;
	if(obj->getter)
	{
		//Call the getter
		LOG(LOG_CALLS,"Calling the getter");
		IFunction* getter=obj->getter->getOverride();
		ret=getter->call(this,NULL,0,level);
		ret->fake_decRef();
		LOG(LOG_CALLS,"End of getter");
	}
	else
		ret=obj->var;

	return ret;
}

tiny_string variables_map::getNameAt(unsigned int index)
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			it++;

		return tiny_string(it->first.name);
	}
	else
		throw RunTimeException("getNameAt out of bounds",sys->getOrigin().raw_buf());
}

unsigned int ASObject::numVariables()
{
	return Variables.size();
}

std::istream& lightspark::operator>>(std::istream& s, CLIPEVENTFLAGS& v)
{
	if(pt->version<=5)
	{
		UI16 t;
		s >> t;
		v.toParse=t;
	}
	else
	{
		s >> v.toParse;
	}
	return s;
}

bool CLIPEVENTFLAGS::isNull()
{
	return toParse==0;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONRECORD& v)
{
	s >> v.EventFlags;
	if(v.EventFlags.isNull())
		return s;
	s >> v.ActionRecordSize;
	LOG(LOG_NOT_IMPLEMENTED,"Skipping " << v.ActionRecordSize << " of action data");
	ignore(s,v.ActionRecordSize);
	return s;
}

bool CLIPACTIONRECORD::isLast()
{
	return EventFlags.isNull();
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONS& v)
{
	s >> v.Reserved >> v.AllEventFlags;
	while(1)
	{
		CLIPACTIONRECORD t;
		s >> t;
		if(t.isLast())
			break;
		v.ClipActionRecords.push_back(t);
	}
	return s;
}

ASObject* lightspark::abstract_d(number_t i)
{
	Number* ret=dManager->get<Number>();
	ret->val=i;
	return ret;
}

ASObject* lightspark::abstract_b(bool i)
{
	return new Boolean(i);
}

ASObject* lightspark::abstract_i(intptr_t i)
{
	Integer* ret=iManager->get<Integer>();
	ret->val=i;
	return ret;
}

void lightspark::stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns)
{
	//Ok, let's split our string into namespace and name part
	for(int i=tmp.len()-1;i>0;i--)
	{
		if(tmp[i]==':')
		{
			assert(tmp[i-1]==':');
			ns=tmp.substr(0,i-1);
			name=tmp.substr(i+1,tmp.len());
			return;
		}
	}
	//No double colons in the string
	name=tmp;
	ns="";
}
