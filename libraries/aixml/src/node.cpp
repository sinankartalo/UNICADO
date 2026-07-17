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

#include "aixml/node.h"

#include <unitConversion/constants.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <utility>

#include "tinyxml.h"

std::vector<std::string> node::input;
std::vector<std::string> node::output;

void split(const std::string& s, char delim, std::vector<std::string>* elems) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems->push_back(item);
  }
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, &elems);
  return elems;
}

template <class T>
std::string num_to_string(const T n) {
  std::stringstream ss;
  ss << n;
  return ss.str();
}

node::node() { init(); }

node::node(const std::string& name, bool writeToXml) { init(name, writeToXml); }

node::node(const std::string& name, const std::string& value, bool writeToXml) { init(name, writeToXml, value); }

node::node(const std::string& name, node* parent, bool writeToXml) { init(name, writeToXml, "", parent); }

node::node(const std::string& name, const std::string& value, node* parent, bool writeToXml) {
  init(name, writeToXml, value, parent);
}

node::node(const node& nodeToCopy)
    : name(nodeToCopy.name),
      attributes(nodeToCopy.attributes),
      parent(nodeToCopy.parent),
      undefinedValue(nodeToCopy.undefinedValue),
      stringValue(nodeToCopy.stringValue),
      doubleValue(nodeToCopy.doubleValue),
      type(nodeToCopy.type),
      writeToXml(nodeToCopy.writeToXml),
      modified(nodeToCopy.modified) {
  for (node* child : nodeToCopy.children) {
    child->duplicate(this);
  }
}

void node::init(const std::string& name, bool writeToXml, const std::string& value, node* parent) {
  this->name = name;
  this->parent = parent;
  undefinedValue = value;
  stringValue = "";
  doubleValue = 0.;
  type = valueType::UNDEFINED;
  this->writeToXml = writeToXml;
  modified = 0;
}

std::vector<node*> node::findVector(std::string subPath, bool findAll, int depth) const {
  std::string identifier_name = "ID";
  std::string identifier = "";
  std::vector<std::string> splittet_at_slash = split(subPath, '/');
  if (splittet_at_slash.size() > 1) {
    if (splittet_at_slash.at(0) == "") {
      splittet_at_slash.erase(splittet_at_slash.begin());
    }
    std::vector<node*> currentNodes = this->findVector(splittet_at_slash.at(0), findAll, depth);
    std::vector<node*> newCurrentNodes;  // Only used by calling with findAll=true
    for (unsigned int i = 1; i < splittet_at_slash.size(); i++) {
      if (findAll) {
        newCurrentNodes.clear();
        for (node* n : currentNodes) {
          std::vector<node*> nodesFromOneChild = n->findVector(splittet_at_slash.at(i), true, depth);
          newCurrentNodes.insert(newCurrentNodes.end(), nodesFromOneChild.begin(), nodesFromOneChild.end());
        }
        currentNodes = newCurrentNodes;
      } else if (!currentNodes.empty()) {
        currentNodes = currentNodes.back()->findVector(splittet_at_slash.at(i), false, depth);
      }
    }
    return currentNodes;
  }
  std::vector<std::string> splittet_at_id = split(subPath, '@');
  if (splittet_at_id.size() > 1) {
    subPath = splittet_at_id.at(0);
    if (splittet_at_id.at(1).find('=') == std::string::npos) {
      identifier = splittet_at_id.at(1);
    } else if (split(splittet_at_id.at(1), '=').size() > 1) {
      identifier_name = split(splittet_at_id.at(1), '=').at(0);
      identifier = split(splittet_at_id.at(1), '=').at(1);
    } else {
      identifier_name = split(splittet_at_id.at(1), '=').at(0);
      identifier = "";
    }
  }
  std::queue<node*> myQueue;
  std::queue<int> myDepths;
  for (node* child : this->children) {
    myQueue.push(child);
    myDepths.push(1);
  }
  int currentDepth = 0;
  std::vector<node*> result;
  while (!myQueue.empty() && currentDepth <= depth) {
    node* currentNode(myQueue.front());
    myQueue.pop();
    currentDepth = myDepths.front();
    myDepths.pop();
    if (currentNode->name == subPath &&
        (splittet_at_id.size() == 1 || currentNode->attributes.at(identifier_name) == identifier)) {
      result.push_back(currentNode);
      if (!findAll) {
        return result;
      }
    } else {
      for (node* child : currentNode->children) {
        myQueue.push(child);
        myDepths.push(currentDepth + 1);
      }
    }
    if (!myDepths.empty()) {
      currentDepth = myDepths.front();
    }
  }
  return result;
}

node& node::at(const std::string& subPath, int depth) const {
  std::vector<node*> result = findVector(subPath, false, depth);
  if (result.empty()) {
    throw std::string("element " + subPath + " in node " + getFullPath() + " not found");
  } else {
    return *result.at(0);
  }
}

node* node::find(const std::string& subPath, int depth) const {
  std::vector<node*> result = findVector(subPath, false, depth);
  if (result.empty()) {
    return nullptr;
  } else {
    return result.at(0);
  }
}

std::vector<node*> node::getVector(const std::string subPath, int depth) const {
  std::vector<node*> result = findVector(subPath, true, depth);
  if (result.empty()) {
    throw std::string("element " + subPath + " in node " + getFullPath() + " not found");
  } else {
    return result;
  }
}

std::vector<double> node::getDoubleVector(const std::string& subPath, int depth) const {
  std::vector<node*> nodes = findVector(subPath, true, depth);
  std::vector<double> res;
  if (nodes.empty()) {
    throw std::string("element " + subPath + " in node " + getFullPath() + " not found");
  } else {
    res.resize(nodes.size());
    std::transform(nodes.begin(), nodes.end(), res.begin(), [](const node* aNode) { return *aNode; });
  }
  return res;
}

std::vector<int> node::getIntVector(const std::string& subPath, int depth) const {
  std::vector<node*> nodes = findVector(subPath, true, depth);
  std::vector<int> res;
  if (nodes.empty()) {
    throw std::string("element " + subPath + " in node " + getFullPath() + " not found");
  } else {
    res.resize(nodes.size());
    std::transform(nodes.begin(), nodes.end(), res.begin(), [](const node* aNode) { return *aNode; });
  }
  return res;
}

std::vector<bool> node::getBoolVector(const std::string& subPath, int depth) const {
  std::vector<node*> nodes = findVector(subPath, true, depth);
  std::vector<bool> res;
  if (nodes.empty()) {
    throw std::string("element " + subPath + " in node " + getFullPath() + " not found");
  } else {
    res.resize(nodes.size());
    std::transform(nodes.begin(), nodes.end(), res.begin(), [](const node* aNode) { return *aNode; });
  }
  return res;
}

std::vector<std::string> node::getStringVector(const std::string& subPath,
                                               int depth) const {  // cppcheck-suppress unusedFunction
  std::vector<node*> nodes = findVector(subPath, true, depth);
  std::vector<std::string> res;
  if (nodes.empty()) {
    throw std::string("element " + subPath + " in node " + getFullPath() + " not found");
  } else {
    res.resize(nodes.size());
    std::transform(nodes.begin(), nodes.end(), res.begin(), [](node* n) { return std::string(*n); });
  }
  return res;
}

std::unordered_map<std::string, std::vector<node*>> node::getAllUidsWithNodes() {
  std::unordered_map<std::string, std::vector<node*>> nodes;
  getAllUidsWithNodes(&nodes);
  return nodes;
}

void node::getAllUidsWithNodes(std::unordered_map<std::string, std::vector<node*>>* nodesAndUids) {
  if (this->attributes.find("uID") != this->attributes.end()) {
    std::string uid = this->attributes.at("uID");
    if (nodesAndUids->find(uid) != nodesAndUids->end()) {
      nodesAndUids->at(uid).push_back(this);
    } else {
      nodesAndUids->insert(std::make_pair(uid, std::vector<node*>(1, this)));
    }
  }
  for (std::vector<node*>::iterator child = this->children.begin(); child != this->children.end(); ++child) {
    (*child)->getAllUidsWithNodes(nodesAndUids);
  }
}

node* node::findNodeByUid(std::string uid) {
  // check, if attribute <uid> exists
  if (this->attributes.find("uID") != this->attributes.end()) {
    if (uid == this->attributes.at("uID")) return this;
  }
  if (this->children.empty()) {
    return nullptr;
  }
  for (std::vector<node*>::iterator child = this->children.begin(); child != this->children.end(); ++child) {
    node* res = (*child)->findNodeByUid(uid);
    if (res != nullptr) return res;
  }
  return nullptr;
}

node::~node() {
  for (unsigned int i = 0; i < children.size(); i++) {
    delete children[i];
  }
  // dtor
}

node& aixml::openDocument(std::string fileName) {
  TiXmlDocument pdoc(fileName.c_str());
  const bool loadOkay = pdoc.LoadFile();
  if (loadOkay) {
    pdoc.SetCondenseWhiteSpace(false);
  } else {
    throw std::string{"Could not open " + fileName + ". The file is either not existing or possibly damaged."};
  }
  node* root = new node();
  root->addNodeToDom(&pdoc);
  return *root;
}

auto aixml::openDocument(const std::filesystem::path& fileName) -> std::shared_ptr<node> {
  TiXmlDocument pdoc(fileName.string().c_str());
  const bool loadOkay = pdoc.LoadFile();
  if (loadOkay) {
    pdoc.SetCondenseWhiteSpace(false);
  } else {
    throw std::string{"Could not open " + fileName.string() + ". The file is either not existing or possibly damaged."};
  }
  auto root = std::make_shared<node>();
  root->addNodeToDom(&pdoc);
  return root;
}

void node::addNodeToDom(TiXmlNode* tiNode) {
  this->name = tiNode->Value();
  if (tiNode->Type() == TiXmlNode::TINYXML_ELEMENT) {
    for (TiXmlAttribute* attribute = tiNode->ToElement()->FirstAttribute(); attribute; attribute = attribute->Next()) {
      this->attributes.emplace(attribute->Name(), attribute->Value());
    }
  }
  for (TiXmlNode* child = tiNode->FirstChild(); child; child = child->NextSibling()) {
    if (child->Type() == TiXmlNode::TINYXML_ELEMENT) {
      node* childNode = new node();
      childNode->parent = this;
      this->children.push_back(childNode);
      childNode->addNodeToDom(child);
    } else if (child->Type() == TiXmlNode::TINYXML_TEXT) {
      this->undefinedValue = child->Value();
    }
  }
}
bool aixml::saveDocument(const node& root, int toolLevel) {
  TiXmlDocument* pdoc = new TiXmlDocument(root.name.c_str());
  for (node* n : root.children) {
    TiXmlElement* element = new TiXmlElement("");
    pdoc->InsertEndChild(*element);
    node::addNodeToDoc(n, pdoc->LastChild()->ToElement(), toolLevel);
  }
  bool succeeded = pdoc->SaveFile();
  delete pdoc;
  return succeeded;
}
void node::addNodeToDoc(node* treeNode, TiXmlElement* docNode, int toolLevel) {
  docNode->SetValue(treeNode->name.c_str());
  for (std::pair<std::string, std::string> attrib : treeNode->attributes) {
    docNode->SetAttribute(attrib.first.c_str(), attrib.second.c_str());
  }
  for (node* child : treeNode->children) {
    if (child->writeToXml) {
      TiXmlElement* element = new TiXmlElement("");
      docNode->InsertEndChild(*element);
      addNodeToDoc(child, docNode->LastChild()->ToElement(), toolLevel);
    }
  }
  if (treeNode->children.size() == 0) {
    if ((!treeNode->modified) || !treeNode->hasAttrib("ToolLevel")) {
      TiXmlText* text = new TiXmlText(treeNode->getResult().c_str());
      docNode->InsertEndChild(*text);
    } else if (treeNode->getIntAttrib("ToolLevel") <= toolLevel) {
      docNode->SetAttribute("ToolLevel", /*int*/ num_to_string(toolLevel).c_str());
      TiXmlText* text = new TiXmlText(treeNode->getResult().c_str());
      docNode->InsertEndChild(*text);
    } else {
      throw std::string("tool level to low (value) " + treeNode->getFullPath());
    }
  }
}
void aixml::closeDocument(const node& root) { delete &root; }

node& node::appendChild(const std::string& name, bool writeToXml) {
  node* new_node = new node();
  new_node->name = name;
  new_node->writeToXml = writeToXml;
  new_node->parent = this;
  this->children.push_back(new_node);
  return *new_node;
}

node& node::appendChild(node* new_node) {
  new_node->parent = this;
  this->children.push_back(new_node);
  return *new_node;
}
node& node::appendChild(const node& nodeToBeInserted) {
  node* new_node = new node(nodeToBeInserted);
  new_node->parent = this;
  this->children.push_back(new_node);
  return *new_node;
}

void node::appendChildren(const std::vector<std::string>& names, bool writeToXml) {
  for (std::string name : names) {
    node* new_node = new node();
    new_node->name = name;
    new_node->writeToXml = writeToXml;
    new_node->parent = this;
    children.push_back(new_node);
  }
}

void node::appendChildren(std::vector<node*> new_nodes) {
  for (node* new_node : new_nodes) {
    new_node->parent = this;
    children.push_back(new_node);
  }
}

void node::appendChildren(const std::vector<node>& nodesToBeInserted) {
  for (node nodeToBeInserted : nodesToBeInserted) {
    node* new_node = new node(nodeToBeInserted);
    new_node->parent = this;
    children.push_back(new_node);
  }
}

void node::deleteChild(const std::string& name) { this->deleteChild(find(name, 1)); }

void node::deleteChild(node* old_node) {
  if (old_node) {
    children.erase(std::remove(children.begin(), children.end(), old_node), children.end());
    delete old_node;
  }
}
void node::deleteChildren() {  // cppcheck-suppress unusedFunction
  for (node* n : children) {
    delete n;
  }
  children.clear();
}

node& node::duplicate(node* new_parent) {
  node* new_node = new node();
  if (new_parent) {
    new_node->parent = new_parent;
    new_parent->children.push_back(new_node);
  } else {
    new_node->parent = parent;
    parent->children.insert(std::find(parent->children.begin(), parent->children.end(), this) + 1, new_node);
  }
  new_node->name = name;
  new_node->undefinedValue = undefinedValue;
  new_node->stringValue = stringValue;
  new_node->doubleValue = doubleValue;
  new_node->type = type;
  new_node->writeToXml = writeToXml;
  new_node->modified = modified;
  std::map<std::string, std::string> new_attributes(attributes);
  new_node->attributes = new_attributes;
  for (node* child : children) {
    child->duplicate(new_node);
  }
  return *new_node;
}
node& node::duplicateEmpty(node* new_parent) {
  node* new_node = new node();
  if (new_parent) {
    new_node->parent = new_parent;
    new_parent->children.push_back(new_node);
  } else {
    new_node->parent = parent;
    parent->children.insert(std::find(parent->children.begin(), parent->children.end(), this) + 1, new_node);
  }
  new_node->name = name;
  new_node->undefinedValue = "0";
  new_node->type = valueType::UNDEFINED;
  new_node->writeToXml = writeToXml;
  new_node->modified = false;
  std::map<std::string, std::string> new_attributes(attributes);
  new_node->attributes = new_attributes;
  for (node* child : children) {
    child->duplicateEmpty(new_node);
  }
  return *new_node;
}

node& node::replaceChild(std::string name, const node& nodeToBeInserted,
                         bool createIfNotExistent) {  // cppcheck-suppress unusedFunction
  node* old_node = this->find(name);
  node* new_node = nullptr;
  if (old_node) {
    new_node = new node(nodeToBeInserted);
    new_node->parent = old_node->parent;
    std::replace(old_node->parent->children.begin(), old_node->parent->children.end(), old_node, new_node);
    delete old_node;
  } else if (createIfNotExistent) {
    new_node = new node(nodeToBeInserted);
    this->appendChild(new_node);
  } else {
    throw std::string("node " + getFullPath() + "/" + name + " not existing");
  }
  return *new_node;
}
node& node::insertAfterChild(std::string name, const node& nodeToBeInserted) {  // cppcheck-suppress unusedFunction
  node* old_node = this->find(name);
  node* new_node = nullptr;
  if (old_node) {
    new_node = new node(nodeToBeInserted);
    new_node->parent = old_node->parent;
    old_node->parent->children.insert(
        std::find(old_node->parent->children.begin(), old_node->parent->children.end(), old_node) + 1, new_node);
  } else {
    throw std::string("node " + getFullPath() + "/" + name + " not existing");
  }
  return *new_node;
}

node& node::insertBeforeChild(std::string name, const node& nodeToBeInserted) {  // cppcheck-suppress unusedFunction
  node* old_node = this->find(name);
  node* new_node = nullptr;
  if (old_node) {
    new_node = new node(nodeToBeInserted);
    old_node->parent->children.insert(
        std::find(old_node->parent->children.begin(), old_node->parent->children.end(), old_node), new_node);
  } else {
    throw std::string("node " + getFullPath() + "/" + name + " not existing");
  }
  return *new_node;
}

bool node::getBoolAttrib(std::string name) const {  // cppcheck-suppress unusedFunction
  if (!this->hasAttrib(name)) {
    throw std::string("attribute " + name + " in node " + getFullPath() + " not found");
  }
  if (!isBoolean(attributes.at(name))) {
    throw std::string("attribute " + name + " in node " + getFullPath() + "is not a valid boolean");
  }
  if (isTrue(attributes.at(name).c_str())) return true;
  if (isFalse(attributes.at(name).c_str())) return false;

  /*previous behaviour only to have a return statement in the end of the function. Technically function won't reach
   * statement */
  return atoi(attributes.at(name).c_str());
}

int node::getIntAttrib(std::string name) const {
  if (!this->hasAttrib(name)) {
    throw std::string("attribute " + name + " in node " + getFullPath() + " not found");
  }
  return atoi(attributes.at(name).c_str());
}

double node::getDoubleAttrib(std::string name) const {  // cppcheck-suppress unusedFunction
  if (!this->hasAttrib(name)) {
    throw std::string("attribute " + name + " in node " + getFullPath() + " not found");
  }
  return atof(attributes.at(name).c_str());
}

std::string node::getStringAttrib(std::string name) const {  // cppcheck-suppress unusedFunction
  if (!this->hasAttrib(name)) {
    throw std::string("attribute " + name + " in node " + getFullPath() + " not found");
  }
  return attributes.at(name);
}
bool node::hasAttrib(std::string name) const { return attributes.count(name); }

void node::deleteAttrib(std::string name) {  // cppcheck-suppress unusedFunction
  attributes.erase(name);
}

std::map<std::string, std::string> node::getAttribMap() const {  // cppcheck-suppress unusedFunction
  return attributes;
}

double checkBoundaries(const node& value, const double& minValue, const double& maxValue) {
  return checkBoundaries(value, minValue, false, maxValue, false);
}

double checkBoundaries(node value, const double& minValue, const bool& includeLeft, const double& maxValue,
                       const bool& includeRight) {
  double res = value;
  if (res > minValue && res < maxValue) {
    return value;
  } else if (includeLeft && (fabs(res - minValue) < ACCURACY_HIGH)) {
    return value;
  } else if (includeRight && (fabs(res - maxValue) < ACCURACY_HIGH)) {
    return value;
  }
  std::string leftBoundary = "(";
  std::string rightBoundary = ")";
  if (includeLeft) {
    leftBoundary = "[";
  }
  if (includeRight) {
    rightBoundary = "]";
  }
  throw std::string("parameter " + value.getFullPath() + "=" + num_to_string(res) + " not in range " + leftBoundary +
                    /*double*/ num_to_string(minValue) + "," + /*double*/ num_to_string(maxValue) + rightBoundary);
}

bool checkBoundaries(node value) {
  std::string res = value.undefinedValue;
  if (!isBoolean(res)) {
    throw std::string("Parameter " + value.getFullPath() + " = " + res + " has not a valid boolean value.");
  } else if (res == "0" || res == "false" || res == "False" || res == "FALSE") {
    return false;
  } else {
    return true;
  }
}

uint16_t checkBoundaries(node value, const uint16_t maxValue) {
  // Only using the number of modes parameter. If using two values function overload would be ambiguous
  std::string res = value.undefinedValue;
  if (!(static_cast<uint16_t>(value) == value)) {
    throw std::string("Parameter " + value.getFullPath() + " = " + res + " has not a valid unsigned integer value.");
  }
  // No check for <0 because uint16_t will never be <0
  if (static_cast<uint16_t>(value) > maxValue) {
    throw std::string("Parameter " + value.getFullPath() + " = " + res + " not in range [0," + num_to_string(maxValue) +
                      "]");
  }
  return static_cast<uint16_t>(value);
}

// overloaded cast operators (operator-magic)
node::operator double() {
  if (children.size() != 0) {
    throw "element " + getFullPath() + " has no value";
  } else if (type == valueType::UNDEFINED) {
    /*
    //Slow but good code
    addToInputOutput();
    //If the Node type is undefined check if you can convert the string to a number. Only if that is so
    // return the converted number and set the node type as double.
    std::regex number_regex("^\\s*\\-?\\d*\\.*\\d*\\s*$");
    if (std::regex_match(undefinedValue, number_regex))
    {
        type=valueType::DOUBLE;
        doubleValue=atof(undefinedValue.c_str());
        return doubleValue;
    }
    //Since the value can not be converted to a Number return nan.
    else
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    */
    // Sloppy but fast code
    addToInputOutput();
    type = valueType::DOUBLE;
    doubleValue = atof(undefinedValue.c_str());
    return doubleValue;
  } else if (type == valueType::STRING) {
    addToInputOutput();
    type = valueType::DOUBLE;
    doubleValue = atof(stringValue.c_str());
    return doubleValue;
  } else if (type == valueType::DOUBLE) {
    return doubleValue;
  } else {
    throw "failed to convert " + getFullPath() + " from string to numeric/bool type";
  }
}

node::operator std::string() {
  if (children.size() != 0) {
    throw "element has no value";
  } else if (type == valueType::UNDEFINED) {
    addToInputOutput();
    type = valueType::STRING;
    stringValue = undefinedValue;
    return stringValue;
  } else if (type == valueType::STRING) {
    return stringValue;
  } else if (type ==
             valueType::DOUBLE) {  // In case the value was already converted to a double return the original string.
    return undefinedValue;
  } else {
    throw "failed to convert numeric/bool type to string";
  }
}

std::ostream& operator<<(std::ostream& stream, node& n) {
  if (n.type == node::valueType::UNDEFINED) {
    n.addToInputOutput();
    stream << n.undefinedValue;
    return stream;
  } else if (n.type == node::valueType::STRING) {
    stream << n.stringValue;
    return stream;
  } else {
    stream << n.doubleValue;
    return stream;
  }
}

std::string node::getResult() const {
  std::stringstream stream;
  if (type == valueType::UNDEFINED) {
    stream << undefinedValue;
    return stream.str();
  } else if (type == valueType::STRING) {
    stream << stringValue;
    return stream.str();
  } else {
    stream.precision(AIXMLPRECISION);
    stream << doubleValue;
    return stream.str();
  }
}

void node::operator=(const double& d) {
  modified = true;
  addToInputOutput(false);
  if (type == valueType::DOUBLE) {
    doubleValue = d;
  } else if (type == valueType::UNDEFINED) {
    type = valueType::DOUBLE;
    doubleValue = d;
  } else {
    stringValue = /*double*/ num_to_string(d);
  }
}

void node::operator=(const int& i) {
  modified = true;
  addToInputOutput(false);
  if (type == valueType::UNDEFINED) {
    type = valueType::DOUBLE;
    doubleValue = i;
  } else if (type == valueType::DOUBLE) {
    doubleValue = i;
  } else {
    stringValue = /*int*/ num_to_string(i);
  }
}

void node::operator=(const bool& b) {
  modified = true;
  addToInputOutput(false);
  if (type == valueType::UNDEFINED) {
    type = valueType::DOUBLE;
    doubleValue = static_cast<double>(b);
  } else if (type == valueType::DOUBLE) {
    doubleValue = static_cast<double>(b);
  } else {
    if (b) {
      stringValue = "1";
    } else {
      stringValue = "0";
    }
  }
}

void node::operator=(const std::string& s) {
  modified = true;
  addToInputOutput(false);
  if (type == valueType::UNDEFINED) {
    type = valueType::STRING;
    stringValue = s;
  } else if (type == valueType::DOUBLE) {
    doubleValue = atof(s.c_str());
  } else {
    stringValue = s;
  }
}

void node::operator=(const char* s) {
  modified = true;
  addToInputOutput(false);
  if (type == valueType::UNDEFINED) {
    type = valueType::STRING;
    stringValue = s;
  } else if (type == valueType::DOUBLE) {
    doubleValue = atof(s);
  } else {
    stringValue = s;
  }
}

std::string operator+(const std::string& value, node n) {
  if (n.type == node::valueType::UNDEFINED) {
    return value + std::string(n);
  } else if (n.type == node::valueType::STRING) {
    return value + n.stringValue;
  } else {
    return value + /*double*/ num_to_string(n.doubleValue);
  }
}

std::string operator+(node n, const std::string& value) {
  if (n.type == node::valueType::UNDEFINED) {
    return std::string(n) + value;
  } else if (n.type == node::valueType::STRING) {
    return n.stringValue + value;
  } else {
    return /*double*/ num_to_string(n.doubleValue) + value;
  }
}

// overloaded arithmetical operators

double operator+(double value, node n) {
  if (n.type == node::valueType::UNDEFINED || n.type == node::valueType::DOUBLE) {
    return value + static_cast<double>(n);
  }
  throw std::string("failed to convert " + n.getFullPath() + "from string to numeric");
}
double operator+(const node& n, double value) { return value + n; }
double operator-(double value, node n) {
  if (n.type == node::valueType::UNDEFINED || n.type == node::valueType::DOUBLE) {
    return value - static_cast<double>(n);
  }
  throw std::string("failed to convert " + n.getFullPath() + "from string to numeric");
}
double operator-(const node& n, double value) { return (-1) * (value - n); }
double operator/(double value, node n) {
  if (n.type == node::valueType::UNDEFINED || n.type == node::valueType::DOUBLE) {
    return value / static_cast<double>(n);
  }
  throw std::string("failed to convert " + n.getFullPath() + "from string to numeric");
}
double operator/(node n, double value) {
  if (n.type == node::valueType::UNDEFINED || n.type == node::valueType::DOUBLE) {
    return static_cast<double>(n) / value;
  }
  throw std::string("failed to convert " + n.getFullPath() + "from string to numeric");
}
double operator*(double value, node n) {
  if (n.type == node::valueType::UNDEFINED || n.type == node::valueType::DOUBLE) {
    return value * static_cast<double>(n);
  }
  throw std::string("failed to convert " + n.getFullPath() + "from string to numeric");
}
double operator*(const node& n, double value) { return value * n; }

double node::operator*(const node& n) { return arithmetic_operation(n, '*'); }
double node::operator/(const node& n) { return arithmetic_operation(n, '/'); }
double node::operator-(const node& n) { return arithmetic_operation(n, '-'); }
double node::operator+(const node& n) { return arithmetic_operation(n, '+'); }

double node::arithmetic_operation(node n, char c) {
  double a;
  double b;
  if (this->type == valueType::UNDEFINED || this->type == valueType::DOUBLE) {
    a = *this;
  } else {
    throw std::string("failed to convert " + getFullPath() + "from string to numeric");
  }
  if (n.type == valueType::UNDEFINED || n.type == valueType::DOUBLE) {
    b = n;
  } else {
    throw std::string("failed to convert " + n.getFullPath() + "from string to numeric");
  }
  switch (c) {
    case '+':
      return a + b;
    case '*':
      return a * b;
    case '/':
      return a / b;
    case '-':
      return a - b;
  }
  return NAN;
}
std::string node::getFullPath(std::string aPath, bool includingIdentifier) const {
  std::string newPath = this->name;
  if (includingIdentifier) {
    if (this->hasAttrib("ID") && this->hasAttrib("uID")) {
      std::cout << "WARNING: in function \"node::" << __FUNCTION__ << "\" XML-element \"" << this->name
                << "\" has ambiguous identifiers. Both, \"ID\" and \"uID\" are present. \"ID\" will be used."
                << std::endl;
      newPath += "@ID=" + this->attributes.at("ID");
    } else {
      if (this->hasAttrib("ID")) {
        newPath += "@ID=" + this->attributes.at("ID");
      } else if (this->hasAttrib("uID")) {
        newPath += "@uID=" + this->attributes.at("uID");
      }
    }
  }
  if (aPath != "") newPath += "/";
  aPath.insert(0, newPath);
  node* theParent = this->parent;
  if (theParent) {
    return theParent->getFullPath(aPath, includingIdentifier);
  }
  return aPath;
}

/**
 * \brief Returns name of root node
 * \return std::string name of root node
 */
std::string node::getRootName() const {
  node* root = this->parent;
  if (root == nullptr) {
    return this->name;
  }
  do {
    root = root->parent;
  } while (root != nullptr);
  return root->name;
}

void node::addToInputOutput(bool isInput) {
  /*std::string thePath=name;
  if (hasAttrib("ID"))
  {
      thePath+="@"+attributes["ID"];
  }
  node* theParent=parent;
  while (theParent)
  {
      std::string theNewPath=theParent->name;
      if (theParent->hasAttrib("ID"))
      {
          theNewPath+="@"+theParent->attributes["ID"];
      }
      thePath=theNewPath+"/"+thePath;
      theParent=theParent->parent;
  }*/
  if (isInput) {
    input.push_back(getFullPath());
  } else {
    output.push_back(getFullPath());
  }
}

bool aixml::checkSchema(const node& schemaXML, const node& xmlToBeTested) {
  for (node* n : xmlToBeTested.children) {
    node* equivalent = schemaXML.find(n->name, 1);
    if (!equivalent) {
      return false;
    } else {
      if (!checkSchema(*equivalent, *n)) {
        return false;
      }
    }
  }
  return true;
}

void aixml::applySchema(const node& schemaXML, node* xmlToBeUpdated) {
  for (std::pair<std::string, std::string> attribute : schemaXML.attributes) {
    if (!xmlToBeUpdated->hasAttrib(attribute.first)) {
      std::cout << "atribute " + attribute.first + " from scheme-element:" << std::endl;
      std::cout << schemaXML.getFullPath() << std::endl;
      std::cout << "gets added to xml at:" << std::endl;
      std::cout << xmlToBeUpdated->getFullPath() << std::endl;
      xmlToBeUpdated->setAttrib(attribute.first, attribute.second);
    }
  }
  for (node* n : schemaXML.children) {
    std::vector<node*> equivalents = xmlToBeUpdated->findVector(n->name, true, 1);
    if (equivalents.empty()) {
      std::cout << "element from scheme:" << std::endl;
      std::cout << n->getFullPath() << std::endl;
      std::cout << "gets added as child to xml at:" << std::endl;
      std::cout << xmlToBeUpdated->getFullPath() << std::endl;
      n->duplicateEmpty(xmlToBeUpdated);
    } else {
      for (node* equivalent : equivalents) {
        applySchema(*n, equivalent);
      }
    }
  }
}
std::string node::getName() const {  // cppcheck-suppress unusedFunction
  return name;
}
void node::setToZero(bool setAttributesToZero) {
  if (setAttributesToZero) {
    for (std::pair<std::string, std::string> attribute : attributes) {
      setAttrib(attribute.first, "0");
    }
  }
  if (children.empty()) {
    undefinedValue = "0";
  } else {
    undefinedValue = "";
  }
  stringValue = "";
  doubleValue = 0;
  type = valueType::UNDEFINED;
  //    writeToXml=writeToXml;
  modified = true;
  for (node* child : children) {
    child->setToZero(setAttributesToZero);
  }
}

node aixml::getSingleNode(const std::string& filename, std::string s, int depth) {  // cppcheck-suppress unusedFunction
  const node& xml = openDocument(filename);
  node res = xml.at(s, depth);
  closeDocument(xml);
  return res;
}

/**
 * \brief Square bracket operator[] - generates paths if not existent
 * \param s std::string parameter - begin relative from current node e.g.
 *          if at root and want to access child at root use nodevariable["<child>/.../.../..."]
 * \return node& returns node at last level of output
 */
node& node::operator[](std::string s) {
  std::vector<std::string> nodenames = split(s, '/');
  if (nodenames.size() == 1 && s.compare(this->name) == 0) {
    return *this;
  }
  if (nodenames.at(0).compare(this->name) == 0) {
    nodenames.erase(nodenames.begin());
  }
  std::string currentpath = "";
  for (std::string nodename : nodenames) {
    if (!this->find(currentpath + nodename)) {
      std::vector<std::string> id = split(nodename, '@');
      if (this->children.size() == 0 || currentpath.compare("") == 0) {
        this->appendChild(id[0], true);
      } else {
        // remove slash since find can't operate on ending /
        std::string modifiedpath = currentpath;
        if (modifiedpath.back() == '/') {
          modifiedpath.pop_back();
        }
        this->find(modifiedpath)->appendChild(id[0], true);
      }
      if (id.size() > 1) {
        auto m_v = this->findVector(currentpath + id[0], true, 1);
        size_t idx = m_v.size();
        m_v.at(idx-1)->addAttrib("ID", id[1]);
      }
      this->find(currentpath + nodename)->type = valueType::UNDEFINED;
    }
    currentpath += nodename;
    if (nodename.compare(nodenames.back()) != 0) {
      currentpath += "/";
    }
  }
  return this->at(currentpath);
}

bool isBoolean(const std::string& aValue) {
  if (isTrue(aValue) || isFalse(aValue)) return true;
  return false;
}

bool isTrue(const std::string& aValue) {
  std::vector<std::string> trueBools = {"1", "true", "True", "TRUE", "on", "On", "ON"};
  bool boolIsTrue = false;
  for (uint8_t i(0); i < trueBools.size(); i++) {
    if (aValue.compare(trueBools.at(i)) == 0) {
      boolIsTrue = true;
      break;
    }
  }
  if (!boolIsTrue) {
    return false;
  } else {
    return true;
  }
}

bool isFalse(const std::string& aValue) {
  std::vector<std::string> falseBools = {"0", "false", "False", "FALSE", "off", "Off", "OFF"};
  bool boolIsFalse = false;
  for (uint8_t i(0); i < falseBools.size(); i++) {
    if (aValue.compare(falseBools.at(i)) == 0) {
      boolIsFalse = true;
      break;
    }
  }
  if (!boolIsFalse) {
    return false;
  } else {
    return true;
  }
}
