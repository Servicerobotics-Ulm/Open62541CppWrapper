/*
 * OpcUaVariant.cc
 *
 *  Created on: May 15, 2019
 *      Author: alexej
 */

#include "OpcUaVariant.hh"

#include <gtest/gtest.h>

namespace OPCUA {

class VariantTestCase : public ::testing::Test {
protected:

	Variant var_null { };

	Variant var_b1 { true };
	Variant var_i1 { 100 };
	Variant var_d1 { 0.5 };
	Variant var_s1 { std::string("Hello") };

	std::vector<bool> barr1 = { true, false };
	Variant var_vb1 { barr1 };
	std::vector<int> iarr1 = { 100, 200 };
	Variant var_vi1 { iarr1 };
	std::vector<double> darr1 = { 0.5, 1.5 };
	Variant var_vd1 { darr1 };
	std::vector<std::string> sarr1 = { "Hello", "World" };
	Variant var_vs1 { sarr1 };
};

TEST_F(VariantTestCase, InitialTest) {
	EXPECT_TRUE(var_null.isEmpty());

	// scalar examples
	EXPECT_FALSE(var_b1.isEmpty());
	EXPECT_FALSE(var_i1.isEmpty());
	EXPECT_FALSE(var_d1.isEmpty());
	EXPECT_FALSE(var_s1.isEmpty());

	EXPECT_TRUE(var_b1.isScalar());
	EXPECT_TRUE(var_i1.isScalar());
	EXPECT_TRUE(var_d1.isScalar());
	EXPECT_TRUE(var_s1.isScalar());

	// vector examples
	EXPECT_FALSE(var_vb1.isEmpty());
	EXPECT_FALSE(var_vi1.isEmpty());
	EXPECT_FALSE(var_vd1.isEmpty());
	EXPECT_FALSE(var_vs1.isEmpty());

	EXPECT_FALSE(var_vb1.isScalar());
	EXPECT_FALSE(var_vi1.isScalar());
	EXPECT_FALSE(var_vd1.isScalar());
	EXPECT_FALSE(var_vs1.isScalar());
}

TEST_F(VariantTestCase, ImplicitCppValueConstructorTest) {
	Variant var_b2 { true };
	EXPECT_EQ(var_b2.getValueAs<bool>(), true);
	Variant var_i2 { 100 };
	EXPECT_EQ(var_i2.getValueAs<int>(), 100 );
	Variant var_d2 { 5.0 };
	EXPECT_EQ(var_d2.getValueAs<double>(), 5.0);
	Variant var_s2 { std::string("Hello") };
	EXPECT_STREQ(var_s2.getValueAs<std::string>().c_str(), "Hello");
}

TEST_F(VariantTestCase, ImplicitCppValueAssigmentTest) {
	var_b1 = false;
	ASSERT_EQ(var_b1.getValueAs<bool>(), false);
	var_i1 = 333;
	ASSERT_EQ(var_i1.getValueAs<int>(), 333);
	var_d1 = 44.33;
	ASSERT_EQ(var_d1.getValueAs<double>(), 44.33);
	var_s1 = std::string("World");
	ASSERT_STREQ(var_s1.getValueAs<std::string>().c_str(), "World");
}

TEST_F(VariantTestCase, GetAsValueTest) {
	ASSERT_EQ(var_b1.getValueAs<bool>(), true);
	ASSERT_EQ(var_i1.getValueAs<int>(), 100);
	ASSERT_EQ(var_d1.getValueAs<double>(), 0.5);
	ASSERT_STREQ(var_s1.getValueAs<std::string>().c_str(), "Hello");
}

TEST_F(VariantTestCase, ImplicitValueCoversionTest) {
	bool b1 = var_b1;
	EXPECT_EQ(b1, true);
	int i1 = var_i1;
	EXPECT_EQ(i1, 100);
	double d1 = var_d1;
	EXPECT_EQ(d1, 0.5);
	// the string constructor is ambiguous
	// (so sometimes the explicit conversion method getValueAs<std::string>() has to be used)
	std::string s1 = var_s1;
	EXPECT_STREQ(s1.c_str(), "Hello");

	// now using assignment operator
	b1 = var_b1;
	EXPECT_EQ(b1, true);
	i1 = var_i1;
	EXPECT_EQ(i1, 100);
	d1 = var_d1;
	EXPECT_EQ(d1, 0.5);
	// the string constructor is ambiguous (so the explicit conversion has to be used here)
	s1 = var_s1.getValueAs<std::string>();
	EXPECT_STREQ(s1.c_str(), "Hello");
}

TEST_F(VariantTestCase, GetAsArrayValueTest) {
	auto vb1_res = var_vb1.getArrayValuesAs<bool>();
	for(size_t i=0; i<barr1.size(); ++i) {
		EXPECT_EQ(barr1[i], vb1_res[i]);
	}

	auto vi1_res = var_vi1.getArrayValuesAs<int>();
	for(size_t i=0; i<iarr1.size(); ++i) {
		EXPECT_EQ(iarr1[i], vi1_res[i]);
	}

	auto vd1_res = var_vd1.getArrayValuesAs<double>();
	for(size_t i=0; i<darr1.size(); ++i) {
		EXPECT_EQ(darr1[i], vd1_res[i]);
	}

	auto vs1_res = var_vs1.getArrayValuesAs<std::string>();
	for(size_t i=0; i<sarr1.size(); ++i) {
		EXPECT_STREQ(sarr1[i].c_str(), vs1_res[i].c_str());
	}

	// now the same procedure with the implicit conversion operator
	std::vector<bool> vb2_res = var_vb1;
	for(size_t i=0; i<barr1.size(); ++i) {
		EXPECT_EQ(barr1[i], vb2_res[i]);
	}

	std::vector<int> vi2_res = var_vi1;
	for(size_t i=0; i<iarr1.size(); ++i) {
		EXPECT_EQ(iarr1[i], vi2_res[i]);
	}

	std::vector<double> vd2_res = var_vd1;
	for(size_t i=0; i<darr1.size(); ++i) {
		EXPECT_EQ(darr1[i], vd2_res[i]);
	}

	std::vector<std::string> vs2_res = var_vs1;
	for(size_t i=0; i<sarr1.size(); ++i) {
		EXPECT_STREQ(sarr1[i].c_str(), vs2_res[i].c_str());
	}
}

TEST_F(VariantTestCase, NativeTypeCopyTest) {
	UA_Int32 ua_i1 = 123;

	// manually create native type on heap
	::UA_Variant *uaVariantPtr = UA_Variant_new();
	UA_Variant_init(uaVariantPtr);
	UA_Variant_setScalarCopy(uaVariantPtr, &ua_i1, &UA_TYPES[UA_TYPES_INT32]);
	bool takeOwnership = true;
	Variant testVar1 { uaVariantPtr, takeOwnership };
	int i1 = testVar1;
	EXPECT_EQ(i1, ua_i1);

	// manually create native type on heap
	::UA_Variant *uaVariantPtr2 = UA_Variant_new();
	UA_Variant_init(uaVariantPtr2);
	UA_Variant_setScalarCopy(uaVariantPtr2, &ua_i1, &UA_TYPES[UA_TYPES_INT32]);
	takeOwnership = false; // means we deep-copy the value, so we can delete the original pointer
	Variant testVar2 { uaVariantPtr, takeOwnership };
	UA_Variant_delete(uaVariantPtr2);
	int i2 = testVar2;
	EXPECT_EQ(i2, ua_i1);

	// create and fill native type on stack
	::UA_Variant uaVariant;
	UA_Variant_init(&uaVariant);
	UA_Variant_setScalarCopy(&uaVariant, &ua_i1, &UA_TYPES[UA_TYPES_INT32]);
	Variant testVar3 { uaVariant };
	UA_Variant_deleteMembers(&uaVariant);
	int i3 = testVar3;
	EXPECT_EQ(i3, ua_i1);
}

TEST_F(VariantTestCase, SharedPtrTest) {
	auto ptr_it = var_i1.getInternalValuePtr();
	ASSERT_TRUE(ptr_it); // check that the pointer is valid
}

TEST_F(VariantTestCase, CopyConstructorTest) {
	auto copy_b1 = var_b1;
	ASSERT_EQ(var_b1.getValueAs<bool>(), copy_b1.getValueAs<bool>());

	auto copy_i1 = var_i1;
	ASSERT_EQ(var_i1.getValueAs<int>(), copy_i1.getValueAs<int>());

	auto copy_d1 = var_d1;
	ASSERT_EQ(var_d1.getValueAs<double>(), copy_d1.getValueAs<double>());

	auto copy_s1 = var_s1;
	ASSERT_STREQ(var_s1.getValueAs<std::string>().c_str(), copy_s1.getValueAs<std::string>().c_str());
}

TEST_F(VariantTestCase, MoveConstructorTest) {
	auto copy_b1 = std::move(var_b1);
	ASSERT_EQ(true, copy_b1.getValueAs<bool>());

	auto copy_i1 = std::move(var_i1);
	ASSERT_EQ(100, copy_i1.getValueAs<int>());

	auto copy_d1 = std::move(var_d1);
	ASSERT_EQ(0.5, copy_d1.getValueAs<double>());

	auto copy_s1 = std::move(var_s1);
	ASSERT_STREQ("Hello", copy_s1.getValueAs<std::string>().c_str());
}

TEST_F(VariantTestCase, SwapTest) {
	auto var_i2 = Variant{};
	var_i2.setValueFrom<int>(200);
	var_i2.swap(var_i1);
	ASSERT_EQ(var_i1.getValueAs<int>(), 200);
	ASSERT_EQ(var_i2.getValueAs<int>(), 100);
}

TEST_F(VariantTestCase, AssignmentCopyTest) {
	auto copy_b1 = Variant{};
	copy_b1 = var_b1;
	ASSERT_EQ(var_b1.getValueAs<bool>(), copy_b1.getValueAs<bool>());

	auto copy_i1 = Variant{};
	copy_i1 = var_i1;
	ASSERT_EQ(var_i1.getValueAs<int>(), copy_i1.getValueAs<int>());

	auto copy_d1 = Variant{};
	copy_d1 = var_d1;
	ASSERT_EQ(var_d1.getValueAs<double>(), copy_d1.getValueAs<double>());

	auto copy_s1 = Variant{};
	copy_s1 = var_s1;
	ASSERT_STREQ(var_s1.getValueAs<std::string>().c_str(), copy_s1.getValueAs<std::string>().c_str());
}

TEST_F(VariantTestCase, AssignmentMoveTest) {
	auto copy_i1 = Variant{};
	copy_i1 = std::move(var_i1);
	ASSERT_EQ(100, copy_i1.getValueAs<int>());
}

TEST_F(VariantTestCase, ToStringTest) {
	auto sb1 = var_b1.toString();
	ASSERT_STREQ("true", sb1.c_str());

	auto si1 = var_i1.toString();
	ASSERT_STREQ("100", si1.c_str());

	auto s1 = var_s1.toString();
	ASSERT_STREQ("Hello", s1.c_str());

	s1 = std::to_string(var_d1.getValueAs<double>());
	auto sd1 = var_d1.toString();
	ASSERT_STREQ(sd1.c_str(), s1.c_str());

	//TODO: test variant types
}

} // end namespace OPCUA
