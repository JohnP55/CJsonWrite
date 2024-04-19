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
    if ((pArray->pStart == NULL) != (pArray->pEnd == NULL)) return false; // if start is null and end isn't then something went horribly wrong, abort now
    return true;
}
bool JSONArrayIsEmpty(JSONArray_t* pArray) {
    JsonAssert(JSONArrayIsValid(pArray));
    return pArray->pStart == NULL; // checking pEnd would work too because if only one is null then something terrible has happened
}
bool JSONNodeIsAvailable(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    if (pNode->pPrevSibling != NULL) return false;
    if (pNode->pNextSibling != NULL) return false;
    return true;
}
bool JSONObjIsValid(JSONObj_t* pObj) {
    if (pObj == NULL) return false;
    if ((pObj->pFirstChild == NULL) != (pObj->pLastChild == NULL)) return false; // if start is null and end isn't then something went horribly wrong, abort now
    return true;
}
bool JSONObjIsEmpty(JSONObj_t* pObj) {
    JsonAssert(JSONObjIsValid(pObj));
    return pObj->pFirstChild == NULL; // same as JSONArrayIsEmpty
}
bool JSONNodeCanHaveChildren(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    return pNode->type == JSONObjType || pNode->type == JSONArrayType;
}
#pragma endregion

#pragma region NODE_UTILS
/// @brief Connects its previous node to its next node, and its next node to its previous node. This is the "write to the JSONNode" step of removing a node from a JSONArray.
/// @param pNode The node in question
/// @return The outcome (always JSONSuccess for this one... for now)
JSONStatus_t JSONNodeConnectNeighbors(JSONNode_t* pNode) {
    JsonAssert(pNode != NULL);
    JSONNode_t* pPrev = pNode->pPrevSibling;
    JSONNode_t* pNext = pNode->pNextSibling;
    
    if (pPrev != NULL) {
        pPrev->pNextSibling = pNext;
    }
    if (pNext != NULL) {
        pNext->pPrevSibling = pPrev;
    }
    return JSONSuccess;
}

/// @brief Connects two nodes together, linking the leftmost node's next sibling node.
/// @param pLeft The leftmost node to link with.
/// @param pNewNode A node with no prior connections ready to be inserted.
/// @return JSONSuccess
JSONStatus_t JSONNodeInsertAfter(JSONNode_t* pLeft, JSONNode_t* pNewNode) {
    JsonAssert(pLeft != NULL);
    JsonAssert(JSONNodeIsAvailable(pNewNode));
    
    JSONNode_t* right = pLeft->pNextSibling;
    if (right != NULL) {
        pNewNode->pNextSibling = right;
        right->pPrevSibling = pNewNode;
    }

    pLeft->pNextSibling = pNewNode;
    pNewNode->pPrevSibling = pLeft;
    return JSONSuccess;
}
#pragma endregion

/// @brief Destroys all elements of a JSONArray.
/// @param pArray The array
/// @return JSONSuccess
JSONStatus_t JSONArrayDestroyElements(JSONArray_t* pArray) {
    JsonAssert(JSONArrayIsValid(pArray));
    
    if (JSONArrayIsEmpty(pArray)) {
        return JSONWarningArrayIsEmpty;
    }

    JSONNode_t* current = pArray->pStart;
    JSONNode_t* next;

    while (current != NULL) {
        next = current->pNextSibling;
        JSONNodeDestroy(current);
        current = next;
    }
    return JSONSuccess;
}
/// @brief Destroys all children of a JSONObj.
/// @param pArray The obj
/// @return JSONSuccess
JSONStatus_t JSONObjDestroyChildren(JSONObj_t* pObj) {
    JsonAssert(JSONObjIsValid(pObj));

    JSONNode_t* current = pObj->pFirstChild;
    JSONNode_t* next;

    while (current != NULL) {
        next = current->pNextSibling;
        JSONNodeDestroy(current);
        current = next;
    }
    return JSONSuccess;
}
/// @brief Recursively destroys each JSONNode in the tree. Handles everything related to the destruction process of any kind of JSONNode. Should be called every time you need to dispose of a JSONNode. 
/// @param pNode The node to destroy
/// @return The result (should be JSONSuccess all the time i think)
JSONStatus_t JSONNodeDestroy(JSONNode_t* pNode) {
    switch (pNode->type) {
        case JSONArrayType: {
            JSONArray_t* pArray = pNode->value.pArray;
            if (!JSONArrayIsEmpty(pArray)) {
                MANDATORY(JSONArrayDestroyElements(pArray));
            }
            jsonFuncs.free(pArray);
            break;
        }
        case JSONObjType: {
            JSONObj_t* pObj = pNode->value.pChildren;
            JsonAssert(JSONObjIsValid(pObj));
            if (pObj->pFirstChild != NULL) {
                MANDATORY(JSONObjDestroyChildren(pObj));
            }
            jsonFuncs.free(pObj);
            break;
        }
        default:
            break;
    }
    
    JSONNodeConnectNeighbors(pNode);

    JSONNode_t* pParent = pNode->pParent;
    if (pParent != NULL) {
        if (pNode->pPrevSibling == NULL && pNode->pNextSibling == NULL) {
            pParent->type = JSONNullType;
            pParent->value.i = NULL;
        }
    }
    jsonFuncs.free(pNode);
    return JSONSuccess;
}

/// @brief Returns the Nth element of an array
/// @param pArray The array
/// @param ppOutNode The out
/// @param n 
/// @return The outcome of the operation.
JSONStatus_t JSONArrayGetNthNode(JSONArray_t* pArray, s32 n, JSONNode_t** ppOutNode) {
    JsonAssert(JSONArrayIsValid(pArray));
    JsonAssert(n >= 0);

    if (pArray->pStart == NULL)
        return JSONWarningArrayIsEmpty;

    JSONNode_t* current = pArray->pStart;
    for (int i = 0; i < n; i++) {
        current = current->pNextSibling;
        if (current == NULL)
            return JSONErrorArrayOutOfRange;
    }

    *ppOutNode = current;
    return JSONSuccess;
}
void JSONArrayNodeAddNode(JSONNode_t* pNode, JSONNode_t* pChild) {
    JsonAssert(pNode->type == JSONArrayType);
    JSONArray_t* pArray = pNode->value.pArray;

    // Make sure everything is valid
    JsonAssert(JSONArrayIsValid(pArray));
    JsonAssert(pChild != NULL);

    // Make sure the node isn't already used in another array
    JsonAssert(pChild->pParent == NULL);
    
    if (JSONArrayIsEmpty(pArray)) {
        pArray->pStart = pChild;
        pArray->pEnd = pChild;
    } else {
        JSONNodeInsertAfter(pArray->pEnd, pChild);
        pArray->pEnd = pChild;
    }
}
JSONStatus_t JSONArrayNodeRemoveNode(JSONNode_t* pNode, s32 idx) {
    JsonAssert(pNode->type == JSONArrayType);
    JSONArray_t* pArray = pNode->value.pArray;

    JsonAssert(JSONArrayIsValid(pArray));

    JSONNode_t* pNodeToDelete = NULL;
    JSONStatus_t res = JSONArrayGetNthNode(pArray, idx, &pNodeToDelete);
    if (res != JSONSuccess) return res;

    MANDATORY(JSONNodeConnectNeighbors(pNodeToDelete));
    MANDATORY(JSONNodeDestroy(pNodeToDelete));
    return JSONSuccess;
}

JSONStatus_t JSONNodeAdoptChildNode(JSONNode_t* pParent, JSONNode_t* pChild) {
    JsonAssert(pChild->pParent == NULL);
    JsonAssert(JSONNodeCanHaveChildren(pParent));

    pChild->pParent = pParent;
    
    JSONObj_t* pChildren = pParent->value.pChildren;
    JsonAssert(JSONObjIsValid(pChildren));
    
    if (JSONObjIsEmpty(pChildren)) {
        pChildren->pFirstChild = pChild;
    } else {
        JSONNodeInsertAfter(pChildren->pLastChild, pChild);
    }
    pChildren->pLastChild = pChild;
    return JSONSuccess;
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
JSONNode_t* JSONCreateNamedIntNode(const char* name, s32 value) {
    JSONValue_t jsonValue = {.i = value};
    return JSONCreateNode(name, JSONIntType, jsonValue);
}
JSONNode_t* JSONCreateNamedFloatNode(const char* name, f32 value) {
    JSONValue_t jsonValue = {.f = value};
    return JSONCreateNode(name, JSONFloatType, jsonValue);
}
JSONNode_t* JSONCreateNamedStrNode(const char* name, const char* value) {
    JSONValue_t jsonValue = {.str = value};
    return JSONCreateNode(name, JSONStringType, jsonValue);
}
JSONNode_t* JSONCreateNewNamedObjNode(const char* name) {
    JSONObj_t* pEmptyChildren = (JSONObj_t*)jsonFuncs.malloc(sizeof(JSONObj_t));
    jsonFuncs.memset(pEmptyChildren, 0, sizeof(JSONObj_t));

    JSONValue_t jsonValue = {.pChildren=pEmptyChildren};
    return JSONCreateNode(name, JSONObjType, jsonValue);
}
JSONNode_t* JSONCreateNamedObjNode(const char* name, JSONObj_t* pObj) {
    JSONValue_t jsonValue = {.pChildren=pObj};
    return JSONCreateNode(name, JSONObjType, jsonValue);
}
JSONNode_t* JSONCreateNewNamedArrayNode(const char* name) {
    JSONArray_t* pEmptyArray = (JSONArray_t*)jsonFuncs.malloc(sizeof(JSONArray_t));
    jsonFuncs.memset(pEmptyArray, 0, sizeof(JSONArray_t));

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
JSONNode_t* JSONCreateIntNode(s32 value) {
    return JSONCreateNamedIntNode("", value);
}
JSONNode_t* JSONCreateFloatNode(f32 value) {
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

JSONStatus_t JSONNodeAddNamedNullNode(JSONNode_t* pParent, const char* name) {
    JSONNode_t* pNewNode = JSONCreateNamedNullNode(name);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNamedBoolNode(JSONNode_t* pParent, const char* name, bool value) {
    JSONNode_t* pNewNode = JSONCreateNamedBoolNode(name, value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNamedIntNode(JSONNode_t* pParent, const char* name, s32 value) {
    JSONNode_t* pNewNode = JSONCreateNamedIntNode(name, value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNamedFloatNode(JSONNode_t* pParent, const char* name, f32 value) {
    JSONNode_t* pNewNode = JSONCreateNamedFloatNode(name, value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNamedStringNode(JSONNode_t* pParent, const char* name, const char* value) {
    JSONNode_t* pNewNode = JSONCreateNamedStrNode(name, value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNewNamedObjNode(JSONNode_t* pParent, const char* name) {
    JSONNode_t* pNewNode = JSONCreateNewNamedObjNode(name);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNamedObjNode(JSONNode_t* pParent, const char* name, JSONObj_t* pObj) {
    JSONNode_t* pNewNode = JSONCreateNamedObjNode(name, pObj);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNewNamedArrayNode(JSONNode_t* pParent, const char* name) {
    JSONNode_t* pNewNode = JSONCreateNewNamedArrayNode(name);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNamedArrayNode(JSONNode_t* pParent, const char* name, JSONArray_t* pArray) {
    JSONNode_t* pNewNode = JSONCreateNamedArrayNode(name, pArray);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}

JSONStatus_t JSONNodeAddNullNode(JSONNode_t* pParent) {
    JSONNode_t* pNewNode = JSONCreateNullNode();
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddBoolNode(JSONNode_t* pParent, bool value) {
    JSONNode_t* pNewNode = JSONCreateBoolNode(value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddIntNode(JSONNode_t* pParent, s32 value) {
    JSONNode_t* pNewNode = JSONCreateIntNode(value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddFloatNode(JSONNode_t* pParent, f32 value) {
    JSONNode_t* pNewNode = JSONCreateFloatNode(value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddStringNode(JSONNode_t* pParent, const char* value) {
    JSONNode_t* pNewNode = JSONCreateStrNode(value);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNewObjNode(JSONNode_t* pParent) {
    JSONNode_t* pNewNode = JSONCreateNewObjNode();
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddObjNode(JSONNode_t* pParent, JSONObj_t* pObj) {
    JSONNode_t* pNewNode = JSONCreateObjNode(pObj);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddNewArrayNode(JSONNode_t* pParent) {
    JSONNode_t* pNewNode = JSONCreateNewArrayNode();
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}
JSONStatus_t JSONNodeAddArrayNode(JSONNode_t* pParent, JSONArray_t* pArray) {
    JSONNode_t* pNewNode = JSONCreateArrayNode(pArray);
    return JSONNodeAdoptChildNode(pParent, pNewNode);
}

/// @brief Gets the length of a node's key followed by a ':'.
/// @param pNode The node
/// @return The length
s32 JSONNodeGetPreValLength(JSONNode_t* pNode) {
    if (pNode->pParent != NULL) {
        if (pNode->pParent->type == JSONObjType) {
            return sizeof(STRING_DELIM) + jsonFuncs.strlen(pNode->name) + sizeof(STRING_DELIM) + sizeof(KEYVAL_SEPARATOR);
        }
    }
    return 0;
}
static s32 JSONBoolNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONBoolType);
    return jsonFuncs.strlen(pNode->value.b == 1 ? "true" : "false");
}
static s32 JSONIntNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONIntType);
    return jsonFuncs.snprintf(NULL, 0, "%d", pNode->value.i);
}
static s32 JSONFloatNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONFloatType);
    return jsonFuncs.snprintf(NULL, 0, "%g", pNode->value.f);
}
static s32 JSONStringNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONStringType);
    return sizeof(STRING_DELIM) + jsonFuncs.strlen(pNode->value.str) + sizeof(STRING_DELIM);
}
static s32 JSONObjNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONObjType);

    s32 length = sizeof(JSONOBJ_START) + sizeof(JSONOBJ_END);
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
static s32 JSONArrayNodeGetValueLength(JSONNode_t* pNode) {
    JsonAssert(pNode->type == JSONArrayType);

    s32 length = sizeof(JSONARRAY_START) + sizeof(JSONARRAY_END);
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
s32 JSONNodeGetValueLength(JSONNode_t* pNode) {
    s32 length;
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
s32 JSONNodeGetLength(JSONNode_t* pNode) {
    return JSONNodeGetPreValLength(pNode) + JSONNodeGetValueLength(pNode);
}

static void JSONNodeDumpPreVal(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JsonAssert(pBuffer != NULL);
    if (pNode->pParent == NULL) return;
    if (pNode->pParent->type != JSONObjType) return;

    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;

    s32 nameLength = jsonFuncs.strlen(pNode->name);
    jsonFuncs.strncpy(pBuffer + *pPosition, pNode->name, nameLength);
    (*pPosition) += nameLength;

    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;
    pBuffer[*pPosition] = KEYVAL_SEPARATOR;
    (*pPosition)++;
}
static void JSONNodeNullValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONNullType);

    jsonFuncs.strncpy(pBuffer + *pPosition, "null", 4);
    (*pPosition) += 4;
}
static void JSONNodeBoolValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONBoolType);

    if (pNode->value.b) {
        jsonFuncs.strncpy(pBuffer + *pPosition, "true", 4);
        (*pPosition) += 4;
    } else {
        jsonFuncs.strncpy(pBuffer + *pPosition, "false", 5);
        (*pPosition) += 5;
    }
}
static void JSONNodeIntValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONIntType);

    s32 length = JSONIntNodeGetValueLength(pNode);
    jsonFuncs.snprintf(pBuffer + *pPosition, length+1, "%d", pNode->value.i);
    (*pPosition) += length;
}
static void JSONNodeFloatValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONFloatType);
    
    s32 length = JSONFloatNodeGetValueLength(pNode);
    jsonFuncs.snprintf(pBuffer + *pPosition, length+1, "%g", pNode->value.f);
    (*pPosition) += length;
}
static void JSONNodeStringValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JsonAssert(pBuffer != NULL);
    JsonAssert(pNode->type == JSONStringType);

    const char* str = pNode->value.str;
    s32 strLength = jsonFuncs.strlen(str);
    
    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;

    jsonFuncs.strncpy(pBuffer + *pPosition, str, strLength);
    (*pPosition) += strLength;

    pBuffer[*pPosition] = STRING_DELIM;
    (*pPosition)++;
}
static void JSONNodeObjValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
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
static void JSONNodeArrayValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
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

void JSONNodeValueDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
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
            JsonAssert(false); // this will never happen unless something goes insanely terribly wrong, just end it right there
        }
    }
}

void JSONNodeDump(JSONNode_t* pNode, char* pBuffer, s32* pPosition) {
    JSONNodeDumpPreVal(pNode, pBuffer, pPosition);
    JSONNodeValueDump(pNode, pBuffer, pPosition);
}

/// @brief Recursively dumps a JSONNode
/// @param pRoot The root of the tree to dump (or a single node if that's what you want to dump)
/// @return The string representation of the JSON node. Must be freed.
const char* JSONDump(JSONNode_t* pRoot) {
    char* pBuffer = NULL;
    s32 length = JSONNodeGetLength(pRoot);
    pBuffer = (char*)jsonFuncs.malloc(length + 1);
    s32 position = 0;
    JSONNodeValueDump(pRoot, pBuffer, &position);
    pBuffer[length] = '\0';
    return (const char*)pBuffer;
}
