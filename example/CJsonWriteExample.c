#include "CJsonWrite/CJsonWrite.h"
#include "CJsonWrite/CJsonWriteExample.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main() {
    // Initialize common CJsonWrite functions. This is useful for embedded systems development
    // For this example I'll use the standard glibc functions
    JSONFuncs_t funcs = {
        .malloc=malloc,
        .free=free,
        .memset=memset,
        .strlen=strlen,
        .snprintf=snprintf,
        .strncpy=strncpy
    };

    // Initialize CJsonWrite with above functions
    CJsonWriteInit(&funcs);

    // Create empty tree
    JSONNode_t* pRoot = JSONCreateNewObjNode();

    // Add string attribute "this is a string"
    JSONNodeAddNamedStringNode(pRoot, "a string", "this is a string");
    // Add int attribute 55
    JSONNodeAddNamedIntNode(pRoot, "an int", 55);
    // Add bool attribute true
    JSONNodeAddNamedBoolNode(pRoot, "boolean", true);

    // Create empty array
    JSONNode_t* pArrayNode = JSONCreateNewNamedArrayNode("some array");
    
    // Add some values to the array
    JSONNodeAddStringNode(pArrayNode, "array element 1");
    JSONNodeAddIntNode(pArrayNode, 55);
    JSONNodeAddNullNode(pArrayNode);

    // Alternatively, create a node and add it to the array later
    JSONNode_t* pFloatNode = JSONCreateFloatNode(5.55);

    // Add our newly-created node to the array
    JSONArrayNodeAddNode(pArrayNode, pFloatNode);

    // Add the array node to our root tree from earlier
    JSONNodeAdoptChildNode(pRoot, pArrayNode);

    // Render the JSON to a string
    const char* full = JSONDump(pRoot);

    // Print the JSON to stdout
    printf("%s\n", full);
    
    // Free the buffer for the rendered JSON string
    funcs.free((void*)full);

    // Destroy the root of the tree, which will recursively free every node
    JSONNodeDestroy(pRoot);

    return 0;
}