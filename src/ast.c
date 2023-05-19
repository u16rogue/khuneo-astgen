#include <kh-astgen/ast.h>

static const kh_u8 INVALID_NODE_OFFSET = KH_U8_INVALID;

kh_ast_response kh_ast_init_tree(kh_ast_tree * tree) {
  kh_ast_node * root = &tree->root[0];

  root->type  = KH_AST_NODE_ROOT;
  root->left  = INVALID_NODE_OFFSET;
  root->right = INVALID_NODE_OFFSET;

  return KH_AST_RESPONSE_OK;
}

kh_ast_response kh_ast_append_left(kh_ast_tree * tree, kh_ast_node_id parent, kh_ast_node_id * child_id_out) {

  return KH_AST_RESPONSE_ERR;
}

#if 0
kh_ast_node * kh_ast_tree_node_by_id(kh_ast_tree * tree, kh_ast_node_id id) {
  if (id * sizeof(kh_ast_node) >= tree->root_sz)
    return 0;

  return &tree->root[id];
}
#endif
