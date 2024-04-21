#include "CJsonWrite/CJsonWrite.h"
#ifndef __cplusplus
#include <stdbool.h>
#endif

JSONFuncs_t jsonFuncs;
void CJsonWriteInit(JSONFuncs_t* _jsonFuncs) {
    jsonFuncs.malloc = _jsonFuncs->malloc;
    jsonFuncs.free = _jsonFuncs->free;
    jsonFuncs.memset = _jsonFuncs->memset;
    jsonFuncs.strlen = _jsonFuncs->strlen;
    jsonFuncs.snprintf = _jsonFuncs->snprintf;
    jsonFuncs.strncpy = _jsonFuncs->strncpy;
}
#pragma region VALIDATOR_UTILS
bool JSONArrayIsValid(JSONArray_t* pArray) {
    if (pArray == NULL) return false;
    if ((pArray->pStart == NULL) != (pArray->pEnd == NULL)) return false;
    return true;
}
void JSONArrayMustBeValid(JSONArray_t* pArray) {
    JsonAssert(pArray != NULL);
    JsonAssertMsg((pArray->pStart == NULL) == (pArray->pEnd == NULL), "Array is corrupted ! Something somewhere has gone horribly wrong.");
}
bool JSONArrayIsEmpty(JSONArray_t* pArray) {
    JSONArrayMustBeValid(pArray);
    return pArray->pStart == NULL; // checking pEnd would work too because if only one is null then something terrible has happened
}
bool JSONArrayNodeIsEmpty(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    JsonAssert(pNode->type == JSONArrayType);
    return JSONArrayIsEmpty(pNode->value.pArray);
}
bool JSONObjIsValid(JSONObj_t* pObj) {
    if (pObj == NULL) return false;
    if ((pObj->pFirstChild == NULL) != (pObj->pLastChild == NULL)) return false;
    return true;
}
void JSONObjMustBeValid(JSONObj_t* pObj) {
    JsonAssert(pObj != NULL);
    JsonAssertMsg((pObj->pFirstChild == NULL) == (pObj->pLastChild == NULL), "JSONObj is corrupted ! Something somewhere has gone horribly wrong.");
}
bool JSONObjIsEmpty(JSONObj_t* pObj) {
    JSONObjMustBeValid(pObj);
    return pObj->pFirstChild == NULL; // checking pLastChild would work too because if only one is null then something terrible has happened
}
size_t JSONArrayGetNumElements(JSONArray_t* pArray) {
    size_t n = 0;
    JSONNode_t* current = pArray->pStart;
    while (current != NULL) {
        current = current->pNextSibling;
        n++;
    }
    return n;
}
bool JSONNodeCanHaveChildren(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    return pNode->type == JSONObjType || pNode->type == JSONArrayType;
}
#pragma endregion

#pragma region NODE_UTILS
/// @brief Connects its previous node to its next node, and its next node to its previous node. This is the "write to the JSONNode" step of removing a node from a JSONArray.
/// @param pNode The node in question
void JSONNodeConnectNeighbors(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    JSONNode_t* pPrev = pNode->pPrevSibling;
    JSONNode_t* pNext = pNode->pNextSibling;
    
    if (pPrev != NULL) {
        pPrev->pNextSibling = pNext;
    }
    if (pNext != NULL) {
        pNext->pPrevSibling = pPrev;
    }
}

/// @brief Connects two nodes together, linking the leftmost node's next sibling node.
/// @param pLeft The leftmost node to link with.
/// @param pNewNode A node with no prior connections ready to be inserted.
void JSONNodeInsertAfter(JSONNode_t* pLeft, JSONNode_t* pNewNode) {
    JsonAssert(pLeft != NULL);
    JsonAssert(pNewNode != NULL);
    JsonAssertMsg(pNewNode->pPrevSibling == NULL, "Tried to insert node into an array, but the new node already has a previous sibling ! Nodes should only have one reference at all times.");
    
    JSONNode_t* right = pLeft->pNextSibling;
    if (right != NULL) {
        pNewNode->pNextSibling = right;
        right->pPrevSibling = pNewNode;
    }

    pLeft->pNextSibling = pNewNode;
    pNewNode->pPrevSibling = pLeft;
}
#pragma endregion

/// @brief Destroys all elements of a JSONArray.
/// @param pArray The array
void JSONArrayDestroyElements(JSONArray_t* pArray) {
    JSONArrayMustBeValid(pArray);
    
    if (JSONArrayIsEmpty(pArray)) {
        return;
    }

    JSONNode_t* current = pArray->pStart;
    JSONNode_t* next;

    while (current != NULL) {
        next = current->pNextSibling;
        JSONNodeDestroy(current);
        current = next;
    }
    pArray->pStart = NULL;
    pArray->pEnd = NULL;
}
/// @brief Destroys all children of a JSONObj.
/// @param pArray The obj
void JSONObjDestroyChildren(JSONObj_t* pObj) {
    JSONObjMustBeValid(pObj);

    JSONNode_t* current = pObj->pFirstChild;
    JSONNode_t* next;

    while (current != NULL) {
        next = current->pNextSibling;
        JSONNodeDestroy(current);
        current = next;
    }
}
/// @brief Recursively destroys each JSONNode in the tree. Handles everything related to the destruction process of any kind of JSONNode. Should be called every time you need to dispose of a JSONNode. 
/// @param pNode The node to destroy
void JSONNodeDestroy(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    switch (pNode->type) {
        case JSONArrayType: {
            JSONArray_t* pArray = pNode->value.pArray;
            if (!JSONArrayIsEmpty(pArray)) {
                JSONArrayDestroyElements(pArray);
            }
            jsonFuncs.free(pArray);
            break;
        }
        case JSONObjType: {
            JSONObj_t* pObj = pNode->value.pChildren;
            JSONObjMustBeValid(pObj);
            if (pObj->pFirstChild != NULL) {
                JSONObjDestroyChildren(pObj);
            }
            jsonFuncs.free(pObj);
            break;
        }
        default:
            break;
    }
    
    JSONNodeConnectNeighbors(pNode);
    jsonFuncs.free(pNode);
}

/// @brief Returns the Nth element of an array
/// @param pArray The array
/// @param n The index of the element
/// @return The node at the specified index in the array
JSONNode_t* JSONArrayGetNthNode(JSONArray_t* pArray, int_type n) {
    JSONArrayMustBeValid(pArray);
    JsonAssertMsg(n >= 0, "Tried to get a negative-th element of an array ! (what are you doing :sob:)");

    JsonAssertMsg(!JSONArrayIsEmpty(pArray), "Tried to get an element in an empty array !");

    JSONNode_t* current = pArray->pStart;
    for (int i = 0; i < n; i++) {
        current = current->pNextSibling;
        JsonAssertMsg(current != NULL, "Array indexed out of range.");
    }

    return current;
}
/// @brief Adds an element node to an array node
/// @param pArrayNode The array node.
/// @param pChild The child node.
void JSONArrayNodeAddNode(JSONNode_t* pArrayNode, JSONNode_t* pChild) {
    JsonAssert(pArrayNode != NULL);
    JsonAssertMsg(pArrayNode->type == JSONArrayType, "Tried to add an array element to a node that wasn't an array ! (what are you doing :sob:)");

    JSONArray_t* pArray = pArrayNode->value.pArray;

    // Make sure everything is valid
    JSONArrayMustBeValid(pArray);
    JsonAssert(pChild != NULL);

    // Make sure the node isn't already used in another array, and add the parent
    JsonAssertMsg(pChild->pParent == NULL, "Tried to add an array element, but the child node already has a parent ! Nodes should only have one reference at all times.");
    pChild->pParent = pArrayNode;

    if (JSONArrayIsEmpty(pArray)) {
        pArray->pStart = pChild;
        pArray->pEnd = pChild;
    } else {
        JSONNodeInsertAfter(pArray->pEnd, pChild);
        pArray->pEnd = pChild;
    }
}
/// @brief Removes an element from an array node at a specified index
/// @param pArrayNode The array node.
/// @param idx The index.
void JSONArrayNodeRemoveNode(JSONNode_t* pArrayNode, int_type idx) {
    JsonAssert(pArrayNode != NULL);
    JsonAssertMsg(pArrayNode->type == JSONArrayType, "Tried to remove an array element from a node that wasn't an array ! (what are you doing :sob:)");

    JSONArray_t* pArray = pArrayNode->value.pArray;

    JSONArrayMustBeValid(pArray);

    JSONNode_t* pNodeToDelete = NULL;
    
    if (idx == ARRAY_POS_END) {
        pNodeToDelete = pArray->pEnd;
    } else {
        pNodeToDelete = JSONArrayGetNthNode(pArray, idx);
    }
    JsonAssertMsg(pNodeToDelete != NULL, "Tried to delete node from empty array.");

    if (pNodeToDelete == pArray->pStart) {
        pArray->pStart = pArray->pStart->pNextSibling;
    }
    if (pNodeToDelete == pArray->pEnd) {
        pArray->pEnd = pArray->pEnd->pPrevSibling;
    }

    JSONNodeConnectNeighbors(pNodeToDelete);
    JSONNodeDestroy(pNodeToDelete);
}
void JSONArrayNodeRemoveAllNodes(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    JsonAssertMsg(pNode->type == JSONArrayType, "Tried to remove elements from a node that wasn't an array ! (what are you doing :sob:)");

    JSONArrayDestroyElements(pNode->value.pArray);
}

void JSONNodeAdoptChildNode(JSONNode_t* pParent, JSONNode_t* pChild) {
    JsonAssert(pParent != NULL);
    JsonAssert(pChild != NULL);
    JsonAssertMsg(pChild->pParent == NULL, "Tried to adopt a child node, but the child node already has a parent ! Nodes should only have one reference at all times.");
    JsonAssertMsg(JSONNodeCanHaveChildren(pParent), "Tried to add a child node to a node that can't have children !");

    pChild->pParent = pParent;
    
    JSONObj_t* pChildren = pParent->value.pChildren;
    JSONObjMustBeValid(pChildren);
    
    if (JSONObjIsEmpty(pChildren)) {
        pChildren->pFirstChild = pChild;
    } else {
        JSONNodeInsertAfter(pChildren->pLastChild, pChild);
    }
    pChildren->pLastChild = pChild;
}
JSONNode_t* JSONCreateNode(const char* name, JSONType_t type, JSONValue_t value) {
    JSONNode_t* pNode = (JSONNode_t*)jsonFuncs.malloc(sizeof(JSONNode_t));
    JsonAssert(pNode != NULL);
    
    pNode->name = name;
    pNode->type = type;
    pNode->value = value;
    pNode->pParent = NULL;
    pNode->pPrevSibling = NULL;
    pNode->pNextSibling = NULL;

    return pNode;
}

JSONNode_t* JSONCreateNamedNullNode(const char* name) {
    JSONValue_t jsonValue = {.i = NULL};
    return JSONCreateNode(name, JSONNullType, jsonValue);
}
JSONNode_t* JSONCreateNamedBoolNode(const char* name, bool value) {
    JSONValue_t jsonValue = {.b = value};
    return JSONCreateNode(name, JSONBoolType, jsonValue);
}
JSONNode_t* JSONCreateNamedIntNode(const char* name, int_type value) {
    JSONValue_t jsonValue = {.i = value};
    return JSONCreateNode(name, JSONIntType, jsonValue);
}
JSONNode_t* JSONCreateNamedFloatNode(const char* name, float_type value) {
    JSONValue_t jsonValue = {.f = value};
    return JSONCreateNode(name, JSONFloatType, jsonValue);
}
JSONNode_t* JSONCreateNamedStrNode(const char* name, const char* value) {
    JSONValue_t jsonValue = {.str = value};
    return JSONCreateNode(name, JSONStringType, jsonValue);
}
JSONNode_t* JSONCreateNewNamedObjNode(const char* name) {
    JSONObj_t* pEmptyChildren = (JSONObj_t*)jsonFuncs.malloc(sizeof(JSONObj_t));
    (void)jsonFuncs.memset(pEmptyChildren, 0, sizeof(JSONObj_t));

    JSONValue_t jsonValue = {.pChildren=pEmptyChildren};
    return JSONCreateNode(name, JSONObjType, jsonValue);
}
JSONNode_t* JSONCreateNamedObjNode(const char* name, JSONObj_t* pObj) {
    JSONValue_t jsonValue = {.pChildren=pObj};
    return JSONCreateNode(name, JSONObjType, jsonValue);
}
JSONNode_t* JSONCreateNewNamedArrayNode(const char* name) {
    JSONArray_t* pEmptyArray = (JSONArray_t*)jsonFuncs.malloc(sizeof(JSONArray_t));
    (void)jsonFuncs.memset(pEmptyArray, 0, sizeof(JSONArray_t));

    JSONValue_t jsonValue = {.pArray=pEmptyArray};
    return JSONCreateNode(name, JSONArrayType, jsonValue);
}
JSONNode_t* JSONCreateNamedArrayNode(const char* name, JSONArray_t* pArray) {
    JSONValue_t jsonValue = {.pArray=pArray};
    return JSONCreateNode(name, JSONArrayType, jsonValue);
}

JSONNode_t* JSONCreateNullNode() {
    return JSONCreateNamedNullNode("");
}
JSONNode_t* JSONCreateBoolNode(bool value) {
    return JSONCreateNamedBoolNode("", value);
}
JSONNode_t* JSONCreateIntNode(int_type value) {
    return JSONCreateNamedIntNode("", value);
}
JSONNode_t* JSONCreateFloatNode(float_type value) {
    return JSONCreateNamedFloatNode("", value);
}
JSONNode_t* JSONCreateStrNode(const char* value) {
    return JSONCreateNamedStrNode("", value);
}
JSONNode_t* JSONCreateNewObjNode() {
    return JSONCreateNewNamedObjNode("");
}
JSONNode_t* JSONCreateObjNode(JSONObj_t* pObj) {
    return JSONCreateNamedObjNode("", pObj);
}
JSONNode_t* JSONCreateNewArrayNode() {
    return JSONCreateNewNamedArrayNode("");
}
JSONNode_t* JSONCreateArrayNode(JSONArray_t* pArray) {
    return JSONCreateNamedArrayNode("", pArray);
}

void JSONNodeAddNamedNullNode(JSONNode_t* pParent, const char* name) {
    JSONNode_t* pNewNode = JSONCreateNamedNullNode(name);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNamedBoolNode(JSONNode_t* pParent, const char* name, bool value) {
    JSONNode_t* pNewNode = JSONCreateNamedBoolNode(name, value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNamedIntNode(JSONNode_t* pParent, const char* name, int_type value) {
    JSONNode_t* pNewNode = JSONCreateNamedIntNode(name, value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNamedFloatNode(JSONNode_t* pParent, const char* name, float_type value) {
    JSONNode_t* pNewNode = JSONCreateNamedFloatNode(name, value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNamedStringNode(JSONNode_t* pParent, const char* name, const char* value) {
    JSONNode_t* pNewNode = JSONCreateNamedStrNode(name, value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNewNamedObjNode(JSONNode_t* pParent, const char* name) {
    JSONNode_t* pNewNode = JSONCreateNewNamedObjNode(name);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNamedObjNode(JSONNode_t* pParent, const char* name, JSONObj_t* pObj) {
    JSONNode_t* pNewNode = JSONCreateNamedObjNode(name, pObj);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNewNamedArrayNode(JSONNode_t* pParent, const char* name) {
    JSONNode_t* pNewNode = JSONCreateNewNamedArrayNode(name);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNamedArrayNode(JSONNode_t* pParent, const char* name, JSONArray_t* pArray) {
    JSONNode_t* pNewNode = JSONCreateNamedArrayNode(name, pArray);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}

void JSONNodeAddNullNode(JSONNode_t* pParent) {
    JSONNode_t* pNewNode = JSONCreateNullNode();
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddBoolNode(JSONNode_t* pParent, bool value) {
    JSONNode_t* pNewNode = JSONCreateBoolNode(value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddIntNode(JSONNode_t* pParent, int_type value) {
    JSONNode_t* pNewNode = JSONCreateIntNode(value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddFloatNode(JSONNode_t* pParent, float_type value) {
    JSONNode_t* pNewNode = JSONCreateFloatNode(value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddStringNode(JSONNode_t* pParent, const char* value) {
    JSONNode_t* pNewNode = JSONCreateStrNode(value);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNewObjNode(JSONNode_t* pParent) {
    JSONNode_t* pNewNode = JSONCreateNewObjNode();
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddObjNode(JSONNode_t* pParent, JSONObj_t* pObj) {
    JSONNode_t* pNewNode = JSONCreateObjNode(pObj);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddNewArrayNode(JSONNode_t* pParent) {
    JSONNode_t* pNewNode = JSONCreateNewArrayNode();
    JSONNodeAdoptChildNode(pParent, pNewNode);
}
void JSONNodeAddArrayNode(JSONNode_t* pParent, JSONArray_t* pArray) {
    JSONNode_t* pNewNode = JSONCreateArrayNode(pArray);
    JSONNodeAdoptChildNode(pParent, pNewNode);
}

// for these i'm not using int_type because i chose to adhere to libc functions' return types instead (i.e. sizeof and strlen are size_t, snprintf returns int)

/// @brief Gets the length of a node's key followed by a ':'.
/// @param pNode The node
/// @return The length
size_t JSONNodeGetPreValLength(JSONNode_t* pNode) {
    if (pNode->pParent != NULL) {
        if (pNode->pParent->type == JSONObjType) {
            return sizeof(STRING_DELIM) + jsonFuncs.strlen(pNode->name) + sizeof(STRING_DELIM) + sizeof(KEYVAL_SEPARATOR);
        }
    }
    return 0;
}
static size_t JSONBoolNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONBoolType);
    return jsonFuncs.strlen(pNode->value.b == 1 ? "true" : "false");
}
static size_t JSONIntNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONIntType);
    int length = jsonFuncs.snprintf(NULL, 0, "%d", pNode->value.i);
    JsonAssert(length >= 0);
    return (size_t)length;
}
static size_t JSONFloatNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONFloatType);
    int length = jsonFuncs.snprintf(NULL, 0, "%g", pNode->value.f);
    JsonAssert(length >= 0);
    return (size_t)length;
}
static size_t JSONStringNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONStringType);
    return sizeof(STRING_DELIM) + jsonFuncs.strlen(pNode->value.str) + sizeof(STRING_DELIM);
}
static size_t JSONObjNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONObjType);

    size_t length = sizeof(JSONOBJ_START) + sizeof(JSONOBJ_END);
    JSONObj_t* jsonObj = pNode->value.pChildren;

    if (!JSONObjIsEmpty(jsonObj)) {
        JSONNode_t* current = jsonObj->pFirstChild;
        while (current != NULL) {
            length += JSONNodeGetLength(current);
            if (current != jsonObj->pLastChild) {
                length += sizeof(CHILD_SEPARATOR);
            }
            current = current->pNextSibling;
        }
    }
    return length;
}
static size_t JSONArrayNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONArrayType);

    size_t length = sizeof(JSONARRAY_START) + sizeof(JSONARRAY_END);
    JSONArray_t* jsonArray = pNode->value.pArray;

    if (!JSONArrayIsEmpty(jsonArray)) {
        JSONNode_t* current = jsonArray->pStart;
        while (current != NULL) {
            length += JSONNodeGetValueLength(current);
            if (current != jsonArray->pEnd) {
                length += sizeof(CHILD_SEPARATOR);
            }
            current = current->pNextSibling;
        }
    }
    return length;
}
/// @brief Gets the length of a node's value, recursing over JSONObjs and JSONArrays.
/// @param pNode The node
/// @return The length
size_t JSONNodeGetValueLength(JSONNode_t* pNode) {
    size_t length;
    switch (pNode->type) {
        case JSONNullType: {
            length = 4; // it's always just null
            break;
        }
        case JSONBoolType: {
            length = JSONBoolNodeGetValueLength(pNode);
            break;
        }
        case JSONIntType: {
            length = JSONIntNodeGetValueLength(pNode);
            break;
        }
        case JSONFloatType: {
            length = JSONFloatNodeGetValueLength(pNode);
            break;
        }
        case JSONStringType: {
            length = JSONStringNodeGetValueLength(pNode);
            break;
        }
        case JSONObjType: {
            length = JSONObjNodeGetValueLength(pNode);
            break;
        }
        case JSONArrayType: {
            length = JSONArrayNodeGetValueLength(pNode);
            break;
        }
        default: {
            length = 0;
            break;
        }
    }
    return length;
}
/// @brief Gets the length of a node (key + ':' + value), recursing over JSONObjs and JSONArrays.
/// @param pNode The node
/// @return The length
size_t JSONNodeGetLength(JSONNode_t* pNode) {
    return JSONNodeGetPreValLength(pNode) + JSONNodeGetValueLength(pNode);
}

static void JSONNodeDumpPreVal(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    if (pNode->pParent == NULL) return;
    if (pNode->pParent->type != JSONObjType) return;

    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;

    size_t nameLength = jsonFuncs.strlen(pNode->name);
    (void)jsonFuncs.strncpy(pBuffer + *pPosition, pNode->name, nameLength);
    (*pPosition) += nameLength;

    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;
    pBuffer[*pPosition] = KEYVAL_SEPARATOR;
    (*pPosition)++;
}
static void JSONNodeNullValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONNullType);

    (void)jsonFuncs.strncpy(pBuffer + *pPosition, "null", 4);
    (*pPosition) += 4;
}
static void JSONNodeBoolValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONBoolType);

    if (pNode->value.b) {
        (void)jsonFuncs.strncpy(pBuffer + *pPosition, "true", 4);
        (*pPosition) += 4;
    } else {
        (void)jsonFuncs.strncpy(pBuffer + *pPosition, "false", 5);
        (*pPosition) += 5;
    }
}
static void JSONNodeIntValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONIntType);

    size_t length = JSONIntNodeGetValueLength(pNode);
    (void)jsonFuncs.snprintf(pBuffer + *pPosition, length+1, "%d", pNode->value.i);
    (*pPosition) += length;
}
static void JSONNodeFloatValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONFloatType);
    
    size_t length = JSONFloatNodeGetValueLength(pNode);
    (void)jsonFuncs.snprintf(pBuffer + *pPosition, length+1, "%g", pNode->value.f);
    (*pPosition) += length;
}
static void JSONNodeStringValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONStringType);

    const char* str = pNode->value.str;
    size_t strLength = jsonFuncs.strlen(str);
    
    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;

    (void)jsonFuncs.strncpy(pBuffer + *pPosition, str, strLength);
    (*pPosition) += strLength;

    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;
}
static void JSONNodeObjValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONObjType);

    pBuffer[*pPosition] = JSONOBJ_START;
    (*pPosition)++;
    
    JSONObj_t* jsonObj = pNode->value.pChildren;
    if (!JSONObjIsEmpty(jsonObj)) {
        JSONNode_t* current = jsonObj->pFirstChild;
        while (current != NULL) {
            JSONNodeDump(current, pBuffer, pPosition);
            if (current != jsonObj->pLastChild) {
                pBuffer[*pPosition] = CHILD_SEPARATOR;
                (*pPosition)++;
            }
            current = current->pNextSibling;
        }
    }
    pBuffer[*pPosition] = JSONOBJ_END;
    (*pPosition)++;
}
static void JSONNodeArrayValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONArrayType);

    pBuffer[*pPosition] = JSONARRAY_START;
    (*pPosition)++;
    
    JSONArray_t* jsonArray = pNode->value.pArray;
    if (!JSONArrayIsEmpty(jsonArray)) {
        JSONNode_t* current = jsonArray->pStart;
        while (current != NULL) {
            JSONNodeValueDump(current, pBuffer, pPosition);
            if (current != jsonArray->pEnd) {
                pBuffer[*pPosition] = CHILD_SEPARATOR;
                (*pPosition)++;
            }
            current = current->pNextSibling;
        }
    }
    pBuffer[*pPosition] = JSONARRAY_END;
    (*pPosition)++;
}

void JSONNodeValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    switch (pNode->type) {
        case JSONNullType: {
            JSONNodeNullValueDump(pNode, pBuffer, pPosition);
            break;
        }
        case JSONBoolType: {
            JSONNodeBoolValueDump(pNode, pBuffer, pPosition);
            break;
        }
        case JSONIntType: {
            JSONNodeIntValueDump(pNode, pBuffer, pPosition);
            break;
        }
        case JSONFloatType: {
            JSONNodeFloatValueDump(pNode, pBuffer, pPosition);
            break;
        }
        case JSONStringType: {
            JSONNodeStringValueDump(pNode, pBuffer, pPosition);
            break;
        }
        case JSONObjType: {
            JSONNodeObjValueDump(pNode, pBuffer, pPosition);
            break;
        }
        case JSONArrayType: {
            JSONNodeArrayValueDump(pNode, pBuffer, pPosition);
            break;
        }
        default: {
            // if you somehow manage to get a node with an invalid type then something has gone insanely terribly horribly wrong
            // in which case one could say you deserve however many layers of UB are bound to arise from this,
            // however since i'm a nice person I put an assert here so you could just end it right there
            // before whatever fire your computer is on destroys the state of your program even further than it already has
            JsonAssertMsg(false, "Tried to dump node of unknown type !");
        }
    }
}

void JSONNodeDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition) {
    JSONNodeDumpPreVal(pNode, pBuffer, pPosition);
    JSONNodeValueDump(pNode, pBuffer, pPosition);
}

/// @brief Recursively dumps a JSONNode
/// @param pRoot The root of the tree to dump (or a single node if that's what you want to dump)
/// @return The string representation of the JSON node. Must be freed.
const char* JSONDump(JSONNode_t* pRoot) {
    char* pBuffer = NULL;
    int_type length = JSONNodeGetLength(pRoot);
    pBuffer = (char*)jsonFuncs.malloc(length + 1);
    int_type position = 0;
    JSONNodeValueDump(pRoot, pBuffer, &position);
    pBuffer[length] = '\0';
    return (const char*)pBuffer;
}
