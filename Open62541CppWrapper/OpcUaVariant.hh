/*
 * Variant.hh
 *
 *  Created on: May 15, 2019
 *      Author: alexej
 */

#ifndef OPEN62541CPPWRAPPER_OPCUAVARIANT_HH_
#define OPEN62541CPPWRAPPER_OPCUAVARIANT_HH_

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <iostream>

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

namespace OPCUA {

// this mapping simplifies the transformations of native C++ types to/from open62541 UA types (see template methods below)
static std::unordered_map<std::type_index, UA_UInt16> UA_TypeMappings =
{
		{ std::type_index(typeid(UA_Boolean)), UA_TYPES_BOOLEAN },
		{ std::type_index(typeid(UA_SByte)), UA_TYPES_SBYTE },
		{ std::type_index(typeid(UA_Byte)), UA_TYPES_BYTE },
		{ std::type_index(typeid(UA_Int16)), UA_TYPES_INT16 },
		{ std::type_index(typeid(UA_UInt16)), UA_TYPES_UINT16 },
		{ std::type_index(typeid(UA_Int32)), UA_TYPES_INT32 },
		{ std::type_index(typeid(UA_UInt32)), UA_TYPES_UINT32 },
		{ std::type_index(typeid(UA_Int64)), UA_TYPES_INT64 },
		{ std::type_index(typeid(UA_UInt64)), UA_TYPES_UINT64 },
		{ std::type_index(typeid(UA_Float)), UA_TYPES_FLOAT },
		{ std::type_index(typeid(UA_Double)), UA_TYPES_DOUBLE }
};

/** This class wraps the functionality of the native Variant struct from the open62541 library.
 *
 *  There are two major goals of this class:
 *  - automating the memory management of the native Variant using a shared_ptr
 *  - implementing conversion operators of plain C++ types and of std::string into/from the Variant
 */
class Variant {
public:
	/// this class internally uses the open62541 Variant as the native and generic OPC UA type
	using NativeVariantType = ::UA_Variant;

	/// Variant default constructor (initializes internal native value to null)
	Variant();

	/// native OPC UA type copy constructor (makes a deep-copy of the value)
	explicit Variant(const NativeVariantType &value);

	/// native OPC UA type pointer constructor (can take ownership of the passed value, otherwise a deep-copy is performed)
	explicit Variant(NativeVariantType *valuePtr, const bool &takeOwnership);

	/// implicitly convert from plain C++ types and from std::string
	template <typename VALUE_TYPE>
	Variant(const VALUE_TYPE &value)
	:	Variant()
	{
		this->setValueFrom(value);
	}

	/// implicitly convert from plain C++ types and from std::string in a std::vector
	template <typename VALUE_TYPE>
	Variant(const std::vector<VALUE_TYPE> &value)
	:	Variant()
	{
		this->setArrayValueFrom(value);
	}

	/// Variant copy constructor
	Variant(const Variant &other);

	/// Variant move constructor
	Variant(Variant &&other);

	/// swap internal values
	void swap(Variant &value);

	/// Variant copy assignment operator
	Variant& operator=(const Variant &other);

	/// Variant move assignment operator
	Variant& operator=(Variant &&other);

	/// checks whether the internal value has been setup to any actual value
	bool isEmpty() const;

	/// check whether the current internal representation is an array or a scalar type
	bool isScalar() const;

	/// get the internal value converted into the provided value-type
	template <typename VALUE_TYPE>
	VALUE_TYPE getValueAs() const;

	/// get the internal values converted into a vector of provided value-type
	template <typename VALUE_TYPE>
	std::vector<VALUE_TYPE> getArrayValuesAs() const;

	/// implicit conversion operator for primitive C++ types
	template <typename VALUE_TYPE>
	operator VALUE_TYPE() const {
		return getValueAs<VALUE_TYPE>();
	}

	/// implicit conversion operator for primitive C++ array types
	template <typename VALUE_TYPE>
	operator std::vector<VALUE_TYPE>() const {
		return getArrayValuesAs<VALUE_TYPE>();
	}

	/// set the internal value from a native C++ value type
	template <typename VALUE_TYPE>
	void setValueFrom(const VALUE_TYPE &value);

	/// set the internal value from a native C++ value type
	template <typename VALUE_TYPE>
	void setArrayValueFrom(const std::vector<VALUE_TYPE> &value);

	/// implicit conversion from plain C++ types and from std::string
	template <typename VALUE_TYPE>
	Variant& operator=(const VALUE_TYPE &value) {
		this->setValueFrom(value);
		return *this;
	}

	/// implicit conversion from plain C++ types and from std::string in a std::vector
	template <typename VALUE_TYPE>
	Variant& operator=(const std::vector<VALUE_TYPE> &value) {
		this->setArrayValueFrom(value);
		return *this;
	}

	/// to string conversion
	std::string toString() const;

	/// return a shared-pointer to internal NativeVariantType value
	inline std::shared_ptr<NativeVariantType> getInternalValuePtr() const {
		return value;
	}

	inline NativeVariantType getInternalValueCopy() const {
		NativeVariantType valueCopy;
		UA_Variant_init(&valueCopy);
		UA_Variant_copy(value.get(), &valueCopy);
		return valueCopy;
	}

private:
	// NativeVariantType is initialized in a shared_pointer (that can be accessed via getInternalValuePtr())
	std::shared_ptr<NativeVariantType> value;
};

template <>
inline std::string Variant::getValueAs() const
{
	if(!isScalar() || isEmpty()) {
		return std::string();
	}
	// always convert to a string if string is required (regardless of the actual internal type)
	return toString();
}

template <typename SIMPLE_VALUE_TYPE>
inline SIMPLE_VALUE_TYPE Variant::getValueAs() const
{
	static_assert(std::is_fundamental<SIMPLE_VALUE_TYPE>::value,
			                  "Value-Type has to be one of the fundamental types");
	if(!isScalar() || isEmpty()) {
		return 0;
	}
	// one could check if the internal value-type matches the given value-type as follows:
//	if(value->type->typeIndex == UA_TypeMappings[std::type_index(typeid(SIMPLE_VALUE_TYPE))]);
	//however, this check might be too restrictive
	return *(static_cast<SIMPLE_VALUE_TYPE *>(value->data));
}


template <>
inline std::vector<std::string> Variant::getArrayValuesAs() const
{
	if(isEmpty()) {
		return std::vector<std::string>();
	} else if(isScalar()) {
		return std::vector<std::string>(1, toString());
	}

	// always convert to a string if string is required (regardless of the actual internal type)
	std::vector<std::string> result { };
	if(value->type->typeIndex == UA_TYPES_STRING) {
		UA_String *uaStrArray = static_cast<UA_String*>(value->data);
		for(size_t i=0; i<value->arrayLength; ++i) {
			std::string str( (const char*)uaStrArray[i].data, uaStrArray[i].length);
			result.push_back( str );
		}
	}
	return result;
}

/// get the internal values converted into a vector of provided value-type
template <typename VALUE_TYPE>
std::vector<VALUE_TYPE> Variant::getArrayValuesAs() const
{
	static_assert(std::is_fundamental<VALUE_TYPE>::value,
			                  "Value-Type has to be one of the fundamental types");
	if(isEmpty()) {
		return std::vector<VALUE_TYPE>();
	} else if(isScalar()) {
		return std::vector<VALUE_TYPE>(1, getValueAs<VALUE_TYPE>());
	}

	std::vector<VALUE_TYPE> result(value->arrayLength);
	VALUE_TYPE* internalVectorPtr = static_cast<VALUE_TYPE*>(value->data);
	std::copy(internalVectorPtr, internalVectorPtr + value->arrayLength, result.begin());
	return result;
}

template <>
inline void Variant::setValueFrom(const std::string &text)
{
	UA_Variant_deleteMembers(this->value.get());
	UA_Variant_init(this->value.get());
	// shallow-copy the string
	UA_String ua_s;
	ua_s.length = text.length();
	ua_s.data = (UA_Byte*)text.c_str();
	// set string variant
	UA_Variant_setScalarCopy(this->value.get(), &ua_s, &UA_TYPES[UA_TYPES_STRING]);
}

template <typename VALUE_TYPE>
inline void Variant::setValueFrom(const VALUE_TYPE &value)
{
	static_assert(std::is_fundamental<VALUE_TYPE>::value,
	                  "Value-Type has to be one of the fundamental types");

	UA_Variant_deleteMembers(this->value.get());
	UA_Variant_init(this->value.get());
	// use type_traits to get the right mapping from provided value-type to the internal value-type index (see UA_TypeMappings)
	UA_Variant_setScalarCopy(this->value.get(), &value, &UA_TYPES[UA_TypeMappings[std::type_index(typeid(VALUE_TYPE))]]);
}

template <>
inline void Variant::setArrayValueFrom(const std::vector<std::string> &textArray)
{
	UA_Variant_deleteMembers(this->value.get());
	UA_Variant_init(this->value.get());

	// shallow-copy the strings into a local array
	UA_String uaStrArray[textArray.size()];
	for(size_t i=0; i<textArray.size(); ++i) {
		uaStrArray[i].length = textArray[i].length();
		uaStrArray[i].data = (UA_Byte*)textArray[i].c_str();
	}
	// don't use the string vector directly as it has a different internal data structure
	UA_Variant_setArrayCopy(this->value.get(), uaStrArray, textArray.size(), &UA_TYPES[UA_TYPES_STRING]);

	// a vector has a single dimension
	value->arrayDimensionsSize = 1;
	// set array dimension
	value->arrayLength = textArray.size();
	// set the single array dimension-size
	value->arrayDimensions = UA_UInt32_new();
	value->arrayDimensions[0] = textArray.size();
}

template <>
inline void Variant::setArrayValueFrom(const std::vector<bool> &arrayValue)
{
	::UA_Variant_deleteMembers(this->value.get());
	::UA_Variant_init(this->value.get());

	// create a copy of the boolean array values
	UA_Boolean uaBoolArray[arrayValue.size()];
	std::copy(arrayValue.cbegin(), arrayValue.cend(), uaBoolArray);
	// don't use the bool vector directly as it has an optimized internal data structure
	UA_Variant_setArrayCopy(this->value.get(), uaBoolArray, arrayValue.size(), &UA_TYPES[UA_TYPES_BOOLEAN]);

	// a vector has a single dimension
	value->arrayDimensionsSize = 1;
	// set array dimension
	value->arrayLength = arrayValue.size();
	// set the single array dimension-size
	value->arrayDimensions = UA_UInt32_new();
	value->arrayDimensions[0] = arrayValue.size();
}

template <typename VALUE_TYPE>
inline void Variant::setArrayValueFrom(const std::vector<VALUE_TYPE> &arrayValue)
{
	static_assert(std::is_fundamental<VALUE_TYPE>::value,
	                  "Value-Type has to be one of the fundamental types");
	UA_Variant_deleteMembers(this->value.get());
	UA_Variant_init(this->value.get());

	// use type_traits to get the right mapping from provided value-type to the internal value-type index (see UA_TypeMappings)
	UA_Variant_setArrayCopy(this->value.get(), arrayValue.data(), arrayValue.size(), &UA_TYPES[UA_TypeMappings[std::type_index(typeid(VALUE_TYPE))]]);

	// a vector has a single dimension
	value->arrayDimensionsSize = 1;
	// set array dimension
	value->arrayLength = arrayValue.size();
	// set the single array dimension-size
	value->arrayDimensions = UA_UInt32_new();
	value->arrayDimensions[0] = arrayValue.size();
}

/// ostream operator
inline std::ostream& operator << (std::ostream& os, const Variant& var)
{
	os << var.toString();
	return os;
}

} /* namespace OPCUA */

#endif /* OPEN62541CPPWRAPPER_OPCUAVARIANT_HH_ */
