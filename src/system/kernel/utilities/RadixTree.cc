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

#include <utilities/RadixTree.h>

//
// RadixTree<void*> implementation.
//

RadixTree<void*>::RadixTree() :
  root(0), nItems(0)
{
}

RadixTree<void*>::~RadixTree()
{
}

RadixTree<void*>::RadixTree(const RadixTree &x) :
  root(0), nItems(0)
{
}

RadixTree<void*> &RadixTree<void*>::operator =(const RadixTree &x)
{
  clear();

  root = cloneNode (x.root, 0);

  return *this;
}

size_t RadixTree<void*>::count() const
{
  return nItems;
}

void RadixTree<void*>::insert(String key, void *value)
{
  // Get the root case out of the way early.
  if (root == 0)
  {
    root = new Node;
    root->key = key;
    root->value = value;
    root->children = 0;
    root->parent = 0;
    root->prev = 0;
    root->_next = 0;
    return;
  }

  // Follow the key down as far as we can.
  Node *n = root;

  const char *cKey = static_cast<const char *> (key);

  // We need to do a little preperation to ensure the invariant/assumption used in the loop body.
  // We assume that the current node has at least one character in common with the key. If this is
  // not the case, a new sibling node is created. So here, we need to scan the top-level siblings
  // to see if any share the first character of the key.
  while (n->_next)
  {
    if (*static_cast<const char *> (n->key) == *cKey)
      break;
    else
      n = n->_next;
  }

  while (true)
  {
    const char *nodeKey = static_cast<const char *> (n->key);
    const char *nodeKeyCopy = nodeKey;

    // Compare cKey with nodeKey, incrementing cKey until it reaches '\0' or nodeKey differs.
    while (*cKey && *nodeKey && *cKey==*nodeKey)
    {
      cKey++;
      nodeKey++;
    }

    // Loop ended - but why?
    if (!*cKey && !*nodeKey)
    {
      // Perfect match - n is the correct node for this key.
      // Overwrite the current value.
      n->value = value;
      return;
    }
    else if (!*cKey)
    {
      // nodeKey was longer than cKey - we need to add a new
      // node as an intermediate.

      // Create the intermediate node.
      Node *inter = new Node;

      inter->children = n->children;
      inter->parent = n;
      n->children = inter;
      inter->prev = inter->_next = 0;

      inter->key = String(nodeKey); // Terminated by nodeKey='\0', above.
      inter->value = n->value;

      // All of inter's new children should be reset with inter as their parent.
      Node *pN = inter->children;
      while (pN)
      {
        pN->parent = inter;
        pN = pN->_next;
      }

      // Set the current node's key to be the uncommon suffix.
      * const_cast<char*>(nodeKey) = '\0';
      n->key = String(nodeKeyCopy);
      n->value = value;

      // Success!
      return;
    }
    else if (!*nodeKey)
    {
      // Iterative case - partial match and nodeKey expired, so
      // we need to look at the node's children.
      Node *pNode = n->children;
      bool found = false;

      while (pNode)
      {
        // We just have to look at the first letter - by
        // inductive assumption we can assume that the
        // first letters in each child are different (else
        // they would be common and pushed into the parent).
        if (static_cast<const char *>(pNode->key)[0] == *cKey)
        {
          n = pNode;
          found = true;
          break;
        }
        pNode = pNode->_next;
      }

      // If we found a match, next iteration.
      if (found)
      {
        continue;
      }

      // No match. Add a child.
      pNode = new Node;
      pNode->parent = n;
      pNode->_next = n->children;
      pNode->prev = 0;
      pNode->key = String(cKey);
      pNode->value = value;
      pNode->children = 0;

      n->children = pNode;

      // Success!
      return;
    }
    else
    {
      // Some (possibly zero) common letters. Take the easy
      // case first - no common letters.
      if (nodeKey == nodeKeyCopy)
      {
        // No common letters, so create a sibling.
        Node *pNode = new Node;
        pNode->parent = n->parent;
        pNode->prev = n;
        pNode->_next = n->_next;
        pNode->key = String(cKey);
        pNode->value = value;
        pNode->children = 0;
        if (pNode->_next)
          pNode->_next->prev = pNode;

        n->_next = pNode;

        // Done.
        return;
      }
      else
      {
        // Some common characters, but not all. Create two children.
        // First child contains n's children, and the suffix of n where it differs from cKey.
        Node *pNode1 = new Node;
        pNode1->parent = n;
        pNode1->prev = 0;
        pNode1->key = String(nodeKey);
        pNode1->value =  n->value;
        pNode1->children = n->children;

        // All of pNode1's new children should be reset with inter as their parent.
        Node *pN = pNode1->children;
        while (pN)
        {
          pN->parent = pNode1;
          pN = pN->_next;
        }

        // Second child contains no children, and the suffix of cKey where it differs from nodeKey.
        Node *pNode2 = new Node;
        pNode2->parent = n;
        pNode2->prev = pNode1;
        pNode2->_next = 0;
        pNode2->key = String(cKey);
        pNode2->value = value;
        pNode2->children = 0;

        // Cross reference node2 from node1...
        pNode1->_next = pNode2;

        // Now, n's children are pNode1 and pNode2;
        n->children = pNode1;

        // Now set n's key to be the common prefix...
        * const_cast<char*> (nodeKey) = '\0';
        n->key = String(nodeKeyCopy);
        n->value = 0;

        // And we're done.
        return;
      }
    }
  }

}

void *RadixTree<void*>::lookup(String key)
{
  // Get the root case out of the way early.
  if (root == 0)
    return 0;

  // Follow the key down as far as we can.
  Node *n = root;
  const char *cKey = static_cast<const char *> (key);

  while (true)
  {
    const char *nodeKey = static_cast<const char *> (n->key);

    // Compare cKey with nodeKey, incrementing cKey until it reaches '\0' or nodeKey differs.
    while (*cKey && *nodeKey && *cKey==*nodeKey)
    {
      cKey++;
      nodeKey++;
    }

    // Why did the loop exit?
    if (cKey == static_cast<const char *> (key))
    {
      // No characters matched. We must be at the root level, so go across to the next root sibling.
      if (n->_next)
      {
        n = n->_next;
        continue;
      }
      else
      {
        // Nothing matched.
        return 0;
      }
    }
    else if (*cKey == '\0' && *nodeKey == '\0')
    {
      return n->value;
    }
    else if (*nodeKey == '\0')
    {
      // Time to move on to the next node.
      n = n->children;
      while (n)
      {
        const char *nodeKey2 = static_cast<const char *> (n->key);

        // We can make the assumption that the initial character of every child will be different, as else they'd have been
        // conjoined. So we just need to find a child that has the correct starting character. Or fail.
        if (*nodeKey2 == *cKey) break;
        n = n->_next;
      }
      if (n) continue;
      // We fail :(
      return 0;
    }
    else
    {
      // Nope. Death.
      return 0;
    }
  }
}

void RadixTree<void*>::remove(String key)
{
  // Get the root case out of the way early.
  if (root == 0)
    return;

  // Follow the key down as far as we can.
  Node *n = root;
  const char *cKey = static_cast<const char *> (key);

  while (true)
  {
    const char *nodeKey = static_cast<const char *> (n->key);

    // Compare cKey with nodeKey, incrementing cKey until it reaches '\0' or nodeKey differs.
    while (*cKey && *nodeKey && *cKey==*nodeKey)
    {
      cKey++;
      nodeKey++;
    }

    // Why did the loop exit?
    if (cKey == static_cast<const char *> (key))
    {
      // No characters matched. We must be at the root level, so go across to the next root sibling.
      if (n->_next)
      {
        n = n->_next;
        continue;
      }
      else
      {
        // Nothing matched.
        return;
      }
    }
    else if (*cKey == '\0' && *nodeKey == '\0')
    {
      // Found it.
      // If we're a parent, we can't delete ourselves - we just have to overwrite the value with zero.
      if (n->children)
      {
        n->value = 0;
        return;
      }
      else
      {
        // We're a leaf, so we can delete ourselves.
        // Jump out of the sibling linked list.
        if (n->_next)
          n->_next->prev = n->prev;
        if (n->prev)
          n->prev->_next = n->_next;

        // Ensure that our parent's children pointer is not pointing at us.
        if (n->parent && n->parent->children == n)
          n->parent->children = n->_next;

        // Nothing is pointing at us, so we can die peacefully.
        delete n;
        return;
      }
    }
    else if (*nodeKey == '\0')
    {
      // Time to move on to the next node.
      n = n->children;
      while (n)
      {
        const char *nodeKey2 = static_cast<const char *> (n->key);
        // We can make the assumption that the initial character of every child will be different, as else they'd have been
        // conjoined. So we just need to find a child that has the correct starting character. Or fail.
        if (*nodeKey2 == *cKey) break;
        n = n->_next;
      }
      if (n) continue;
      // We fail :(
      return;
    }
    else
    {
      // Nope. Death.
      return;
    }
  }
}

void RadixTree<void*>::deleteNode(Node *node)
{
  if (!node) return;

  deleteNode(node->children);

  deleteNode(node->_next);

  delete node;
}

RadixTree<void*>::Node *RadixTree<void*>::cloneNode(Node *node, Node *parent)
{
  // Deal with the easy case first.
  if (!node)
  {
    return 0;
  }

  Node *n = new Node;
  n->key = node->key;
  n->value = node->value;
  n->children = cloneNode(node->children, n);
  n->_next = cloneNode(node->_next, parent);
  if (n->_next)
  {
    n->_next->prev = n;
  }
  n->prev = 0;
  n->parent = parent;

  return n;
}

void RadixTree<void*>::clear()
{
  deleteNode(root);
  root = 0;
}

//
// Explicitly instantiate RadixTree<void*>
//
template class RadixTree<void*>;
