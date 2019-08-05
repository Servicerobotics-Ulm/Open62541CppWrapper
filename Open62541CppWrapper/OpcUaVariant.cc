/*
 * Variant.cc
 *
 *  Created on: May 15, 2019
 *      Author: alexej
 */

#include "OpcUaVariant.hh"

namespace OPCUA {

Variant::Variant()
{
	// custom deleter lambda for NativeVariantType
	auto uaVariantDeleter = [](NativeVariantType *uaVariant) {
		UA_Variant_deleteMembers(uaVariant);
		UA_Variant_delete(uaVariant);
	};
	// use unique pointer with custom deleter (see lambda) to manage the lifecycle of the internal NativeVariantType
	value = { UA_Variant_new(), uaVariantDeleter };
	UA_Variant_init(value.get());
}

/// native Variant copy constructor (makes a deep-copy of the value)
Variant::Variant(const NativeVariantType &uaValueRef)
:	Variant()
{
	UA_Variant_copy(&uaValueRef, this->value.get());
}

/// native Variant copy constructor (optionally takes ownership)
Variant::Variant(NativeVariantType *uaValuePtr, const bool &takeOwnership)
:	Variant()
{
	if(takeOwnership == true) {
		auto uaVariantDeleter = [](NativeVariantType *uaVariant) {
			UA_Variant_deleteMembers(uaVariant);
			UA_Variant_delete(uaVariant);
		};
		this->value.reset(uaValuePtr, uaVariantDeleter);
	} else {
		UA_Variant_copy(uaValuePtr, this->value.get());
	}
}

/// Variant copy constructor
Variant::Variant(const Variant &other)
:	Variant()
{
	UA_Variant_copy(other.value.get(), this->value.get());
}

/// Variant move constructor
Variant::Variant(Variant &&other)
{
	this->value = other.value;
}

void Variant::swap(Variant &other)
{
	this->value.swap(other.value);
}

/// Variant copy assignment operator
Variant& Variant::operator=(const Variant &other)
{
	UA_Variant_deleteMembers(value.get());
	UA_Variant_copy(other.value.get(), this->value.get());
	return *this;
}

/// Variant move assignment operator
Variant& Variant::operator=(Variant &&other)
{
	this->value = other.value;
	return *this;
}

bool Variant::isEmpty() const
{
	return UA_Variant_isEmpty(this->value.get());
}

bool Variant::isScalar() const
{
	if(isEmpty()) {
		return false;
	}
	return UA_Variant_isScalar(this->value.get());
}

std::string Variant::toString() const
{
	if(isEmpty() || !isScalar()) {
		//TODO: implement to-string conversion for array types
		return std::string();
	}

	auto typeIndex = value->type->typeIndex;
	if( typeIndex == UA_TYPES_BOOLEAN) {
		auto boolValue = getValueAs<bool>();
		if(boolValue == true) {
			return std::string("true");
		} else {
			return std::string("false");
		}
	} else if( typeIndex <= UA_TYPES_INT32 ) {
		// all numeric types that fit into 32 bit
		auto intValue = getValueAs<int>();
		return std::to_string(intValue);
	} else if( typeIndex == UA_TYPES_UINT32 ) {
		auto uintValue = getValueAs<unsigned int>();
		return std::to_string(uintValue);
	} else if( typeIndex == UA_TYPES_INT64 ) {
		auto intValue = getValueAs<long int>();
		return std::to_string(intValue);
	} else if( typeIndex == UA_TYPES_UINT64 ) {
		auto uintValue = getValueAs<unsigned long int>();
		return std::to_string(uintValue);
	} else if( typeIndex <= UA_TYPES_DOUBLE ) {
		// floating types
		auto dblValue = getValueAs<double>();
		return std::to_string(dblValue);
	} else if(typeIndex == UA_TYPES_STRING) {
		UA_String *uaStringPtr = static_cast<UA_String*>(value->data);
		// reinterpret cast is quite a sledge hammer here
		return std::string(reinterpret_cast<const char*>(uaStringPtr->data), uaStringPtr->length);
	} else if(typeIndex == UA_TYPES_QUALIFIEDNAME) {
		UA_QualifiedName *uaQname = static_cast<UA_QualifiedName*>(value->data);
		std::string index= std::to_string(uaQname->namespaceIndex);
		std::string simple_name(reinterpret_cast<const char*>(uaQname->name.data), uaQname->name.length);
		return index + ":" + simple_name;
	} else if(typeIndex == UA_TYPES_LOCALIZEDTEXT) {
		UA_LocalizedText *uaText = static_cast<UA_LocalizedText*>(value->data);
		// reinterpret cast is quite a sledge hammer here
		return std::string(reinterpret_cast<const char*>(uaText->text.data), uaText->text.length);
	}
	return std::string();
}

} /* namespace OPCUA */
