/*
 * UNICADO - UNIversity Conceptual Aircraft Design and Optimization
 *
 * Copyright (C) 2025 UNICADO consortium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * This file is part of UNICADO.
 */

#include <aixml/endnode.h>
#include <aixml/node.h>
#include <gtest/gtest.h>

/* == Directives == */
using std::literals::string_literals::operator""s;

/* === Fixtures === */

/**
 * @class SimpleFile
 * @brief Test fixture which provides a simple xml file.
 */
class SimpleFile : public ::testing::Test {
 protected:
  void SetUp() override {
    /* Check if the file exists */
    if (!std::filesystem::exists(file)) {
      FAIL() << "File " << file << " does not exist.";
    }
  }
  const std::filesystem::path file{"../../../aixml/test/stubs/simple.xml"};
};

/**
 * @class SimpleFile
 * @brief Test fixture which provides a simple xml file.
 */
class EndnodeFile : public ::testing::Test {
 protected:
  void SetUp() override {
    /* Check if the file exists */
    if (!std::filesystem::exists(file)) {
      FAIL() << "File " << file << " does not exist.";
    }
  }
  const std::filesystem::path file{"../../../aixml/test/stubs/endnode.xml"};
};

/* === Tests === */
/* Test using the at() operator for a previously defined node
 * Tested function from node.h: node& at(const std::string& s, int depth =
 * std::numeric_limits<int>::max()) const; Expecting equality of a set value for
 * the node entry
 */
TEST(testNodeAtAccessOperator, accessOfExistingNode) {
  node testNode("root");
  std::cout << testNode.getName() << std::endl;
  testNode.appendChild("child1");
  testNode.at("child1") = 5;
  int child1_testVal = testNode.at("child1");
  EXPECT_EQ(child1_testVal, 5);
}

/* Test using the at() operator for a non existing node
 * Tested function from node.h: node& at(const std::string& s, int depth =
 * std::numeric_limits<int>::max()) const; Expect to throw an error of any kind
 */

/* Test using the [] operator for a non existing node
 * Tested function from node.h: node& operator[](std::string s)
 * Expect no throw if element is found correctly
 */
TEST(testNodeAtAccessOperator, accessFindWithoutID) {
  node testNode("/");
  testNode.appendChild("quark");
  node *mynode = testNode.find("quark");
  EXPECT_NO_THROW(testNode.find("quark"));
  EXPECT_EQ(0, mynode->getName().compare("quark"));
}

/* Test using the [] operator for a non existing node
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node but intended ID set is wrong
 */
TEST(testNodeAtAccessOperator, accessFindWithIDSetIncorrect) {
  node testNode("/");
  testNode.appendChild("quark");
  testNode.setAttrib("ID", "0");
  EXPECT_ANY_THROW(testNode.find("quark@0"));
}

/* Test using the [] operator for a non existing node
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node and ID set is correct
 */
TEST(testNodeAtAccessOperator, accessFindWithIDSetCorrect) {
  node testNode("/");
  testNode.appendChild("quark").setAttrib("ID", "0");
  EXPECT_NO_THROW(testNode.find("quark@0"));
}
/* Test using the [] operator for a previously defined node
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting equality of a set value for the node entry
 */
TEST(testNodeBracketAccessOperator, accessOfExistingNode) {
  const std::string name = "root";
  node testNode(name, true);
  testNode.appendChild("child1", true);
  testNode.at("child1") = 5;
  int child1_testVal = testNode["child1"s];
  EXPECT_EQ(child1_testVal, 5);
}

/* Test using the [] operator for a non existing node
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node is created and its value is set to 0.0
 */
TEST(testNodeBracketAccessOperator, accessOfNonExistingNode) {
  node testNode("root");
  testNode["root/child2"s] = 2;
  int child2_testVal = testNode["root/child2"s];
  EXPECT_EQ(child2_testVal, 2);
}

/* Test using the [] operator for a non existing node with id
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node is created and its value is set to 0.0
 */
TEST(testNodeBracketAccessOperator, accessOfNonExistingNodeWithID) {
  node testNode("root");
  testNode["child2@0"s] = 2;
  std::string id = testNode.at("child2@0").getStringAttrib("ID");
  EXPECT_EQ(id, "0");
  int expected_val = 2;
  int is_val = testNode["child2@0"s];
  EXPECT_EQ(expected_val, is_val);
}

/* Test using the [] operator for a non existing node with id
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node fails to create ID 1 with value 2
 * Cant create nodes while lower id's does not exist
 */
TEST(testNodeBracketAccessOperator, accessOfHigherIDsWithoutlowerIDexistence) {
  node testNode("root");
  EXPECT_ANY_THROW(testNode["child2@1"s] = 2);
}

/* Test using the [] operator for a non existing node with id
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node fails to create ID 1 with value 2
 * Cant create nodes while lower id's does not exist
 */
TEST(testNodeBracketAccessOperator, accessOfNonExistingNodeMultipleElements) {
  node testNode("root");
  testNode["root/child2"s] = 2;
  int child2_testVal = testNode["root/child2"s];
  EXPECT_EQ(child2_testVal, 2);

  /* Insert another node at root level */
  testNode["root/child3"s] = 3;
  EXPECT_EQ(static_cast<double>(testNode.at("child3")), 3);
}

/* Test using the [] operator for a non existing node with id
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node set ID correct and assigned value is
 * set correctly
 */
TEST(testNodeBracketAccessOperator, accessOfNonExistingNodeWithIDonMultipleStages) {
  node testNode("root");
  testNode["child@0/what/keks/bla@0"s] = 5;

  std::string id = testNode.at("child@0").getStringAttrib("ID");
  EXPECT_NE(id, "1");
  EXPECT_EQ(id, "0");

  int is_val = testNode.at("child@0/what/keks/bla@0");
  int expected_val = 5;
  EXPECT_EQ(expected_val, is_val);
}

/* Test using the [] operator for a non existing node with id
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that the node cant be set to any ID if first
 * id is not existent
 */
TEST(testNodeBracketAccessOpertaor, noOverwriteNonIDNode) {
  node testNode("root");
  testNode["root/child/newchild"s] = 3.5;
  EXPECT_ANY_THROW(testNode["root/child@0/newchild"s] = 2.1);
  EXPECT_ANY_THROW(testNode["root/child@1/newchild"s] = 2.2);
}

/* Test using the [] operator for a non existing node with id
 * Tested function from node.h: node& operator[](std::string s)
 * Expecting, that values are set correct to nodes with different
 * id's
 */
TEST(testNodeBracketAccessOpertaor, accessOfNonExistingNodeWithMultipleIDs) {
  node testNode("root");
  testNode["child@0/newchild"s] = 3.5;
  testNode["child@1/newchild"s] = 2.1;

  double is_val = testNode["child@0/newchild"s];
  double expected_val = 3.5;

  EXPECT_EQ(expected_val, is_val);

  expected_val = 2.1;
  is_val = testNode["child@1/newchild"s];
  EXPECT_EQ(expected_val, is_val);

  testNode["child@1/newchild/child@0"s] = 5;
  int int_is_val = testNode["child@1/newchild/child@0"s];
  int int_expected_val = 5;
  EXPECT_EQ(int_expected_val, int_is_val);
}

/*
 * Test opening an xml file and returning a shared pointer to the root node.
 */
TEST_F(SimpleFile, openWithSharedPointer) {
  /* Create shared node */
  std::shared_ptr<node> testNode;

  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file);
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }

  /* The shared pointer should be unique */
  EXPECT_EQ(testNode.use_count(), 1);
  EXPECT_TRUE(testNode.unique());

  /* The shared node should contain valid data */
  EXPECT_EQ(testNode->name, file.string());
  ASSERT_TRUE(testNode->find("root/child"));
  EXPECT_EQ(static_cast<int>(testNode->at("root/child")), 1);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value 0
 */
TEST(testIsBooleanFunc, returnTrueFor0) {
  std::string boolToTest = "0";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value 1
 */
TEST(testIsBooleanFunc, returnTrueFor1) {
  std::string boolToTest = "1";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value true
 */
TEST(testIsBooleanFunc, returnTrueFortrue) {
  std::string boolToTest = "true";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value True
 */
TEST(testIsBooleanFunc, returnTrueForTrue) {
  std::string boolToTest = "True";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value TRUE
 */
TEST(testIsBooleanFunc, returnTrueForTRUE) {
  std::string boolToTest = "TRUE";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value false
 */
TEST(testIsBooleanFunc, returnTrueForfalse) {
  std::string boolToTest = "false";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value False
 */
TEST(testIsBooleanFunc, returnTrueForFalse) {
  std::string boolToTest = "False";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting true for value FALSE
 */
TEST(testIsBooleanFunc, returnTrueForFALSE) {
  std::string boolToTest = "FALSE";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string on
 */
TEST(testIsBooleanFunc, returnTrueForStringon) {
  std::string boolToTest = "on";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string On
 */
TEST(testIsBooleanFunc, returnTrueForStringOn) {
  std::string boolToTest = "On";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string ON
 */
TEST(testIsBooleanFunc, returnTrueForStringON) {
  std::string boolToTest = "ON";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string off
 */
TEST(testIsBooleanFunc, returnTrueForStringoff) {
  std::string boolToTest = "off";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string Off
 */
TEST(testIsBooleanFunc, returnTrueForStringOff) {
  std::string boolToTest = "Off";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string OFF
 */
TEST(testIsBooleanFunc, returnTrueForStringOFF) {
  std::string boolToTest = "OFF";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_TRUE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string a
 */
TEST(testIsBooleanFunc, returnFalseForStringa) {
  std::string boolToTest = "a";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_FALSE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string hallo
 */
TEST(testIsBooleanFunc, returnFalseForStringhallo) {
  std::string boolToTest = "hallo";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_FALSE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string 2
 */
TEST(testIsBooleanFunc, returnFalseForString2) {
  std::string boolToTest = "2";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_FALSE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string !
 */
TEST(testIsBooleanFunc, returnFalseForStringExclamationMark) {
  std::string boolToTest = "!";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_FALSE(stringEvaluation);
}

/* Test using the isBoolean function to evaluate a string expression whether it is a boolean or not
 * Tested function from functions.h: bool isBoolean(const std::string &aValue);
 * Expecting False for value string Tru
 */
TEST(testIsBooleanFunc, returnFalseForTru) {
  std::string boolToTest = "Tru";
  bool stringEvaluation = isBoolean(boolToTest);
  EXPECT_FALSE(stringEvaluation);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute true of type boolean*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeTrueIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  testNode.at("child1").addAttrib("Boolean Attribute", true);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute false of type boolean*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeFalseIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  testNode.at("child1").addAttrib("Boolean Attribute", false);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute "True" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringTrueIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "True";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute "true" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringtrueIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "true";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute "TRUE" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringTRUEIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "TRUE";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "false" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringfalseIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "false";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "False" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringFalseIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "False";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "FALSE" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringFALSEIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "FALSE";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute 1 of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeString1IsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "1";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "0" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeString0IsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "0";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute "on" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringonIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "on";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute "On" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringOnIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "On";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a true boolean for an added attribute "ON" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringONIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "ON";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, true);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "off" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringoffIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "off";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "Off" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringOffIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "Off";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to return a false boolean for an added attribute "OFF" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringOFFIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "OFF";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  bool accessBoolAttribute = testNode.at("child1").getBoolAttrib("Boolean Attribute");
  EXPECT_EQ(accessBoolAttribute, false);
}

/* Test the getBoolAttrib function to throw an error for an added attribute "Hallo" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringHalloIsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "Hallo";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  EXPECT_ANY_THROW(testNode.at("child1").getBoolAttrib("Boolean Attribute"));
}

/* Test the getBoolAttrib function to throw an error for an added attribute "-1" of type string*/
TEST(testGetBoolAttrib, testIfSetBooleanAttributeStringMinus1IsValid) {
  node testNode("root");
  testNode.appendChild("child1");
  std::string testExpression = "-1";
  testNode.at("child1").addAttrib("Boolean Attribute", testExpression);
  EXPECT_ANY_THROW(testNode.at("child1").getBoolAttrib("Boolean Attribute"));
}

/* Test appendChildren() function with a vector of strings*/
TEST(testAppendChildren, testIfChildrenCanBeAppendedAsVectorOfStrings) {
  node testNode("root");
  std::vector<std::string> children = {"child1", "child2", "child3"};
  testNode.appendChildren(children, true);
  try {
    testNode.at("child1");
  } catch (...) {
    FAIL() << "child1 was not appended - or could not be accessed";
  }
  try {
    testNode.at("child2");
  } catch (...) {
    FAIL() << "child2 was not appended - or could not be accessed";
  }
  try {
    testNode.at("child3");
  } catch (...) {
    FAIL() << "child3 was not appended - or could not be accessed";
  }
}

/* Test appendChildren() function with a vector of strings and initialize one of the children*/
TEST(testAppendChildren, testIfChildrenCanBeAppendedAsVectorOfStringsAndInitialized) {
  node testNode("root");
  std::vector<std::string> children = {"child1", "child2", "child3"};
  testNode.appendChildren(children, true);
  try {
    testNode.at("child1") = 1;
  } catch (...) {
    FAIL() << "child1 was not appended - or could not be accessed";
  }
  int child1NodeVal = testNode.at("child1");
  EXPECT_EQ(child1NodeVal, 1);
}

/* Test appendChildren() function with a vector of node* */
TEST(testAppendChildren, testIfChildrenCanBeAppendedAsVectorOfNodePt) {
  node testNode("root");
  node *childPt1 = new node("childPt1", true);
  node *childPt2 = new node("childPt2", true);
  node *childPt3 = new node("childPt3", true);
  std::vector<node *> children;
  children.push_back(childPt1);
  children.push_back(childPt2);
  children.push_back(childPt3);
  testNode.appendChildren(children);
  try {
    testNode.at("childPt1");
  } catch (...) {
    FAIL() << "childPt1 was not appended - or could not be accessed";
  }
  try {
    testNode.at("childPt2");
  } catch (...) {
    FAIL() << "childPt2 was not appended - or could not be accessed";
  }
  try {
    testNode.at("childPt3");
  } catch (...) {
    FAIL() << "childPt3 was not appended - or could not be accessed";
  }
}

/* Test appendChildren() function with a vector of node* and initialize a child node */
TEST(testAppendChildren, testIfChildrenCanBeAppendedAsVectorOfNodePtAndInitialized) {
  node testNode("root");
  node *childPt1 = new node("childPt1", true);
  std::vector<node *> children;
  children.push_back(childPt1);
  testNode.appendChildren(children);
  try {
    testNode.at("childPt1") = 1;
  } catch (...) {
    FAIL() << "childPt1 was not appended - or could not be accessed";
  }
  int child1NodeVal = testNode.at("childPt1");
  EXPECT_EQ(child1NodeVal, 1);
}

/* Test appendChildren() function with a vector of node references */
TEST(testAppendChildren, testIfChildrenCanBeAppendedAsVectorOfNodeReferences) {
  node testNode("root");
  node child1("child1", true);
  node child2("child2", true);
  node child3("child3", true);
  std::vector<node> children;
  children.push_back(child1);
  children.push_back(child2);
  children.push_back(child3);
  testNode.appendChildren(children);
  try {
    testNode.at("child1");
  } catch (...) {
    FAIL() << "child1 was not appended - or could not be accessed";
  }
  try {
    testNode.at("child2");
  } catch (...) {
    FAIL() << "child2 was not appended - or could not be accessed";
  }
  try {
    testNode.at("child3");
  } catch (...) {
    FAIL() << "child3 was not appended - or could not be accessed";
  }
}

/* Test appendChildren() function with a vector of references and initialize a child node */
TEST(testAppendChildren, testIfChildrenCanBeAppendedAsVectorOfNodeReferenceAndInitialized) {
  node testNode("root");
  node child1("child1", true);
  std::vector<node> children;
  children.push_back(child1);
  testNode.appendChildren(children);
  try {
    testNode.at("child1") = 1;
  } catch (...) {
    FAIL() << "child1 was not appended - or could not be accessed";
  }
  int child1NodeVal = testNode.at("child1");
  EXPECT_EQ(child1NodeVal, 1);
}

TEST(testEndnodeDouble, testEndnodeUpdate) {
  node testNode("root");
  Endnode<double> foo("root/foo", "description of foo");
  foo.update(testNode);
  EXPECT_NO_THROW(testNode.at("foo/value"));
  EXPECT_NO_THROW(testNode.at("foo/unit"));
  EXPECT_NO_THROW(testNode.at("foo/lower_boundary"));
  EXPECT_NO_THROW(testNode.at("foo/upper_boundary"));
}

TEST(testEndnodeDouble, testEndnodeDefaultValues) {
  node testNode("root");
  Endnode<double> foo("foo", "description of foo");
  foo.update(testNode);
  foo.read(testNode);

  EXPECT_NEAR(0.0, foo.value(), ACCURACY_MEDIUM);
  EXPECT_EQ(foo.unit().compare("1"), 0);
  EXPECT_NEAR(std::numeric_limits<double>::lowest(), foo.lower_boundary(), ACCURACY_MEDIUM);
  EXPECT_NEAR(std::numeric_limits<double>::max(), foo.upper_boundary(), ACCURACY_MEDIUM);
}

TEST(testEndnodeDouble, testEndnodeSetValues) {
  node testNode("root");
  Endnode<double> foo("foo", "description of foo");
  foo.set_value(5.0);
  foo.set_unit("m");
  foo.set_lower_boundary(-5.0);
  foo.set_upper_boundary(10.0);
  foo.update(testNode);
  foo.read(testNode);

  EXPECT_NEAR(5.0, foo.value(), ACCURACY_LOW);
  EXPECT_EQ(foo.unit().compare("m"), 0);
  EXPECT_NEAR(-5.0, foo.lower_boundary(), ACCURACY_MEDIUM);
  EXPECT_NEAR(10.0, foo.upper_boundary(), ACCURACY_MEDIUM);
}

TEST(testEndnodeDouble, testEndnodeCheckUnitFail) {
  node testNode("root");
  Endnode<double> foo("foo", "description of foo");
  EXPECT_THROW(foo.set_unit("olf"), std::invalid_argument);
}

TEST(testEndnodeDouble, testEndnodeCheckUnitSuccess) {
  node testNode("root");
  Endnode<double> foo("foo", "description of foo");
  EXPECT_NO_THROW(foo.set_unit("m"));
}

TEST(testEndnodeDouble, testEndnodeCheckBoundariesTrue) {
  myRuntimeInfo = new runtimeInfo(0, 0, "logfile.log", "test_aixml");
  node testNode("root");
  Endnode<double> foo("foo", "description of foo");
  foo.set_value(5.1);
  foo.set_unit("m");
  foo.set_lower_boundary(5.1);
  foo.set_upper_boundary(10.0);
  foo.update(testNode);
  foo.read(testNode);

  EXPECT_NEAR(5.1, foo.lower_boundary(), ACCURACY_MEDIUM);
  EXPECT_NEAR(10.0, foo.upper_boundary(), ACCURACY_MEDIUM);
  EXPECT_NO_THROW(foo.check_boundaries());

  delete myRuntimeInfo;
}

TEST(testEndnodeDouble, testEndnodeCheckBoundariesFalse) {
  myRuntimeInfo = new runtimeInfo(0, 0, "logfile.log", "test_aixml");
  node testNode("root");
  Endnode<double> foo("foo", "description of foo");
  foo.set_value(5.1);
  foo.set_unit("m");
  foo.set_lower_boundary(5.1001);
  foo.set_upper_boundary(10.0);
  EXPECT_NEAR(5.1001, foo.lower_boundary(), ACCURACY_MEDIUM);
  EXPECT_NEAR(10.0, foo.upper_boundary(), ACCURACY_MEDIUM);
  EXPECT_THROW(foo.update(testNode), std::out_of_range);

  delete myRuntimeInfo;
}

TEST(testEndnodeString, testEndnodeUpdate) {
  node testNode("root");
  Endnode<std::string> foo("foo", "description of foo");
  foo.update(testNode);
  EXPECT_NO_THROW(testNode.at("foo/value"));
  EXPECT_THROW(testNode.at("foo/unit"), std::string);
  EXPECT_THROW(testNode.at("foo/lower_boundary"), std::string);
  EXPECT_THROW(testNode.at("foo/upper_boundary"), std::string);
}

TEST(testEndnodeString, testEndnodeDefaultValues) {
  node testNode("root");
  Endnode<std::string> foo("foo", "description of foo");
  foo.update(testNode);
  foo.read(testNode);
  EXPECT_EQ(foo.value().compare(""), 0);
}

TEST(testEndnodeBool, testEndnodeDefaultValues) {
  node testNode("root");
  Endnode<bool> foo("foo", "description of foo");
  foo.update(testNode);
  EXPECT_TRUE(foo.value());
}

TEST(testEndnodeBool, testEndnodeCheckWriteInXmlTrue) {
  node testNode("root");
  Endnode<bool> foo("foo", "description of foo");
  foo.update(testNode);

  EXPECT_TRUE(foo.value());
  std::string expect_value = testNode.at("foo/value");

  EXPECT_EQ(expect_value.compare("true"), 0);
}

TEST(testEndnodeBool, testEndnodeCheckWriteInXmlFalse) {
  node testNode("root");
  Endnode<bool> foo("foo", "description of foo");
  foo.set_value(false);
  foo.update(testNode);

  EXPECT_FALSE(foo.value());
  std::string expect_value = testNode.at("foo/value");

  EXPECT_EQ(expect_value.compare("false"), 0);
}

TEST(EndnodeOperator, addOperator) {
  /* Create shared node */
  Endnode<double> foo("foo", "myfoo", 0.5);
  foo += 1.0;

  EXPECT_DOUBLE_EQ(1.5, foo.value());
}

TEST(EndnodeOperator, subtractOperator) {
  /* Create shared node */
  Endnode<double> foo("foo", "myfoo", 0.5);
  foo -= 1.0;

  EXPECT_DOUBLE_EQ(-0.5, foo.value());
}

TEST(EndnodeOperator, multiplyOperator) {
  /* Create shared node */
  Endnode<double> foo("foo", "myfoo", 0.5);
  foo *= 3.0;

  EXPECT_DOUBLE_EQ(1.5, foo.value());
}

TEST(EndnodeOperator, divideOperator) {
  /* Create shared node */
  Endnode<double> foo("foo", "myfoo", 0.5);
  foo /= 0.25;

  EXPECT_DOUBLE_EQ(2.0, foo.value());
}

TEST(EndnodeAdditional, unitMethodSet) {
  Endnode<double> foo("foo", "myfoo", 5.0, "kg", 0.5, 6.0);

  EXPECT_EQ(foo.unit().compare("kg"), 0);
  foo.set_unit("s");
  EXPECT_EQ(foo.unit().compare("s"), 0);
}

TEST(EndnodeAdditional, unitMethodThrow) {
  Endnode<double> foo("foo", "myfoo", 5.0, "kg", 0.5, 6.0);

  EXPECT_THROW(foo.set_unit("mm"), std::invalid_argument);
}

TEST_F(EndnodeFile, testEndnodeReadBoolFromXml) {
  /* Create shared node */
  std::shared_ptr<node> testNode;

  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file);
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }

  EndnodeReadOnly<bool> foo("bool");
  foo.read(*testNode.get());
  EXPECT_TRUE(foo.value());
}

TEST_F(EndnodeFile, testEndnodeRuntimeInfoUninitialized) {
  /* Create shared node */
  std::shared_ptr<node> testNode;

  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file);
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }

  /* Set myRuntimeInfo to nullptr */
  myRuntimeInfo = nullptr;
  EndnodeReadOnly<bool> foo("foobar");
  Endnode<bool> bar("barfoo", "");

  /* Read from file -> should fail with message */
  foo.read(*testNode.get());
  bar.read(*testNode.get());

  EXPECT_EQ(myRuntimeInfo, nullptr);
  /* Restore myRuntimeInfo pointer */

  /* If tests does not lead to segmentation fault -> successful */
}

TEST_F(EndnodeFile, testEndnodeRuntimeInfoInitialized) {
  /* Create shared node */
  std::shared_ptr<node> testNode;
  myRuntimeInfo = new runtimeInfo(0, 0, "logfile.log", "test_aixml");
  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file);
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }
  EndnodeReadOnly<bool> foo("foobar");
  Endnode<bool> bar("barfoo", "");
  /* Read from file -> should fail with message via runtimeinfo */
  foo.read(*testNode.get());
  bar.read(*testNode.get());
  EXPECT_NE(myRuntimeInfo, nullptr);
  /* If tests does not lead to segmentation fault -> successful */
  delete myRuntimeInfo;
}

TEST_F(EndnodeFile, boundaryCheckFailure) {
  /* Create shared node */
  std::shared_ptr<node> testNode;

  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file);
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }

  Endnode<double> foo("foo","myfoo",-0.5,"1",0.0,1.0);
  EXPECT_THROW(foo.update(*testNode.get()),std::out_of_range);
}


TEST_F(EndnodeFile, boundaryCheckReadFailure) {
  /* Create shared node */
  std::shared_ptr<node> testNode;

  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file); 
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }

  Endnode<double> foo("faildouble","new description");
  EXPECT_THROW(foo.read(*testNode.get()), std::out_of_range);
}


TEST_F(EndnodeFile, boundaryCheckReadOnlyFailure) {
  /* Create shared node */
  std::shared_ptr<node> testNode;

  /* Open the file which should not throw */
  try {
    testNode = aixml::openDocument(file);
  } catch (...) {
    FAIL() << "Opening the file did throw an exception!";
  }

  EndnodeReadOnly<double> foo("faildouble");
  EXPECT_THROW(foo.read(*testNode.get()), std::out_of_range);
}