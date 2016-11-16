#include "BTreeNode.h"
#include "BTreeIndex.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdio>
using namespace std;

	
TEST_CASE( "Leaf Insert", "[leaf]" ) {
	BTLeafNode leaf = BTLeafNode();
	RecordId rid;
	rid.sid = 1;
	rid.pid = 2;
	SECTION("Single Insert") {
		REQUIRE( leaf.insert(1,rid) == 0 );
	}
	SECTION( "rid with negative pid" ) {
		RecordId ridInvalid;
		ridInvalid.sid = 0;
		ridInvalid.pid = -1;
		REQUIRE( leaf.insert(-10,ridInvalid) == RC_INVALID_RID);
	
	}
	SECTION("Inserts in sorted order") {

		leaf.insert(5,rid);
		leaf.insert(2,rid);
		leaf.insert(8,rid);
		leaf.insert(10,rid);
		leaf.insert(9,rid);
		leaf.insert(3,rid);
		leaf.insert(1,rid);
		leaf.insert(4,rid);
		leaf.insert(7,rid);
		leaf.insert(6,rid);

		int key;
		for (int i = 0; i < 10;i++) {
			leaf.readEntry(i,key,rid);
			REQUIRE(( (key == i+1) && (rid.sid ==1) && (rid.pid == 2)));
		}
	}
	SECTION("Insert to full leaf") {
		for (int i = 1; i<=BTLeafNode::MAX_KEYS;i++) {
			leaf.insert(i,rid);
		}
		REQUIRE(leaf.insert(BTLeafNode::MAX_KEYS+1,rid)==RC_NODE_FULL);
	}
	SECTION("Insert and Locate") {
		leaf.setNextNodePtr(2);
		leaf.insert(0,rid);
		leaf.insert(3,rid);
		leaf.insert(7,rid);
		leaf.insert(10,rid);
		leaf.insert(14,rid);
		leaf.printBuf();
		int eid;
		REQUIRE(leaf.locate(7,eid) == 0);
		REQUIRE(eid == 2);
		REQUIRE(leaf.locate(2,eid) == RC_NO_SUCH_RECORD);
		REQUIRE(eid == 1);
		REQUIRE(leaf.locate(15,eid) == RC_NO_SUCH_RECORD);
		REQUIRE(eid == 5);
		REQUIRE(leaf.locate(9,eid) == RC_NO_SUCH_RECORD);
		REQUIRE(eid == 3);
	}
	SECTION("Insert and Split"){
		for (int i = 1; i <=BTLeafNode::MAX_KEYS;i++) {
			leaf.insert(i,rid);
		}
		leaf.setNextNodePtr(2);

		// leaf.printBuf();

		BTLeafNode leafSib = BTLeafNode();
		int siblingKey;
		REQUIRE(leaf.insertAndSplit(BTLeafNode::MAX_KEYS+1,rid,leafSib,siblingKey) == 0);
		REQUIRE(siblingKey == ((BTLeafNode::MAX_KEYS+1)/2)+ 1);
		leaf.printBuf();
		leafSib.printBuf();
	}

}
// // Redirect cout.
// streambuf* oldCoutStreamBuf = cout.rdbuf();
// ostringstream strCout;
// cout.rdbuf( strCout.rdbuf() );
// // This goes to the string stream.
// cout << "Hello, World!" << endl;

// // Restore old cout.
// cout.rdbuf( oldCoutStreamBuf );

// // Will output our Hello World! from above.
// REQUIRE(strCout.str() == "Hello, World!\n");

// for (int i = 11; i <85;i++) {
//   leaf->insert(i,rid);
// }





// PageFile* pf = new PageFile();
// pf->open("test.tbl",'w');
// leaf->write(pf->endPid(), *pf);

// BTLeafNode* leaf2 = new BTLeafNode();
// char buf[1024];
// leaf2->read(pf->endPid()-1,*pf);


/////////////////////////////
///// test nonLeaf nodes
/////////////////////////////
TEST_CASE("NonLeafNode Operations"){
	BTNonLeafNode node = BTNonLeafNode();

	SECTION("Insert in order") {
		node.insert(5,5);
		node.insert(2,2);
		node.insert(8,8);
		node.insert(10,10);
		node.insert(9,9);
		node.insert(3,3);
		node.insert(1,1);
		node.insert(4,4);
		node.insert(7,7);
		node.insert(6,6);
		int key;
		PageId pid;
		for (int i = 0; i < 10; i++) {
			REQUIRE(node.readEntry(i,key,pid) == 0);
			REQUIRE( ((key == i+1) && (pid == i+1)));
		}
	}
	SECTION("Insert to full node") {
		for (int i = 1; i <= BTNonLeafNode::MAX_KEYS;i++) {
			node.insert(i,i+1);
		}
		REQUIRE( node.insert(1000,1000) == RC_NODE_FULL);
	}

	SECTION("Insert and split") {
		for (int i = 1; i <= BTNonLeafNode::MAX_KEYS;i++) {
			node.insert(i,i+1);
		}
		BTNonLeafNode sib = BTNonLeafNode();
		int midKey;
		REQUIRE(node.insertAndSplit(BTNonLeafNode::MAX_KEYS+1, BTNonLeafNode::MAX_KEYS+2,sib,midKey) == 0);
		// node.printBuf();
		// sib.printBuf();
		REQUIRE(midKey == BTNonLeafNode::MAX_KEYS/2 + 1);
		int key;
		PageId pid;
		const int numTests = 2;
		int testEids[numTests] = {0,62};
		RC expectedResults[numTests] = {0,0};
		int expectedKeys[numTests] = {1,63};
		for (int i = 0; i < numTests; i++) {
			REQUIRE(node.readEntry(testEids[i],key,pid) == 0);
			REQUIRE(key == expectedKeys[i]);
			REQUIRE(pid == expectedKeys[i]+1);

		}
		REQUIRE(node.readEntry(63,key,pid) == RC_NO_SUCH_RECORD);
		REQUIRE(sib.readEntry(0,key,pid) == 0);
		REQUIRE(key == 65);
		REQUIRE(sib.readEntry(62,key,pid) == 0);
		REQUIRE(key == 127);
		REQUIRE(sib.readEntry(63,key,pid) == RC_NO_SUCH_RECORD);
		// REQUIRE()
	}
	SECTION("Locate Child Ptr") {
		node.insert(50,1);
		node.insert(20,2);
		node.insert(80,3);
		node.insert(100,4);
		node.insert(90,5);
		node.insert(30,6);
		node.insert(10,7);
		node.insert(40,8);
		node.insert(70,9);
		node.insert(60,10);

		PageId pid;
		// will have to change this because the very first pid isnt being 
		// set by anything so by default is 0
		REQUIRE(node.locateChildPtr(5,pid) == RC_NO_SUCH_RECORD);
		REQUIRE(pid == 0);

		REQUIRE(node.locateChildPtr(10,pid) == 0);
		REQUIRE(pid == 7);
		REQUIRE(node.locateChildPtr(15,pid) == RC_NO_SUCH_RECORD);
		REQUIRE(pid == 7);
		REQUIRE(node.locateChildPtr(105,pid) == RC_NO_SUCH_RECORD);
		REQUIRE(pid == 4);
		REQUIRE(node.locateChildPtr(95,pid) == RC_NO_SUCH_RECORD);
		REQUIRE(pid == 5);
		
	}
}

TEST_CASE("BTreeIndex") {
	SECTION("One leaf/root node") {
		// PageFile pf = PageFile();
		// pf.open("index.txt",'w');
		remove("indexTestA.txt");
		BTreeIndex index = BTreeIndex();
		REQUIRE(index.open("indexTestA.txt", 'w') == 0);
		// index.printBuf();

		RecordId rid;
		rid.sid = 0;
		rid.pid = 15;
		for (int i = BTLeafNode::MAX_KEYS; i >=1; i--) {
			REQUIRE(index.insert(i,rid) == 0);
		}
		index.printIndex();
		IndexCursor cursor;
		int key;
		REQUIRE(index.locate(1,cursor) == 0);
		REQUIRE(cursor.pid == 1);
		REQUIRE(cursor.eid == 0);
		REQUIRE(index.readForward(cursor,key,rid) == 0);
		REQUIRE(((key==1) && (rid.pid == 15) && (rid.sid == 0)));

		REQUIRE(index.locate(83,cursor) == 0);
		REQUIRE(cursor.pid == 1);
		REQUIRE(cursor.eid == 82);
		REQUIRE(index.locate(84,cursor) == RC_NO_SUCH_RECORD);


		index.close();
		// remove("indexTestA.txt");
	}
		
	SECTION("One root, two leaves") {
		remove("indexTestB.txt");
		BTreeIndex index = BTreeIndex();
		REQUIRE(index.open("indexTestB.txt", 'w') == 0);
		RecordId rid;
		rid.sid = 0;
		rid.pid = 15;
		for (int i = BTLeafNode::MAX_KEYS; i >=1; i--) {
			REQUIRE(index.insert(i,rid) == 0);
		}
		REQUIRE(index.insert(BTLeafNode::MAX_KEYS+1,rid) == 0);
		index.printIndex();

		IndexCursor cursor;
		index.locate(1, cursor);
		for (int i = 1; i<=BTLeafNode::MAX_KEYS+1;i++) {
			int key;
			index.readForward(cursor, key,rid);
			REQUIRE(key == i);
		}
		index.close();
	}
	cout << endl << endl;
	SECTION("four leaves (3 insert and splits)") {
		remove("indexTestC.txt");
		BTreeIndex index = BTreeIndex();
		REQUIRE(index.open("indexTestC.txt", 'w') == 0);
		RecordId rid;
		rid.sid = 0;
		rid.pid = 15;
		for (int i = 1; i <=BTLeafNode::MAX_KEYS*2+2; i++) {
			REQUIRE(index.insert(i,rid) == 0);
		}
		index.printIndex();

		IndexCursor cursor;
		index.locate(1, cursor);
		for (int i = 1; i<=BTLeafNode::MAX_KEYS*2+2;i++) {
			int key;
			index.readForward(cursor, key,rid);
			REQUIRE(key == i);
		}
		index.close();	
	}
		
	SECTION("height = 3, root (nonleaf) splits") {
		remove("indexTestD.txt");
		BTreeIndex index = BTreeIndex();
		REQUIRE(index.open("indexTestD.txt", 'w') == 0);
		RecordId rid;
		rid.sid = 0;
		rid.pid = 15;
		// (MAX_KEYS/2+1)*126 is enough to fill up 126 leafs wih 42 each. another 83 for 127th leaf. 1 more should 
		// cause root overflow
		int iterations = (BTLeafNode::MAX_KEYS/2+1)*126+83+1;
		for (int i = 1; i <= iterations;i++) {
			REQUIRE(index.insert(i,rid) == 0);
		}
		index.printRoot();
		// index.printIndex();
		IndexCursor cursor;
		index.locate(1, cursor);
		for (int i = 1; i<=iterations;i++) {
			int key;
			index.readForward(cursor, key,rid);
			REQUIRE(key == i);
		}
		index.close();
	}
	SECTION("height = 3, split nonleaf nonroot") {
		remove("indexTestE.txt");
		BTreeIndex index = BTreeIndex();
		REQUIRE(index.open("indexTestE.txt",'w') == 0);
		RecordId rid;
		rid.sid = 0;
		rid.pid = 15;
		int iterations = (BTLeafNode::MAX_KEYS/2+1)*(128+62)+83+1;
		for (int i = 1; i <= iterations;i++) {
			REQUIRE(index.insert(i,rid) == 0);
		}
		index.printRoot();
		// index.printIndex();
		IndexCursor cursor;
		index.locate(1, cursor);
		for (int i = 1; i<=iterations;i++) {
			int key;
			index.readForward(cursor, key,rid);
			REQUIRE(key == i);
		}
		index.close();
		REQUIRE(index.open("madeup.txt",'r') == RC_FILE_OPEN_FAILED);
	}
/*
	SECTION("test LOTS of inserts") {
		remove("indexTestF.txt");
		BTreeIndex index = BTreeIndex();
		REQUIRE(index.open("indexTestF.txt",'w') == 0);
		RecordId rid;
		rid.sid = 0;
		rid.pid = 15;
		// inserting key i, where i is continually incremented, causes leafs to not be 
		// completely filled in. after a split, the previous node is left with half of its keys.
		// Ex: fill leaf up to 83 pairs, next pair will cause a split to 42 and 42 pairs
		// Ex: nonleaf has 127 pairs, next pair will cause a split to 64 and 64
		// Similarly, nonleaf nodes will also be left with half its keys after splitting.
		// Therefore, to fill 3 layers with this method, we have to fill 126 nonleaf nodes
		// halfway (64 pointers to leaves). Each of these leaves has 42. The last (127th)
		// nonleaf node will have 126 of its own leaves with 42, and its last leaf will have 83.
		// Inserting one more will cause overflow at the root.
		
		int iterations = (BTLeafNode::MAX_KEYS/2+1)*(BTNonLeafNode::MAX_KEYS/2+1)*
			BTNonLeafNode::MAX_KEYS + BTNonLeafNode::MAX_KEYS*(BTLeafNode::MAX_KEYS/2+1)+
			BTLeafNode::MAX_KEYS + 1;
		for (int i = 1; i <= iterations;i++) {
			REQUIRE(index.insert(i,rid) == 0);
		}
		index.printRoot();
		index.printPid(130,0);
		index.printPid(196,0);
		index.printPid(67,1);
		// index.printIndex();
		// testing to see whether it can locate a key and read forward all the way to the end
		IndexCursor cursor;
		index.locate(1, cursor);
		for (int i = 1; i<=iterations;i++) {
			int key;
			index.readForward(cursor, key,rid);
			REQUIRE(key == i);
		}
		index.locate((iterations/2)+1, cursor);
		for (int i = (iterations/2)+1; i<=iterations;i++) {
			int key;
			index.readForward(cursor, key,rid);
			REQUIRE(key == i);
		}

		index.close();
		SECTION("test persistence of index") {
			// BTreeIndex index = BTreeIndex();
			REQUIRE(index.open("indexTestF.txt",'w') == 0);
			// IndexCursor cursor;
			// RecordId rid;
			// int iterations = (BTLeafNode::MAX_KEYS/2+1)*(BTNonLeafNode::MAX_KEYS*BTNonLeafNode::MAX_KEYS);
			index.locate(1, cursor);
			for (int i = 1; i<=iterations;i++) {
				int key;
				index.readForward(cursor, key,rid);
				REQUIRE(key == i);
			}
			index.locate((iterations/2)+1, cursor);
			for (int i = (iterations/2)+1; i<=iterations;i++) {
				int key;
				index.readForward(cursor, key,rid);
				REQUIRE(key == i);
			}

			index.close();
		}
	}*/
}


