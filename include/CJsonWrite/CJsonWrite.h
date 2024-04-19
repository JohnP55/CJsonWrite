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
    JSONWarningArrayIsEmpty,
    JSONErrorArrayOutOfRange
} JSONStatus_t;

typedef struct JSONObj {
    struct JSONNode* pFirstChild;
    struct JSONNode* pLastChild;
} JSONObj_t;

/// @brief union type for the value of a JSONNode.
/*
 * The primitive types are self-explanatory: bool is a boolean value (true/false), char* is a string value, etc.
 * The interesting ones are JSONNode and JSONArray, but actually they're also pretty self-explanatory.
 * JSONNode means the value of the node is a JSON object itself (the root is of type JSONNode, and so are elements of an array, etc.)
 * JSONArray means the value of the node is an array, and that array has a bunch of nodes inside it (or possibly none). It's just an array.
*/
typedef union JSONValue {
    bool b;
    int i;
    float f;
    const char* str;
    JSONObj_t* pChildren;
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

/// @brief This is the JSONValue type for a JSON array. It's just a linked list containing a pointer to the start and end.
typedef struct JSONArray {
    JSONNode_t* pStart;
    JSONNode_t* pEnd;
} JSONArray_t;

/// @brief Function pointers for CJsonWrite
typedef struct JSONFuncs {
    void* (*malloc)(size_t);
    void (*free)(void*);
    void* (*memset)(void*, s32, size_t);
    size_t (*strlen)(const char*);
    s32 (*snprintf)(char*, size_t, const char*, ...);
    char* (*strncpy)(char*, const char*, size_t);
} JSONFuncs_t;
extern JSONFuncs_t jsonFuncs;

#define MANDATORY(res)   \
    do {    \
        JsonAssert(res == JSONSuccess); \
    } while(0)

#ifdef __cplusplus
extern "C" {
#endif

void CJsonWriteInit(JSONFuncs_t* _memory);

bool JSONArrayIsValid(JSONArray_t* pArray);
bool JSONArrayIsEmpty(JSONArray_t* pArray);
bool JSONNodeIsAvailable(JSONNode_t* pNode);
bool JSONObjIsValid(JSONObj_t* pObj);
bool JSONObjIsEmpty(JSONObj_t* pObj);
bool JSONNodeCanHaveChildren(JSONNode_t* pNode);

JSONStatus_t JSONNodeConnectNeighbors(JSONNode_t* pNode);
JSONStatus_t JSONNodeInsertAfter(JSONNode_t* pLeft, JSONNode_t* pNewNode);

JSONStatus_t JSONArrayDestroyElements(JSONArray_t* pArray);
JSONStatus_t JSONObjDestroyChildren(JSONObj_t* pObj);
JSONStatus_t JSONNodeDestroy(JSONNode_t* pNode);

JSONStatus_t JSONArrayGetNthNode(JSONArray_t* pArray, s32 n, JSONNode_t** pOutNode);
void JSONArrayNodeAddNode(JSONNode_t* pNode, JSONNode_t* pChild);
JSONStatus_t JSONArrayNodeRemoveNode(JSONNode_t* pNode, s32 idx);

JSONStatus_t JSONNodeAdoptChildNode(JSONNode_t* pParent, JSONNode_t* pChild);
JSONNode_t* JSONCreateNode(const char* name, JSONType_t type, JSONValue_t value);

JSONNode_t* JSONCreateNamedNullNode(const char* name);
JSONNode_t* JSONCreateNamedBoolNode(const char* name, bool value);
JSONNode_t* JSONCreateNamedIntNode(const char* name, s32 value);
JSONNode_t* JSONCreateNamedFloatNode(const char* name, f32 value);
JSONNode_t* JSONCreateNamedStrNode(const char* name, const char* value);
JSONNode_t* JSONCreateNewNamedObjNode(const char* name);
JSONNode_t* JSONCreateNamedObjNode(const char* name, JSONObj_t* pObj);
JSONNode_t* JSONCreateNewNamedArrayNode(const char* name);
JSONNode_t* JSONCreateNamedArrayNode(const char* name, JSONArray_t* pArray);

JSONNode_t* JSONCreateNullNode();
JSONNode_t* JSONCreateBoolNode(bool value);
JSONNode_t* JSONCreateIntNode(s32 value);
JSONNode_t* JSONCreateFloatNode(f32 value);
JSONNode_t* JSONCreateStrNode(const char* value);
JSONNode_t* JSONCreateNewObjNode();
JSONNode_t* JSONCreateObjNode(JSONObj_t* pObj);
JSONNode_t* JSONCreateNewArrayNode();
JSONNode_t* JSONCreateArrayNode(JSONArray_t* pArray);

JSONStatus_t JSONNodeAddNamedNullNode(JSONNode_t* pParent, const char* name);
JSONStatus_t JSONNodeAddNamedBoolNode(JSONNode_t* pParent, const char* name, bool value);
JSONStatus_t JSONNodeAddNamedIntNode(JSONNode_t* pParent, const char* name, s32 value);
JSONStatus_t JSONNodeAddNamedFloatNode(JSONNode_t* pParent, const char* name, f32 value);
JSONStatus_t JSONNodeAddNamedStringNode(JSONNode_t* pNode, const char* name, const char* value);
JSONStatus_t JSONNodeAddNewNamedObjNode(JSONNode_t* pParent, const char* name);
JSONStatus_t JSONNodeAddNamedObjNode(JSONNode_t* pParent, const char* name, JSONObj_t* pObj);
JSONStatus_t JSONNodeAddNewNamedArrayNode(JSONNode_t* pParent, const char* name);
JSONStatus_t JSONNodeAddNamedArrayNode(JSONNode_t* pParent, const char* name, JSONArray_t* pArray);

JSONStatus_t JSONNodeAddNullNode(JSONNode_t* pParent);
JSONStatus_t JSONNodeAddBoolNode(JSONNode_t* pParent, bool value);
JSONStatus_t JSONNodeAddIntNode(JSONNode_t* pParent, s32 value);
JSONStatus_t JSONNodeAddFloatNode(JSONNode_t* pParent, f32 value);
JSONStatus_t JSONNodeAddStringNode(JSONNode_t* pNode, const char* value);
JSONStatus_t JSONNodeAddNewObjNode(JSONNode_t* pParent);
JSONStatus_t JSONNodeAddObjNode(JSONNode_t* pParent, JSONObj_t* pObj);
JSONStatus_t JSONNodeAddNewArrayNode(JSONNode_t* pParent);
JSONStatus_t JSONNodeAddArrayNode(JSONNode_t* pParent, JSONArray_t* pArray);

s32 JSONNodeGetPreValLength(JSONNode_t* pNode);
s32 JSONNodeGetValueLength(JSONNode_t* pNode);
s32 JSONNodeGetLength(JSONNode_t* pNode);

void JSONNodeValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition);
void JSONNodeDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition);

const char* JSONDump(JSONNode_t* pRoot);

#define JSONOBJ_START '{'
#define JSONOBJ_END '}'
#define JSONARRAY_START '['
#define JSONARRAY_END ']'
#define STRING_DELIM '"'
#define KEYVAL_SEPARATOR ':'
#define CHILD_SEPARATOR ','

#ifdef __cplusplus
}
#endif
