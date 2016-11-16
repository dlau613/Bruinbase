/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include <cstring>
#include <iostream>

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    treeHeight = 0;
    memset(buffer,0,PageFile::PAGE_SIZE);
}

RC BTreeIndex::printBuf() {
	PageId pid;
	memcpy(&pid, buffer,sizeof(PageId));
	cout << "rootPid: " << pid << endl;
	int height;
	memcpy(&height, buffer+sizeof(int),sizeof(int));
	cout << "treeHeight: " << height << endl;
	return 0;
}
int BTreeIndex::getTreeHeight() {
	return treeHeight;
}
RC BTreeIndex::printRoot() {
	if (treeHeight == 0) {
		cout << "Index is empty. Height is 0" << endl;
	}
	else if (treeHeight == 1) {
		cout << "Printing root leaf.....pid: " << rootPid << endl;
		BTLeafNode root = BTLeafNode();
		root.read(rootPid, pf);
		root.printBuf();
		cout << "Done" << endl;
	}
	else {
		BTNonLeafNode root = BTNonLeafNode();
		cout << "Printing root, pid: " << rootPid << ",key count: " << root.getKeyCount() << "..." << endl;
		root.read(rootPid,pf);
		root.printBuf();
		cout << "Done" << endl;
	}
	return 0;
}
// 1-> leaf, 0->nonleaf
RC BTreeIndex::printPid(PageId pid, int leaf) {
	if (leaf) {
		BTLeafNode leaf = BTLeafNode();
		leaf.read(pid,pf);
		leaf.printBuf();
	}
	else {
		BTNonLeafNode node = BTNonLeafNode();
		node.read(pid,pf);
		node.printBuf();
	}
	return 0;
}
RC BTreeIndex::printIndex() {
	if (treeHeight == 0) {
		cout << "Index is empty. Height is 0" << endl;
		return 0;
	}
	else if (treeHeight == 1) {
		cout << "Printing root leaf.....pid: " << rootPid << endl;
		BTLeafNode root = BTLeafNode();
		root.read(rootPid, pf);
		root.printBuf();
		cout << "Done" << endl;
		return 0;
	}
	else {
		printIndexRec(rootPid, 1);
	}
	return 0;
}
RC BTreeIndex::printIndexRec(PageId pid, int currentHeight) {
	// if leaf
	cout << "Printing node at pid: " << pid << endl;
	if (currentHeight == treeHeight) {
		BTLeafNode leaf = BTLeafNode();
		leaf.read(pid,pf);
		leaf.printBuf();
		cout << endl;
		return 0;
	}
	// if not leaf
	else { //if (currentHeight == 1) {
		BTNonLeafNode root = BTNonLeafNode();
		root.read(pid,pf);
		root.printBuf();
		cout << endl;
		printIndexRec(root.getFirstPid(), currentHeight+1);

		for (int i = 0; i < root.getKeyCount();i++) {
			int key;
			PageId childPid;
			root.readEntry(i,key,childPid);
			printIndexRec(childPid,currentHeight+1);
		}
		// int szPair = sizeof(PageId) + sizeof(int);
		// for (int i = szPair; i < szPair+root.getKeyCount()*szPair;i+=szPair) {
		// 	PageId childPid;
		// 	memcpy(&childPid,buffer+i+sizeof(int),sizeof(int));
		// 	printIndexRec(childPid, currentHeight + 1);
		// }
		return 0;
	}

}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
	int error = pf.open(indexname, mode);
	if (error != 0) {
		return error;
	}

	// if no error, read pid 0 from the pf into buffer
	pf.read(0, buffer);
	memcpy(&rootPid, buffer, sizeof(int));
	memcpy(&treeHeight, buffer + sizeof(int), sizeof(int));
	cout << "Opened " << indexname << ". Loaded rootPid: " << rootPid << " and height: " << treeHeight << endl;
	pf.write(0, buffer);
	return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
	memcpy(buffer, &rootPid, sizeof(PageId));
	memcpy(buffer+sizeof(int), &treeHeight, sizeof(int));
	pf.write(0,buffer);
	cout << "Closing...Wrote rootPid: " << rootPid << " and height: " << treeHeight << endl;
    return pf.close();
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
	if (treeHeight == 0) {
		treeHeight += 1;
		BTLeafNode leafRoot = BTLeafNode();
		rootPid = 1;
		leafRoot.setNextNodePtr(0);
		leafRoot.insert(key,rid);
		leafRoot.write(rootPid,pf);
		return 0;
	}
	// locate the leaf node to insert to
	// IndexCursor cursor;
	// locate(key, cursor);
	// BTLeafNode leaf = BTLeafNode();
	// leaf.read(cursor.pid, pf);
	// // if its not full then insert
	// int error;
	// if ((error = leaf.insert(key,rid)) == 0) {
	// 	leaf.write(cursor.pid,pf);
	// 	return 0;
	// }
	// else {
		int newKey;
		PageId newNodePid;
		return insertHelper(rootPid,key, rid, 1, newKey,newNodePid);
	// }
	// else split the leaf
	// 	create new leaf
	// 	insertandsplit
	// 	insert the smallest key from new leaf into the parent
	// 		if the parent is full, insert and split
	// 	repeat until find a parent that isnt full
	// if splititng root, create new root which has one key and two pointers
    // return 0;
}

RC BTreeIndex::insertHelper(PageId pid, int key, const RecordId& rid, int currentHeight,int& newKey, PageId& newNodePid) {
	// keep recursing till you find leaf node
	// if at leaf node, try inserting to it. if success, done, otherwise insertAndSplit
	if (currentHeight == treeHeight) {
		BTLeafNode leaf = BTLeafNode();
		leaf.read(pid, pf);
		if (leaf.getKeyCount() < BTLeafNode::MAX_KEYS) {
			leaf.insert(key,rid);
			leaf.write(pid,pf);
			return 0;
		}
		// need to split
		else {
			// create a new leaf node to split with. grab the next pid
			BTLeafNode newLeaf = BTLeafNode();
			PageId prevNextPid = leaf.getNextNodePtr();
			PageId newLeafPid = pf.endPid();
			newNodePid = newLeafPid;

			// insertandsplit and update the nextnodeptrs. then write them to disk
			leaf.insertAndSplit(key, rid, newLeaf, newKey);
			leaf.setNextNodePtr(newLeafPid);
			leaf.write(pid,pf);
			newLeaf.setNextNodePtr(prevNextPid);
			newLeaf.write(newLeafPid,pf);
			// if the leaf is also the root
			if (treeHeight == 1) {
				cout << "SPLIT ROOT LEAF" << endl;
				// create new root 
				BTNonLeafNode newRoot = BTNonLeafNode();
				rootPid = pf.endPid();
				treeHeight += 1;
				newRoot.initializeRoot(pid, newKey, newLeafPid);
				newRoot.write(rootPid,pf);
				return 0;
			}
			// return error and pass the sibling key and new pid back to caller so they can update
			return RC_NODE_FULL;
		}
	}
	// if nonleaf, keep looking for leaf
	else {
		// 	create a nonleaf node and read the pid into its buffer
		// cout << "SPAM" << endl;
		BTNonLeafNode nonLeaf = BTNonLeafNode();
		nonLeaf.read(pid,pf);	
		// 	search the node for the correct childptr
		PageId nextPid;
		nonLeaf.locateChildPtr(key,nextPid);
		// 	return recurse on new childptr and currenheight+1	
		int error = insertHelper(nextPid,key,rid,currentHeight+1, newKey, newNodePid);
		// if a successful insertion without splitting was done return 0
		if (error == 0) {
			// cout << "no error" << endl;
			nonLeaf.write(pid,pf);
			return 0;
		}
		// if a split was done
		else { // error == RC_NODE_FULL
			// if the current node isnt full then insert the key and pid of the new node
			if (nonLeaf.getKeyCount() != BTNonLeafNode::MAX_KEYS) {
				// cout << "inserting newKey: " << newKey << " and newNodePid: " << newNodePid << endl;
				if ((error = nonLeaf.insert(newKey,newNodePid)) != 0)
					return error; 
				nonLeaf.write(pid,pf);
				return 0;
			}
			// if current node is root
			if (currentHeight == 1) {
				cout << "SPLITTING ROOT NONLEAF" << endl;
				// create a new node and split current node with it
				BTNonLeafNode newNonLeaf = BTNonLeafNode();
				int keyFromChild = newKey;
				nonLeaf.insertAndSplit(keyFromChild, newNodePid, newNonLeaf, newKey);
				nonLeaf.write(pid,pf);
				PageId newNodePid = pf.endPid();
				newNonLeaf.write(newNodePid,pf);

				// create a new root
				BTNonLeafNode newRoot = BTNonLeafNode();
				treeHeight += 1;
				this->rootPid = pf.endPid();
				newRoot.initializeRoot(pid, newKey, newNodePid);
				// newRoot.initializeRoot(rootPid, newKey, newNodePid);
				newRoot.write(rootPid,pf);
				return 0;
			}
			// if current node is not the root
			else {
				// cout << "SPLITTING NONLEAF NONROOT" << endl;
				//create a node to split with, get a pid for it. insertand split will return 
				// a mid key that should be passed up. also pass up the pid of the new node
				BTNonLeafNode newNonLeaf = BTNonLeafNode();
				PageId newNonLeafPid = pf.endPid();
				int keyFromChild = newKey;
				nonLeaf.insertAndSplit(keyFromChild,newNodePid, newNonLeaf, newKey);
				newNodePid = newNonLeafPid;
				nonLeaf.write(pid,pf);
				newNonLeaf.write(newNonLeafPid,pf);
				return RC_NODE_FULL;
			}
		}
	}
}

/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
	return locateHelper(searchKey, rootPid, cursor, 1);
}

RC BTreeIndex::locateHelper(int searchKey, PageId pid, IndexCursor& cursor, int currentHeight) {
	// if node is a leaf (based off of current height)
	if (currentHeight == treeHeight) {
		// 	create a leaf node and read the pid into its buffer
		BTLeafNode leaf = BTLeafNode();
		leaf.read(pid,pf);
		// 	search the leaf for the key and store the location in cursor
		cursor.pid = pid;
		return leaf.locate(searchKey, cursor.eid);
	}
	// if node is nonleaf
	else {
		// 	create a nonleaf node and read the pid into its buffer
		BTNonLeafNode nonleaf = BTNonLeafNode();
		nonleaf.read(pid,pf);	
		// 	search the node for the correct childptr
		PageId nextPid;
		nonleaf.locateChildPtr(searchKey,nextPid);
		// 	return recurse on new childptr and currenheight+1	
		return locateHelper(searchKey,nextPid,cursor,currentHeight+1);
	}
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
	int error;
	BTLeafNode leaf = BTLeafNode();
	if ((error = leaf.read(cursor.pid, pf)) != 0)
		return error;
	if ((error = leaf.readEntry(cursor.eid, key, rid)) != 0)
		return error;
	// move cursor forward to next entry
	if (cursor.eid == (leaf.getKeyCount() - 1)) {
		cursor.pid = leaf.getNextNodePtr();
		cursor.eid = 0;
	}
	else {
		cursor.eid+=1;
	}
    return 0;
}
