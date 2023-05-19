#pragma once

#include <kh-core/types.h>

typedef kh_u32 kh_ast_node_id;

typedef enum _kh_ast_node_type {
  KH_AST_NODE_EMPTY,
  KH_AST_NODE_ROOT,
} kh_ast_node_type;

typedef enum _kh_ast_response {
  KH_AST_RESPONSE_OK,
  KH_AST_RESPONSE_ERR,
  KH_AST_RESPONSE_BUFFER_EXHAUSTED,

  /*
   *  If an append is requested from a parent and the requested child is already
   *  occupied this response is returned instead.
   */
  KH_AST_RESPONSE_OCCUPIED,
} kh_ast_response;

typedef struct _kh_ast_node {
  kh_ast_node_type type;
  kh_u8 left;
  kh_u8 right;
} kh_ast_node;

typedef struct _kh_ast_tree {
  kh_ast_node * root;
  kh_sz         sz; // size of the entire tree buffer in bytes.
  
  kh_sz last_node; // Last created node for fast append lookup
} kh_ast_tree;

/*
 *  Initializes a fresh `kh_ast_tree` by filling the root with the approriate values.
 *  NOTE: This only initializes the `tree` structure. No allocations are made, that's
 *  your job.
 */
kh_ast_response kh_ast_init_tree(kh_ast_tree * tree);

/*
 *  Appends a child node to the left of a parent node
 */
kh_ast_response kh_ast_append_left(kh_ast_tree * tree, kh_ast_node_id parent, kh_ast_node_id * child_id_out);
