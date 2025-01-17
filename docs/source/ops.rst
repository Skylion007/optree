Tree Operations
===============

.. currentmodule:: optree

Constants
---------

.. autosummary::

    MAX_RECURSION_DEPTH
    NONE_IS_NODE
    NONE_IS_LEAF

.. autodata:: MAX_RECURSION_DEPTH
.. autodata:: NONE_IS_NODE
.. autodata:: NONE_IS_LEAF

Tree Manipulation Functions
---------------------------

.. autosummary::

    tree_flatten
    tree_flatten_with_path
    tree_unflatten
    tree_leaves
    tree_structure
    tree_paths
    all_leaves
    tree_map
    tree_map_
    tree_map_with_path
    tree_map_with_path_
    tree_reduce
    tree_transpose
    tree_replace_nones
    tree_all
    tree_any
    broadcast_prefix
    prefix_errors

.. autofunction:: tree_flatten
.. autofunction:: tree_flatten_with_path
.. autofunction:: tree_unflatten
.. autofunction:: tree_leaves
.. autofunction:: tree_structure
.. autofunction:: tree_paths
.. autofunction:: all_leaves
.. autofunction:: tree_map
.. autofunction:: tree_map_
.. autofunction:: tree_map_with_path
.. autofunction:: tree_map_with_path_
.. autofunction:: tree_reduce
.. autofunction:: tree_transpose
.. autofunction:: tree_replace_nones
.. autofunction:: tree_all
.. autofunction:: tree_any
.. autofunction:: broadcast_prefix
.. autofunction:: prefix_errors

PyTreeSpec Functions
--------------------

.. autosummary::

    treespec_children
    treespec_is_leaf
    treespec_is_strict_leaf
    treespec_leaf
    treespec_none
    treespec_tuple

.. autofunction:: treespec_children
.. autofunction:: treespec_is_leaf
.. autofunction:: treespec_is_strict_leaf
.. autofunction:: treespec_leaf
.. autofunction:: treespec_none
.. autofunction:: treespec_tuple
