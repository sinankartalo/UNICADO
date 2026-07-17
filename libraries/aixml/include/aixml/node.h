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

#ifndef AIXML_INCLUDE_AIXML_NODE_H_
#define AIXML_INCLUDE_AIXML_NODE_H_
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef BUILD_AIXML_SHARED
#ifdef _WIN32
#define AIXMLDLLEXPORT __declspec(dllexport)
#else
#define AIXMLDLLEXPORT __attribute__((visibility("default")))
#endif
#elif defined(IMPORT_AIXML_SHARED)
#ifdef _WIN32
#define AIXMLDLLEXPORT __declspec(dllimport)
#else
#define AIXMLDLLEXPORT
#endif
#else
#define AIXMLDLLEXPORT
#endif

#define AIXMLPRECISION 10

AIXMLDLLEXPORT void split(const std::string& s, char delim, std::vector<std::string>* elems);
AIXMLDLLEXPORT std::vector<std::string> split(const std::string& s, char delim);

class node; /**< Forward declaration for namespace */
class TiXmlNode;
class TiXmlElement;
class TiXmlDocument;

namespace aixml {
/** \brief Function generates xml-tree of file, returns reference to the root-node
 * \param filename
 */
AIXMLDLLEXPORT node& openDocument(std::string fileName);

/** \brief Function generates xml-tree of file, returns a shared pointer to the root-node
 * \param filename
 */
AIXMLDLLEXPORT auto openDocument(const std::filesystem::path& fileName) -> std::shared_ptr<node>;

/** \brief Function saves the XML-tree to the file, throws error if toolLevel is smaller than the ToolLevel-attribute in
 * a modified element \param root \param toolLevel
 */
AIXMLDLLEXPORT bool saveDocument(const node& root, int toolLevel = 0);

/** \brief Function deletes xml tree
 * \param root
 */
AIXMLDLLEXPORT void closeDocument(const node& root);

/** \brief Function checks weather an XML-File ist path compatible with another (schema)XML
 * \param schemaXML
 * \param xmlToBeTested
 */
AIXMLDLLEXPORT bool checkSchema(const node& schemaXML, const node& xmlToBeTested);

/** \brief Function applies scheme to xmlToBeUpdated, empty nodes get added to xmlToBeUpdated if missing
 * \param schemaXML
 * \param xmlToBeUpdated
 */
AIXMLDLLEXPORT void applySchema(const node& schemaXML, node* xmlToBeUpdated);

/** \brief Function returns a single parent node including childs and not the full document
 * \param filename
 * \param s
 * \param depth
 */
AIXMLDLLEXPORT node getSingleNode(const std::string& filename, std::string s,
                                  int depth = std::numeric_limits<int>::max());
} /* namespace aixml */

/**< \brief Class for the description of node
 */
class AIXMLDLLEXPORT node {
 private:
  enum class valueType { UNDEFINED, STRING, DOUBLE }; /**< INT,BOOL */
 public:
  node();
  explicit node(const std::string& name, bool writeToXml = true);
  node(const std::string& name, const std::string& value, bool writeToXml = true);
  node(const std::string& name, node* parent, bool writeToXml = true);
  node(const std::string& name, const std::string& value, node* parent, bool writeToXml = true);
  node(const node& nodeToCopy);
  virtual ~node();

  AIXMLDLLEXPORT friend node& aixml::openDocument(std::string fileName);
  AIXMLDLLEXPORT friend auto aixml::openDocument(const std::filesystem::path& fileName) -> std::shared_ptr<node>;
  AIXMLDLLEXPORT friend bool aixml::saveDocument(const node& root, int toolLevel);
  AIXMLDLLEXPORT friend void aixml::closeDocument(const node& root);
  AIXMLDLLEXPORT friend bool aixml::checkSchema(const node& schemaXML, const node& xmlToBeTested);
  AIXMLDLLEXPORT friend void aixml::applySchema(const node& schemaXML, node* xmlToBeUpdated);
  node getSingleNode(std::string filename, std::string s, int depth);

  std::string getName() const;
  /** \brief Function returns a reference to node matching the subPath, if no node is found throws error
   * \param s
   * \param depth
   */
  node& at(const std::string& s, int depth = std::numeric_limits<int>::max()) const;

  /** \brief Function returns a pointer to node matching the subPath, if no node is found returns 0=false
   *  \details The function splits the given string s into subpaths and searchs if the
   *   current element is existing somewhere in the subtree. If the element is found somewhere
   *   in the tree, the subtree of this element is taken to search for the next element.
   *   If you set the depth variable to 1, you are able to search explicitly for a given unique path s
   * \param s
   * \param depth
   */
  node* find(const std::string& s, int depth = std::numeric_limits<int>::max()) const;

  /** \brief Function returns a vector with nodes matching the subPath
   * \param subPath
   * \param depth
   */
  std::vector<node*> getVector(std::string subPath, int depth = 1) const;

  /** \brief Function returns a vector with doubles of elements matching the subPath
   * \param subPath
   * \param depth
   */
  std::vector<double> getDoubleVector(const std::string& subPath, int depth = 1) const;

  /** \brief Function returns a vector with integers of elements matching the subPath
   * \param subPath
   * \param depth
   */
  std::vector<int> getIntVector(const std::string& subPath, int depth = 1) const;

  /** \brief Function returns a vector with booleans of elements matching the subPath
   * \param subPath
   * \param depth
   */
  std::vector<bool> getBoolVector(const std::string& subPath, int depth = 1) const;

  /** \brief Function returns a vector with strings of elements matching the subPath
   * \param subPath
   * \param depth
   */
  std::vector<std::string> getStringVector(const std::string& subPath, int depth = 1) const;

  /** \brief Function returns the first node with an attribute uID with a value matching the given string
   * \param uid
   */
  node* findNodeByUid(std::string uid);

  /** \brief Function returns a map with every existing uid as keys and the corresponding vector of nodes as values
   */
  std::unordered_map<std::string, std::vector<node*>> getAllUidsWithNodes();

  /** \brief Runction returns an attribute with the given name as boolean
   * \param name
   */
  bool getBoolAttrib(std::string name) const;

  /** \brief returns an attribute with the given name as integer
   * \param name
   */
  int getIntAttrib(std::string name) const;

  /** \brief returns an attribute with the given name as double
   * \param name
   */
  double getDoubleAttrib(std::string name) const;

  /** \brief returns an attribute with the given name as string
   * \param name
   */
  std::string getStringAttrib(std::string name) const;

  /** \brief returns an attribute with the given name as string
   * \param name
   */
  bool hasAttrib(std::string name) const;

  /** \brief Add an attribute to the node, keeps the old value if attribute already exists
   * \param name
   * \param T value
   */
  template <class T>
  void addAttrib(std::string name, T value);

  /** \brief Sets an attribute, depending on addIfNotExistent a new attribute can be added
   * \param name
   * \param T value
   * \param addIfNotExistent
   */
  template <class T>
  void setAttrib(std::string name, T value, bool addIfNotExistent = true);

  /** \brief Deletes an attribute from the node
   * \param name
   */
  void deleteAttrib(std::string name);

  /** \brief returns a map of attributes from the node
   */
  std::map<std::string, std::string> getAttribMap() const;

  /** \brief Constructs an new node and appends to this node as child, if writeToXML is true, new node appears in
   * XML-File \param name \param writeToXML
   */
  node& appendChild(const std::string& name, bool writeToXML = false);

  /** \brief Appends an existing node to this node as child
   * \param new_mode
   */
  node& appendChild(node* new_node);

  /** \brief Copies and appends an existing node to this node as child
   * \param nodeToBeInserted
   */
  node& appendChild(const node& nodeToBeInserted);

  /** \brief Appends a vector of string names as children to a current node. If writeToXML is true, new node appears in
   * XML-File \param names \param writeToXML
   */
  void appendChildren(const std::vector<std::string>& names, bool writeToXml);

  /** \brief Appends a vector of existing nodes to this node as children
   * \param new_nodes
   */
  void appendChildren(std::vector<node*> new_nodes);

  /** \brief Copies and appends a vector of existing nodes to this node as children
   * \param nodesToBeInserted
   */
  void appendChildren(const std::vector<node>& nodesToBeInserted);

  /** \brief Deletes the child and the full subtree
   * \param name
   */
  void deleteChild(const std::string& name);

  /** \brief Recursive function get called for deleting the subtree from the child on
   * \param old_node
   */
  void deleteChild(node* old_node);

  /** \brief Deletes all children including their subtrees
   */
  void deleteChildren();

  /** \brief duplicates this node and by default appends to parent-element after this node, returns reference to new
   * node \param new_parent
   */
  node& duplicate(node* new_parent = nullptr);

  /** \brief duplicates this node, fills new node with zeros and by default appends to parent-element after this node,
   * returns reference to new node \param new_parent
   */
  node& duplicateEmpty(node* new_parent = nullptr);

  /** \brief creates copy of nodeToBeInserted and replaces existing node with name=name (including identifiers) old node
   * gets deleted \param name \param nodeToBeInserted \param createIfNotExistent
   */
  node& replaceChild(std::string name, const node& nodeToBeInserted, bool createIfNotExistent = false);

  /** \brief creates copy of nodeToBeInserted and inserts after node with name=name (including identifiers)
   * \param name
   * \param noteToBeInserted
   */
  node& insertAfterChild(std::string name, const node& nodeToBeInserted);

  /** \brief creates copy of nodeToBeInserted and inserts before node with name=name (including identifiers)
   * \param name
   * \param nodeToBeInserted
   */
  node& insertBeforeChild(std::string name, const node& nodeToBeInserted);

  /** \brief sets everything to zero, if setAttributesToZero is set, also attributes get set to zero
   * \param setAttributesToZero
   */
  void setToZero(bool setAttributesToZero = false);

  /* Conversion operators */
  operator double();
  operator std::string();
  node& operator[](std::string s);

  /* Overloaded operators */
  AIXMLDLLEXPORT friend std::ostream& operator<<(std::ostream& stream, node& n);

  void operator=(const double& d);
  void operator=(const int& i);
  void operator=(const bool& b);
  void operator=(const std::string& s);
  void operator=(const char* s);

  AIXMLDLLEXPORT friend std::string operator+(const std::string& value, node n);
  AIXMLDLLEXPORT friend std::string operator+(node n, const std::string& value);
  AIXMLDLLEXPORT friend double operator+(double value, node n);
  AIXMLDLLEXPORT friend double operator+(const node& n, double value);
  AIXMLDLLEXPORT friend double operator-(double value, node n);
  AIXMLDLLEXPORT friend double operator-(const node& n, double value);
  AIXMLDLLEXPORT friend double operator/(double value, node n);
  AIXMLDLLEXPORT friend double operator/(node n, double value);
  AIXMLDLLEXPORT friend double operator*(double value, node n);
  AIXMLDLLEXPORT friend double operator*(const node& n, double value);
  double operator*(const node& n);
  double operator/(const node& n);
  double operator+(const node& n);
  double operator-(const node& n);

  double arithmetic_operation(node n, char c);

  AIXMLDLLEXPORT friend double checkBoundaries(const node& value, const double& minValue, const double& maxValue);
  AIXMLDLLEXPORT friend double checkBoundaries(node value, const double& minValue, const bool& includeLeft,
                                               const double& maxValue, const bool& includeRight);
  AIXMLDLLEXPORT friend bool checkBoundaries(node value);
  AIXMLDLLEXPORT friend uint16_t checkBoundaries(node value, const uint16_t maxValue);

  std::string getFullPath(std::string aPath = "", bool includingIdentifier = true) const;
  std::string getRootName() const;

  static std::vector<std::string> input;
  static std::vector<std::string> output;

  std::string name;

  auto getChildren() const -> const std::vector<node*>& { return children; }
  auto getParent() const -> const node* { return parent; }

 private:
  std::map<std::string, std::string> attributes;
  node* parent;
  std::vector<node*> children;
  std::string undefinedValue;
  std::string stringValue;
  //    int intValue;
  double doubleValue;
  //    bool boolValue;
  valueType type;
  void addNodeToDom(TiXmlNode* tiNode);
  std::string getResult() const;
  static void addNodeToDoc(node* treeNode, TiXmlElement* docNode, int toolLevel);
  bool writeToXml;
  bool modified;
  void addToInputOutput(bool isInput = true);

  /** \brief Function initializes the attributes with given values
   * \param name
   * \param writeToXml
   * \param value
   * \param parent
   */
  void init(const std::string& name = "", bool writeToXml = true, const std::string& value = "",
            node* parent = nullptr);
  std::vector<node*> findVector(std::string subPath, bool findAll, int depth) const;
  void getAllUidsWithNodes(std::unordered_map<std::string, std::vector<node*>>* nodesAndUids);
};

/** \brief Checks whether a given string value is a boolean type
 * \details Currently a valid boolean would be 0, 1, true, false, True, False, TRUE, FALSE, on, On, ON, off, Off, OFF
 * \param aValue: A value of type string
 * \return a boolean value which indicates whether the string value is a boolean or not
 */
AIXMLDLLEXPORT bool isBoolean(const std::string& aValue);

/** \brief Checks whether a given string value is can be classified as true
 * \details Currently a true boolean would be 1, true, True, TRUE, on, On, ON
 * \param aValue: A value of type string
 * \return a boolean value which indicates whether the string value is true
 */
AIXMLDLLEXPORT bool isTrue(const std::string& aValue);

/** \brief Checks whether a given string value is can be classified as false
 * \details Currently a true boolean would be 0, false, False, FALSE, off, Off, OFF
 * \param aValue: A value of type string
 * \return a boolean value which indicates whether the string value is a boolean or not
 */
AIXMLDLLEXPORT bool isFalse(const std::string& aValue);

/** \brief Add an attribute to the node
 */
template <class T>
void node::addAttrib(std::string name, T value) {
  std::stringstream ss;
  ss << value;
  attributes.emplace(name, ss.str());
}

/** \brief Add an attribute to the node
 */
template <class T>
void node::setAttrib(std::string name, T value, bool addIfNotAxistent) {
  std::stringstream ss;
  ss << value;
  if (addIfNotAxistent) {
    attributes[name] = ss.str();
  } else if (attributes.count(name)) {
    attributes[name] = ss.str();
  }
}
#endif  // AIXML_INCLUDE_AIXML_NODE_H_
