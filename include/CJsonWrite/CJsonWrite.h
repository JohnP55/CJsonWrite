// Made by JohnP55
// i made this for a super paper mario mod
// you're welcome

#pragma once
#ifndef __cplusplus
#include <stdbool.h>
#endif
#include "CJsonWrite/CJsonWrite_config.h"

/// @brief Types of values that a JSONNode can have.
typedef enum JSONType {
    JSONNullType,
    JSONBoolType,
    JSONIntType,
    JSONFloatType,
    JSONStringType,
    JSONObjType,
    JSONArrayType
} JSONType_t;

/// @brief Different statuses that can be returned.
typedef enum JSONStatus {
    JSONSuccess,
    JSONWarningArrayIsEmpty
} JSONStatus_t;

/// @brief union type for the value of a JSONNode.
/*
 * The primitive types are self-explanatory: bool is a boolean value (true/false), char* is a string value, etc.
 * The interesting ones are JSONNode and JSONArray, but actually they're also pretty self-explanatory.
 * JSONNode means the value of the node is a JSON object itself (the root is of type JSONNode, and so are elements of an array, etc.)
 * JSONArray means the value of the node is an array, and that array has a bunch of nodes inside it (or possibly none). It's just an array.
*/
typedef union JSONValue {
    bool b;
    int_type i;
    float_type f;
    const char* str;
    struct JSONObj* pChildren;
    struct JSONArray* pArray;
} JSONValue_t;

/// @brief The main man: this is a thing. Any actual attribute that a JSON tree can have is a JSONNode.
/// The root of the tree is a JSONNode. A key-value pair attribute in a node is a JSONNode. Elements of a JSON array are JSONNodes.
/// It has a name, a type (see JSONType), a value (see JSONValue), a pointer to its parent node, and pointers to its previous and next sibling nodes.
/// A JSONNode should only ever be referenced once. Identical values should use separate JSONNodes.
/// (the only exception is a JSONArray with only one element, in which case pStart and pEnd would point to the same JSONNode)
typedef struct JSONNode {
    const char* name;
    JSONType_t type;
    JSONValue_t value;
    struct JSONNode* pParent;
    struct JSONNode* pPrevSibling;
    struct JSONNode* pNextSibling;
} JSONNode_t;

typedef struct JSONObj {
    struct JSONNode* pFirstChild;
    struct JSONNode* pLastChild;
} JSONObj_t;

/// @brief This is the JSONValue type for a JSON array. It's just a linked list containing a pointer to the start and end.
typedef struct JSONArray {
    JSONNode_t* pStart;
    JSONNode_t* pEnd;
} JSONArray_t;

/// @brief Function pointers for CJsonWrite
typedef struct JSONFuncs {
    void* (*malloc)(size_t);
    void (*free)(void*);
    void* (*memset)(void*, int, size_t);
    size_t (*strlen)(const char*);
    int (*snprintf)(char*, size_t, const char*, ...);
    char* (*strncpy)(char*, const char*, size_t);
} JSONFuncs_t;
extern JSONFuncs_t jsonFuncs;

// For removing the last element of an array
#define ARRAY_POS_END -1

#ifdef __cplusplus
extern "C" {
#endif

void CJsonWriteInit(JSONFuncs_t* _memory);

bool JSONArrayIsValid(JSONArray_t* pArray);
void JSONArrayMustBeValid(JSONArray_t* pArray);
bool JSONArrayIsEmpty(JSONArray_t* pArray);
bool JSONArrayNodeIsEmpty(JSONNode_t* pNode);
bool JSONObjIsValid(JSONObj_t* pObj);
void JSONObjMustBeValid(JSONObj_t* pObj);
bool JSONObjIsEmpty(JSONObj_t* pObj);
size_t JSONArrayGetNumElements(JSONArray_t* pArray);
bool JSONNodeCanHaveChildren(JSONNode_t* pNode);

JSONStatus_t JSONNodeConnectNeighbors(JSONNode_t* pNode);
JSONStatus_t JSONNodeInsertAfter(JSONNode_t* pLeft, JSONNode_t* pNewNode);

JSONStatus_t JSONArrayDestroyElements(JSONArray_t* pArray);
JSONStatus_t JSONObjDestroyChildren(JSONObj_t* pObj);
JSONStatus_t JSONNodeDestroy(JSONNode_t* pNode);

JSONStatus_t JSONArrayGetNthNode(JSONArray_t* pArray, int_type n, JSONNode_t** pOutNode);
void JSONArrayNodeAddNode(JSONNode_t* pArrayNode, JSONNode_t* pChild);
JSONStatus_t JSONArrayNodeRemoveNode(JSONNode_t* pArrayNode, int_type idx);
JSONStatus_t JSONArrayNodeRemoveAllNodes(JSONNode_t* pNode);

JSONStatus_t JSONNodeAdoptChildNode(JSONNode_t* pParent, JSONNode_t* pChild);
JSONNode_t* JSONCreateNode(const char* name, JSONType_t type, JSONValue_t value);

JSONNode_t* JSONCreateNamedNullNode(const char* name);
JSONNode_t* JSONCreateNamedBoolNode(const char* name, bool value);
JSONNode_t* JSONCreateNamedIntNode(const char* name, int_type value);
JSONNode_t* JSONCreateNamedFloatNode(const char* name, float_type value);
JSONNode_t* JSONCreateNamedStrNode(const char* name, const char* value);
JSONNode_t* JSONCreateNewNamedObjNode(const char* name);
JSONNode_t* JSONCreateNamedObjNode(const char* name, JSONObj_t* pObj);
JSONNode_t* JSONCreateNewNamedArrayNode(const char* name);
JSONNode_t* JSONCreateNamedArrayNode(const char* name, JSONArray_t* pArray);

JSONNode_t* JSONCreateNullNode();
JSONNode_t* JSONCreateBoolNode(bool value);
JSONNode_t* JSONCreateIntNode(int_type value);
JSONNode_t* JSONCreateFloatNode(float_type value);
JSONNode_t* JSONCreateStrNode(const char* value);
JSONNode_t* JSONCreateNewObjNode();
JSONNode_t* JSONCreateObjNode(JSONObj_t* pObj);
JSONNode_t* JSONCreateNewArrayNode();
JSONNode_t* JSONCreateArrayNode(JSONArray_t* pArray);

JSONStatus_t JSONNodeAddNamedNullNode(JSONNode_t* pParent, const char* name);
JSONStatus_t JSONNodeAddNamedBoolNode(JSONNode_t* pParent, const char* name, bool value);
JSONStatus_t JSONNodeAddNamedIntNode(JSONNode_t* pParent, const char* name, int_type value);
JSONStatus_t JSONNodeAddNamedFloatNode(JSONNode_t* pParent, const char* name, float_type value);
JSONStatus_t JSONNodeAddNamedStringNode(JSONNode_t* pNode, const char* name, const char* value);
JSONStatus_t JSONNodeAddNewNamedObjNode(JSONNode_t* pParent, const char* name);
JSONStatus_t JSONNodeAddNamedObjNode(JSONNode_t* pParent, const char* name, JSONObj_t* pObj);
JSONStatus_t JSONNodeAddNewNamedArrayNode(JSONNode_t* pParent, const char* name);
JSONStatus_t JSONNodeAddNamedArrayNode(JSONNode_t* pParent, const char* name, JSONArray_t* pArray);

JSONStatus_t JSONNodeAddNullNode(JSONNode_t* pParent);
JSONStatus_t JSONNodeAddBoolNode(JSONNode_t* pParent, bool value);
JSONStatus_t JSONNodeAddIntNode(JSONNode_t* pParent, int_type value);
JSONStatus_t JSONNodeAddFloatNode(JSONNode_t* pParent, float_type value);
JSONStatus_t JSONNodeAddStringNode(JSONNode_t* pNode, const char* value);
JSONStatus_t JSONNodeAddNewObjNode(JSONNode_t* pParent);
JSONStatus_t JSONNodeAddObjNode(JSONNode_t* pParent, JSONObj_t* pObj);
JSONStatus_t JSONNodeAddNewArrayNode(JSONNode_t* pParent);
JSONStatus_t JSONNodeAddArrayNode(JSONNode_t* pParent, JSONArray_t* pArray);

size_t JSONNodeGetPreValLength(JSONNode_t* pNode);
size_t JSONNodeGetValueLength(JSONNode_t* pNode);
size_t JSONNodeGetLength(JSONNode_t* pNode);

void JSONNodeValueDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition);
void JSONNodeDump(JSONNode_t* pNode, char* pBuffer, int_type* pPosition);

const char* JSONDump(JSONNode_t* pRoot);

#define JSONOBJ_START (char) '{'
#define JSONOBJ_END (char) '}'
#define JSONARRAY_START (char) '['
#define JSONARRAY_END (char) ']'
#define STRING_DELIM (char) '"'
#define KEYVAL_SEPARATOR (char) ':'
#define CHILD_SEPARATOR (char) ','

#ifdef __cplusplus
}
#endif
