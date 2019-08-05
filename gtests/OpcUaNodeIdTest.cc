/*
 * OpcUaNodeIdTest.cc
 *
 *  Created on: Oct 12, 2018
 *      Author: alexej
 */

#include "OpcUaNodeId.hh"


#include <gtest/gtest.h>

namespace OPCUA {

class NodeIdTest : public ::testing::Test {
protected:
	NodeIdTest() {
		id_num1 = NodeId(100);
		id_num2 = NodeId(200);
		id_num3 = NodeId(200);
		id_str = NodeId("Hallo");
	}

	NodeId id0;
	NodeId id_num1;
	NodeId id_num2;
	NodeId id_num3;
	NodeId id_str;
};

TEST_F(NodeIdTest, EmptynessTest) {
	EXPECT_TRUE(id0.isNull());
	EXPECT_FALSE(id_num1.isNull());
	EXPECT_FALSE(id_num2.isNull());
	EXPECT_FALSE(id_str.isNull());
}

TEST_F(NodeIdTest, ComparisonTest) {
	// id1 < id2
	EXPECT_LT(id_num1, id_num2);
	// id1 != id2
	EXPECT_NE(id_num1, id_num2);
	// id2 == id3
	EXPECT_EQ(id_num2, id_num3);
}

TEST_F(NodeIdTest, ObjectCopyTest) {
	NodeId id_test1(id_num1);
	EXPECT_EQ(id_test1, id_num1);

	NodeId id_test2(id_str);
	EXPECT_EQ(id_test2, id_str);

	NodeId id_test3 = id_num3;
	EXPECT_EQ(id_test3, id_num3);

	NodeId id_test4;
	{
		id_test4 = NodeId(400);
	}
	EXPECT_EQ(id_test4, NodeId(400));
}

TEST_F(NodeIdTest, NativeTypeTest) {
	NodeId::NativeIdType type = id_num1;
	NodeId id1(type);
	EXPECT_EQ(id1, id_num1);
}

TEST_F(NodeIdTest, SimpleNameTest) {
	std::string s1 = id_num1.getSimpleName();
	EXPECT_EQ(s1, "100");
	std::string s2 = id_str.getSimpleName();
	EXPECT_EQ(s2, "Hallo");
}

} // end namespace OPCUA

