/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <utilities/Tree.h>

//
// Tree<void*,void*> implementation.
//

Tree<void*,void*>::Tree() :
  root(0), nItems(0), m_Begin(0)
{
}

Tree<void*,void*>::~Tree()
{
  delete m_Begin;
}

void Tree<void*,void*>::traverseNode_Insert(Node *n)
{
  if (!n) return;
  insert(n->key, n->element);
  traverseNode_Insert(n->leftChild);
  traverseNode_Insert(n->rightChild);
}

void Tree<void*,void*>::traverseNode_Remove(Node *n)
{
  if (!n) return;
  traverseNode_Remove(n->leftChild);
  traverseNode_Remove(n->rightChild);
  delete n;
}


Tree<void*,void*>::Tree(const Tree &x) :
  root(0), nItems(0), m_Begin(0)
{
  // Traverse the tree, adding everything encountered.
  traverseNode_Insert(x.root);
  
  m_Begin = new IteratorNode(root, 0);
}

Tree<void*,void*> &Tree<void*,void*>::operator =(const Tree &x)
{
  clear();
  // Traverse the tree, adding everything encountered.
  traverseNode_Insert(x.root);
  
  m_Begin = new IteratorNode(root, 0);
  
  return *this;
}

size_t Tree<void*,void*>::count() const
{
  return nItems;
}

void Tree<void*,void*>::insert(void *key, void *value)
{
  Node *n = new Node;
  n->key = key;
  n->element = value;
  n->height = 0;
  n->parent = 0;
  n->leftChild = 0;
  n->rightChild = 0;
  
  bool inserted = false;
  
  if (lookup(key)) return; // Key already in tree.
  
  if (root == 0)
  {
    root = n; // We are the root node.
    
    m_Begin = new IteratorNode(root, 0);
  }
  else
  {
    // Traverse the tree.
    Node *currentNode = root;
    
    while (!inserted)
    {
      if (key > currentNode->key)
        if (currentNode->rightChild == 0) // We have found our insert point.
        {
          currentNode->rightChild = n;
          n->parent = currentNode;
          inserted = true;
        }
        else
          currentNode = currentNode->rightChild;
      else
        if (currentNode->leftChild == 0) // We have found our insert point.
        {
          currentNode->leftChild = n;
          n->parent = currentNode;
          inserted = true;
        }
        else
          currentNode = currentNode->leftChild;
    }
    
    // The value has been inserted, but has that messed up the balance of the tree?
    while (currentNode)
    {
      int b = balanceFactor(currentNode);
      if ( (b<-1) || (b>1) )
        rebalanceNode(currentNode);
      currentNode = currentNode->parent;
    }
  }
  
  nItems++;
}

void *Tree<void*,void*>::lookup(void *key)
{
  Node *n = root;
  while (n != 0)
  {
    if (n->key == key)
      return n->element;
    else if (reinterpret_cast<uintptr_t>(n->key) > reinterpret_cast<uintptr_t>(key))
      n = n->leftChild;
    else
      n = n->rightChild;
  }
  return 0;
}

void Tree<void*,void*>::remove(void *key)
{
  Node *n = root;
  while (n != 0)
  {
    if (n->key == key)
      break;
    else if (reinterpret_cast<uintptr_t>(n->key) > reinterpret_cast<uintptr_t>(key))
      n = n->leftChild;
    else
      n = n->rightChild;
  }
  
  Node *orign = n;
  if (n == 0) return;
  
  while (n->leftChild || n->rightChild) // While n is not a leaf.
  {
    size_t hl = height(n->leftChild);
    size_t hr = height(n->rightChild);
    if (hl == 0)
      rotateLeft(n); // N is now a leaf.
    else if (hr == 0)
      rotateRight(n); // N is now a leaf.
    else if (hl <= hr)
    {
      rotateRight(n);
      rotateLeft(n); // These are NOT inverse operations - rotateRight changes n's position.
    }
    else
    {
      rotateLeft(n);
      rotateRight(n);
    }
  }

  // N is now a leaf, so can be easily pruned.
  if (n->parent == 0)
    root = 0;
  else
    if (n->parent->leftChild == n)
      n->parent->leftChild = 0;
    else
      n->parent->rightChild = 0;

  // Work our way up the path, balancing.
  while (n)
  {
    int b = balanceFactor(n);
    if ( (b < -1) || (b > 1) )
      rebalanceNode(n);
    n = n->parent;
  }

  delete orign;
  nItems--;
}

void Tree<void*,void*>::clear()
{
  traverseNode_Remove(root);
  root = 0;
  nItems = 0;
  
  delete m_Begin;
  m_Begin = 0;
}

/// \todo Implement
Tree<void*,void*>::Iterator Tree<void*,void*>::erase(Iterator iter)
{
  static Iterator ret(0);
  return ret;
}

void Tree<void*,void*>::rotateLeft(Node *n)
{
  // See Cormen,Lieserson,Rivest&Stein  pp-> 278 for pseudocode.
  Node *y = n->rightChild;            // Set Y.

  n->rightChild = y->leftChild;       // Turn Y's left subtree into N's right subtree.
  if (y->leftChild != 0)
    y->leftChild->parent = n;

  y->parent = n->parent;              // Link Y's parent to N's parent.
  if (n->parent == 0)
    root = y;
  else if (n == n->parent->leftChild)
    n->parent->leftChild = y;
  else
    n->parent->rightChild = y;
  y->leftChild = n;
  n->parent = y;
}

void Tree<void*,void*>::rotateRight(Node *n)
{
  Node *y = n->leftChild;
  
  n->leftChild = y->rightChild;
  if (y->rightChild != 0)
    y->rightChild->parent = n;
  
  y->parent = n->parent;
  if (n->parent == 0)
    root = y;
  else if (n == n->parent->leftChild)
    n->parent->leftChild = y;
  else
    n->parent->rightChild = y;
  
  y->rightChild = n;
  n->parent = y;
}

size_t Tree<void*,void*>::height(Node *n)
{
  // Assumes: n's children's heights are up to date. Will always be true if balanceFactor 
  //          is called in a bottom-up fashion.
  if (n == 0) return 0;
  
  size_t tempL = 0;
  size_t tempR = 0;
  
  if (n->leftChild != 0)
    tempL = n->leftChild->height;
  if (n->rightChild != 0)
    tempR = n->rightChild->height;
  
  tempL++; // Account for the height increase stepping up to us, its parent.
  tempR++;
  
  if (tempL > tempR) // If one is actually bigger than the other, return that, else return the other.
  {
    n->height = tempL;
    return tempL;
  }
  else
  {
    n->height = tempR;
    return tempR;
  }
}

int Tree<void*,void*>::balanceFactor(Node *n)
{
  return static_cast<int>(height(n->rightChild)) - static_cast<int>(height(n->leftChild));
}

void Tree<void*,void*>::rebalanceNode(Node *n)
{
  // This way of choosing which rotation to do took me AGES to find...
  // See http://www.cmcrossroads.com/bradapp/ftp/src/libs/C++/AvlTrees.html
  int balance = balanceFactor(n);
  if (balance < -1) // If it's left imbalanced, we need a right rotation.
  {
    if (balanceFactor(n->leftChild) > 0) // If its left child is right heavy...
    {
      // We need a RL rotation - left rotate n's left child, then right rotate N.
      rotateLeft(n->leftChild);
      rotateRight(n);
    }
    else
    {
      // RR rotation will do.
      rotateRight(n);
    }
  }
  else if (balance > 1)
  {
    if (balanceFactor(n->rightChild) < 0) // If its right child is left heavy...
    {
      // We need a LR rotation; Right rotate N's right child, then left rotate N.
      rotateRight(n->rightChild);
      rotateLeft(n);
    }
    else
    {
      // LL rotation.
      rotateLeft(n);
    }
  }
}
