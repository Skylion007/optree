# Copyright 2022-2023 MetaOPT Team. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

# pylint: disable=all

from __future__ import annotations

import builtins
from typing import Any, Callable, Iterable, Sequence

from optree.typing import CustomTreeNode, FlattenFunc, MetaData, PyTree, T, U, UnflattenFunc

class InternalError(RuntimeError): ...

MAX_RECURSION_DEPTH: int

def flatten(
    tree: PyTree[T],
    leaf_predicate: Callable[[T], bool] | None = None,
    node_is_leaf: bool = False,
    namespace: str = '',
) -> builtins.tuple[list[T], PyTreeSpec]: ...
def flatten_with_path(
    tree: PyTree[T],
    leaf_predicate: Callable[[T], bool] | None = None,
    node_is_leaf: bool = False,
    namespace: str = '',
) -> builtins.tuple[list[builtins.tuple[Any, ...]], list[T], PyTreeSpec]: ...
def all_leaves(
    iterable: Iterable[T],
    node_is_leaf: bool = False,
    namespace: str = '',
) -> bool: ...
def leaf(node_is_leaf: bool = False) -> PyTreeSpec: ...
def none(node_is_leaf: bool = False) -> PyTreeSpec: ...
def tuple(treespecs: Sequence[PyTreeSpec], node_is_leaf: bool = False) -> PyTreeSpec: ...
def is_namedtuple_class(cls: type) -> bool: ...
def is_structseq_class(cls: type) -> bool: ...
def structseq_fields(obj: object | type) -> builtins.tuple[str]: ...

class PyTreeSpec:
    num_nodes: int
    num_leaves: int
    num_children: int
    none_is_leaf: bool
    namespace: str
    type: builtins.type | None
    def unflatten(self, leaves: Iterable[T]) -> PyTree[T]: ...
    def flatten_up_to(self, full_tree: PyTree[T]) -> list[PyTree[T]]: ...
    def compose(self, inner_treespec: PyTreeSpec) -> PyTreeSpec: ...
    def walk(
        self,
        f_node: Callable[[builtins.tuple[U, ...], MetaData], U],
        f_leaf: Callable[[T], U] | None,
        leaves: Iterable[T],
    ) -> U: ...
    def children(self) -> list[PyTreeSpec]: ...
    def is_leaf(self, strict: bool = True) -> bool: ...
    def __repr__(self) -> str: ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __len__(self) -> int: ...

def register_node(
    cls: type[CustomTreeNode[T]],
    to_iterable: FlattenFunc,
    from_iterable: UnflattenFunc,
    namespace: str,
) -> None: ...
