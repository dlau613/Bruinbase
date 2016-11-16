#include "BTreeNode.h"
#include <iostream>
#include <cstring>
using namespace std;



BTLeafNode::BTLeafNode() {
	memset(buffer, 0, PageFile::PAGE_SIZE);
	int numKeys = 0;
	memcpy(buffer,&numKeys,sizeof(int));
}
RC BTLeafNode::printBuf() {
	cout << "Number of Keys: " << getKeyCount() << endl;
	int szPair = sizeof(RecordId) + sizeof(int);
	for (int i = 4; i < 4+getKeyCount()*szPair;i+=szPair) {
		int key;
		memcpy(&key, buffer+i,sizeof(int));
		cout << key << " ";
	}
	cout << endl;
	cout << "Next node pid: " << getNextNodePtr() << endl;
	return 0;
}
/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{ 
	return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{ 
	return pf.write(pid,buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{ 
	int numKeys;
	memcpy(&numKeys, buffer, sizeof(int));
	return numKeys;; 
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid, int split)
{ 
	if (rid.sid<0 || rid.pid<0) 
		return RC_INVALID_RID;
	// check if full
	int numKeys = getKeyCount();
	// 4 bytes for numKeys, 4 for next pid. floor(1016/12) =84
	if ((numKeys == MAX_KEYS) &&  (split == 0)) {
		cout << "Leaf is full" << endl;
		return RC_NODE_FULL;
	}
	// iterate through pairs (size of pair is size of rid + size of key)
	int szPair = sizeof(int) + sizeof(RecordId);
	int posFirstPair = sizeof(int); //first 4 bytes is number of keys
	int posEndLastPair = posFirstPair + numKeys*szPair;
	int i;
	for (i = posFirstPair; i < posEndLastPair; i+=szPair) {
		// look for pair with key greater than one being inserted
		int val;
		memcpy(&val,buffer+i,sizeof(int));
		if (val > key) {
			break;
		}
	}
	//save next pid
	int nextPageId;
	memcpy(&nextPageId, buffer+PageFile::PAGE_SIZE-sizeof(int),sizeof(int));

	char tmp[PageFile::PAGE_SIZE];
	memset(tmp,0,PageFile::PAGE_SIZE);

	// copy the first i bytes
	memcpy(tmp,buffer,i);

	// copy the key and rid
	memcpy(tmp+i,&key,sizeof(int));
	memcpy(tmp+i+sizeof(int), &rid,sizeof(RecordId));

	// copy almost everything after i, and then the next pid
	memcpy(tmp+i+szPair,buffer+i,PageFile::PAGE_SIZE-i-
		sizeof(PageId)-sizeof(RecordId));
	memcpy(tmp+PageFile::PAGE_SIZE-sizeof(int),&nextPageId,sizeof(PageId));

	//update numKeys and finally copy tmp to buffer
	numKeys+=1;
	memcpy(tmp,&numKeys,sizeof(int));
	memcpy(buffer,tmp,PageFile::PAGE_SIZE);
	return 0; 
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ 
	if (rid.sid<0 || rid.pid<0)
		return RC_INVALID_RID;

	// called when node is full, so call insert with split=1 to ignore that it's "full" 
	insert(key,rid,1);
	int numKeys = getKeyCount();
	int numKeysLeft = numKeys/2;
	int numKeysRight = numKeys - numKeysLeft;
	// use a loop of inserts since can't directly set its buffer
	int szInt = sizeof(int);
	int szRid = sizeof(RecordId);
	int szPair = szInt + szRid;
	int startEid = numKeysLeft;
	int lastEid = numKeys -1;
	for (int eid = startEid; eid <= lastEid;eid++) {
		int keyToTransfer;
		RecordId ridSib;
		readEntry(eid, keyToTransfer, ridSib);
		sibling.insert(keyToTransfer,ridSib);
	}
	memset(buffer+szInt+numKeysLeft*szPair,0,numKeysRight*szPair);
	memcpy(buffer,&numKeysLeft,szInt);

	RecordId tmpRid;
	sibling.readEntry(0,siblingKey, tmpRid);
	return 0; 
}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
                   behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{ 
	int numKeys = getKeyCount();
	// if (numKeys == 0) {

	// }
	int szInt = sizeof(int);
	int szPair = szInt + sizeof(RecordId);
	// use a binary search and start in the middle
	int left = 0;
	int right = numKeys - 1;
	int key;
	int middle;
	while (left <= right) {
		middle = (left + right) / 2;
		int bytePos = szInt + middle*szPair;
		memcpy(&key,buffer+bytePos,sizeof(int));
		if (key == searchKey) {
			eid = middle;
			cout << "found it! eid: " << eid << endl;
			return 0;
		}
		else if (key > searchKey)
			right = middle - 1;
		else 
			left = middle + 1;
	}
	if(key > searchKey) {
		eid = middle;
	}
	else {
		eid = middle + 1;
	}
	cout << "couldnt find. eid: " <<eid << endl;
	return RC_NO_SUCH_RECORD; 
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ 
	if (eid<0 || getKeyCount()<=eid) {
		return RC_NO_SUCH_RECORD;
	}
	int szInt = sizeof(int);
	int szRid = sizeof(RecordId);
	int szPair = szInt + szRid;
	memcpy(&key, buffer + szInt +eid*szPair,szInt);
	memcpy(&rid, buffer + szInt +eid*szPair + szInt,szRid);
	return 0; 
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ 	
	PageId nextPid;
	memcpy(&nextPid, buffer+PageFile::PAGE_SIZE-sizeof(int),sizeof(int));
	return nextPid; 
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ 
	if (pid<0) 
		return RC_INVALID_PID;
	memcpy(buffer+PageFile::PAGE_SIZE-sizeof(int), &pid,sizeof(int));
	return 0;

}

/////////////////////////
////////BTNonLeafNOde
/////////////////////////

BTNonLeafNode::BTNonLeafNode() {
	memset(buffer, 0, PageFile::PAGE_SIZE);
	int numKeys = 0;
	memcpy(buffer,&numKeys,sizeof(int));
}
RC BTNonLeafNode::printBuf() {
	cout << "Number of Keys: " << getKeyCount() << endl;
	cout << getFirstPid() << " ";
	int szPair = sizeof(PageId) + sizeof(int);
	for (int i = szPair; i < szPair+getKeyCount()*szPair;i+=szPair) {
		int key;
		memcpy(&key, buffer+i,sizeof(int));
		PageId pid;
		memcpy(&pid,buffer+i+sizeof(int),sizeof(int));
		cout << key << ":" << pid << " ";
	}
	cout << endl;
	return 0;
}

RC BTNonLeafNode::setFirstPid(PageId pid) {
	memcpy(buffer+sizeof(int), &pid, sizeof(int));
	return 0;
}
PageId BTNonLeafNode::getFirstPid() {
	PageId firstPid;
	memcpy(&firstPid, buffer+sizeof(int),sizeof(int));
	return firstPid;
}


/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ 
	if (pid<0) 
		return RC_INVALID_PID;
	return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ 
	if (pid<0) 
		return RC_INVALID_PID;
	return pf.write(pid,buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ 
	int numKeys;
	memcpy(&numKeys, buffer, sizeof(int));
	return numKeys; 
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::readEntry(int eid, int& key, PageId& pid)
{ 
	if (eid<0 || getKeyCount()<=eid) {
		return RC_NO_SUCH_RECORD;
	}
	int szInt = sizeof(int);
	int szPid = sizeof(PageId);
	int szPair = szInt + szPid;
	memcpy(&key, buffer + szPair +eid*szPair,szInt);
	memcpy(&pid, buffer + szPair +eid*szPair + szInt,szPid);
	if (pid<0) 
		return RC_INVALID_PID;
	return 0; 
}

/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid, int split)
{ 
	if (pid<0) 
		return RC_INVALID_PID;
	// check if full
	int numKeys = getKeyCount();
	// 4 bytes for numKeys, 4 for next pid. floor(1016/12) =84
	if ((numKeys == MAX_KEYS) && (split == 0)) {
		cout << "NonLeaf is full" << endl;
		return RC_NODE_FULL;
	}
	// iterate through pairs (size of pair is size of pid + size of key)
	int szInt = sizeof(int);
	int szPid = sizeof(PageId);
	int szPair = szInt + szPid;
	int posFirstPair = szPair; 
	int posEndLastPair = posFirstPair + numKeys*szPair;
	int i;
	for (i = posFirstPair; i < posEndLastPair; i+=szPair) {
		// look for pair with key greater than one being inserted
		int val;
		memcpy(&val,buffer+i,szInt);
		if (val > key) {
			break;
		}
	}
	//save next pid
	// int nextPageId;
	// memcpy(&nextPageId, buffer+PageFile::PAGE_SIZE-sizeof(int),sizeof(int));

	char tmp[PageFile::PAGE_SIZE];
	memset(tmp,0,PageFile::PAGE_SIZE);

	// copy all the pairs that need to be shifted to tmp
	memcpy(tmp,buffer+i,posEndLastPair-i);

	// insert the key and rid
	memcpy(buffer+i,&key,szInt);
	memcpy(buffer+i+szInt, &pid,szPid);

	// copy the pairs back but shifted a pair to the right
	memcpy(buffer+i+szPair,tmp,posEndLastPair-i);

	//update numKeys and finally copy tmp to buffer
	numKeys+=1;
	memcpy(buffer,&numKeys,szInt);
	return 0; 
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ 
	// actually going to split and insert, since insert won't work when node is full
	if (pid<0)
		return RC_INVALID_PID;
	insert(key, pid, 1);
	int numKeys = getKeyCount();
	int numKeysLeft = numKeys/2;
	int numKeysRight = numKeys - numKeysLeft;
	// use a loop of inserts since can't directly set its buffer
	int szInt = sizeof(int);
	int szPid = sizeof(PageId);
	int szPair = szInt + szPid;
	int startEid = numKeysLeft;
	int lastEid = numKeys -1;
	// skip the middle eid (startEid) because it will be moved to parent 
	for (int eid = startEid + 1; eid <= lastEid;eid++) {
		int keyToTransfer;
		PageId pidToTransfer;
		readEntry(eid, keyToTransfer, pidToTransfer);
		sibling.insert(keyToTransfer,pidToTransfer);
	}
	// store the midKey before it's overwritten. also need to put the associated
	// pid into the firstpid spot of the new node
	PageId tmpPid;
	readEntry(startEid,midKey, tmpPid);
	sibling.setFirstPid(tmpPid);
	
	memset(buffer+szPair+numKeysLeft*szPair,0,numKeysRight*szPair);
	memcpy(buffer,&numKeysLeft,szInt);

	// cout << "mid key: " << midKey << endl;
	return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
	int numKeys = getKeyCount();
	// handle case of searchKey being less than first key
	// if (numKeys == 0) {

	// }
	int szInt = sizeof(int);
	int szPid = sizeof(PageId);
	int szPair = szInt + szPid;
	// use a binary search and start in the middle
	int left = 0;
	int right = numKeys - 1;
	int key;
	int middle;
	int bytePos;
	while (left <= right) {
		middle = (left + right) / 2;
		bytePos = szPair + middle*szPair;
		memcpy(&key,buffer+bytePos,szInt);
		if (key == searchKey) {
			memcpy(&pid, buffer+bytePos+szInt,szPid);
			cout << "found " << searchKey << "! pid: " << pid << endl;
			return 0;
		}
		else if (searchKey < key)
			right = middle - 1;
		else 
			left = middle + 1;
	}
	if(searchKey < key) {
		memcpy(&pid,buffer+bytePos+szInt-szPair,szPid);
	}
	else {
		memcpy(&pid,buffer+bytePos+szInt,szPid);
	}
	// cout << "couldnt find " << searchKey << " pid: " <<pid << endl;
	return RC_NO_SUCH_RECORD;  
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ 
	if (pid1<0 || pid2<0) {
		return RC_INVALID_PID;
	}
	setFirstPid(pid1);
	insert(key,pid2);
	return 0;
}
